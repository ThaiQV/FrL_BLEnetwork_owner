/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_database.c
 *Created on		: Jul 15, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_nwk_database.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

typedef struct {
	uint32_t timestamp;
	uint32_t magic; //
}__attribute__((packed)) fl_rtc_entry_t;

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#ifdef MASTER_CORE
fl_db_master_profile_t MASTER_PROFILE_DEFAULT = {
		.magic = MASTER_PROFILE_MAGIC,
		.nwk = {.chn ={10,11,12}},
};
#else
fl_slave_profiles_t SLAVE_PROFILE_DEFAULT = {
											.slaveid = 0xFF,
											.magic= SLAVE_PROFILE_MAGIC,
											.run_stt.rst_factory = 0,
											.run_stt.join_nwk = 0,
											.nwk = {.chn = {37,38,39}},
											};
fl_slave_settings_t SLAVE_SETTINGS_DEFAULT = {
											.magic= SLAVE_SETTINGS_MAGIC,
											};
fl_slave_userdata_t SLAVE_USERDATA_DEFAULT = {
											.data={.len=40},
											.magic= SLAVE_USERDATA_MAGIC,
											};
fl_tbs_data_t TBS_PROFILE_DEFAULT = 		{
											.magic= TBS_PROFILE_MAGIC,
											};
#endif

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

void _db_earse_sector_full(u32 _addr, u8 _size){
	for (u8 var = 0; var < _size; ++var) {
		flash_erase_sector(_addr + var*SECTOR_FLASH_SIZE);
	}
}

bool _db_recheck_write(u32 _addr,u16 _sample_size, u8* _sample_check){
#ifdef MASTER_CORE
	u8 data_buff[2024];
#else
	u8 data_buff[250];
#endif
	memset(data_buff,0,sizeof(data_buff));
	flash_page_program(_addr,_sample_size,_sample_check);
	//recheck
	flash_dread(_addr,_sample_size,data_buff);
	return (memcmp(data_buff,_sample_check,_sample_size) == 0);
}

uint32_t fl_db_crc32(uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
    return ~crc;
}

/******************** RTC FUNCTIONs *******************/
void fl_db_rtc_save(u32 _timetamp) {
	fl_rtc_entry_t entry = { .timestamp = _timetamp, .magic = RTC_MAGIC };
	fl_rtc_entry_t check;
	check.magic = 0;
	for (u16 i = 0; i < RTC_MAX_ENTRIES; i++) {
		check.magic = 0;
		flash_dread(ADDR_RTC_START + i * RTC_ENTRY_SIZE,RTC_ENTRY_SIZE,(uint8_t*) &check);
//		LOGA(FLA,"RTC DB(0x%X):0x%X\r\n",ADDR_RTC_START + i * RTC_ENTRY_SIZE,check.magic);
		if (check.magic != RTC_MAGIC) {
//			flash_page_program(ADDR_RTC_START + i * RTC_ENTRY_SIZE,RTC_ENTRY_SIZE,(uint8_t*) &entry);
			if (_db_recheck_write(ADDR_RTC_START + i * RTC_ENTRY_SIZE,RTC_ENTRY_SIZE,(uint8_t*) &entry)) {
//			LOGA(FLA,"RTC backup(0x%X):%ld\r\n",ADDR_RTC_START + i * RTC_ENTRY_SIZE,entry.timestamp);
				return;
			}
		}
	}
//	flash_erase_sector(ADDR_RTC_START);
//	flash_page_program(ADDR_RTC_START,RTC_ENTRY_SIZE,(uint8_t*) &entry);
	_db_earse_sector_full(ADDR_RTC_START,RTC_SIZE/SECTOR_FLASH_SIZE);
	fl_db_rtc_save(_timetamp);
}

u32 fl_db_rtc_load(void) {
	fl_rtc_entry_t entry;
	for (s16 i = RTC_MAX_ENTRIES - 1; i >= 0; i--) {
		flash_dread(ADDR_RTC_START + i * RTC_ENTRY_SIZE,RTC_ENTRY_SIZE,(uint8_t*) &entry);
//		LOGA(FLA,"RTC DB(0x%X):0x%X\r\n",ADDR_RTC_START + i * RTC_ENTRY_SIZE,entry.magic);
		if (entry.magic == RTC_MAGIC) {
//			LOGA(FLA,"RTC load(0x%X):%ld\r\n",ADDR_RTC_START + i * RTC_ENTRY_SIZE,entry.timestamp);
			return entry.timestamp;
		}
	}
	return 0;
}

void fl_db_rtc_init(void) {
	u32 curr_timetamp = fl_db_rtc_load();
	if (curr_timetamp == 0) {
		//clear all and write original time
//		flash_erase_sector(ADDR_RTC_START);
		_db_earse_sector_full(ADDR_RTC_START,RTC_SIZE/SECTOR_FLASH_SIZE);
		fl_db_rtc_save(RTC_ORIGINAL_TIMETAMP);
	}
}
void fl_db_rtc_factory(void){
	_db_earse_sector_full(ADDR_RTC_START,RTC_SIZE/SECTOR_FLASH_SIZE);
}
/******************** NODELIST FUNCTIONs *******************/
#ifdef MASTER_CORE
void _nodelist_printf(fl_node_data_t *_node, u8 _size){
	LOG_P(FLA,"******** NODELIST ********\r\n");
	for (u8 var = 0; var < _size; ++var) {
		LOGA(FLA,"[%d]Mac:0x%02X%02X%02X%02X%02X%02X\r\n",_node[var].slaveid,
				_node[var].mac[0],_node[var].mac[1],_node[var].mac[2],_node[var].mac[3],_node[var].mac[4],_node[var].mac[5]);
	}
	LOG_P(FLA,"******** END *************\r\n");
}
bool fl_db_nodelist_load(fl_nodelist_db_t* slavedata_arr) {
	fl_nodelist_db_t nodelist_db = { .num_slave = 0 };
	u8 num_slave_buf = 0;
	//calculate position slave_data
	unsigned long addr_slave_data = ADDR_NODELIST_DATA;
	const u8 size_slave_data = sizeof(fl_node_data_t)/sizeof(u8);
	for (u8 var = 0; var < NODELIST_NUMSLAVE_SIZE; ++var) {
		flash_dread(ADDR_NODELIST_NUMSLAVE + var,1,&num_slave_buf);
		if (num_slave_buf == 0xFF) {
			LOGA(FLA,"Num_slave:%d\r\n",nodelist_db.num_slave);
			if (nodelist_db.num_slave && nodelist_db.num_slave != 0xFF) {
				LOGA(FLA,"Load NODELIST at slot(%d)-addr(%08lX)\r\n",var -1,addr_slave_data);
				/* get data slave */
				flash_dread(addr_slave_data,nodelist_db.num_slave * size_slave_data,(u8*) &nodelist_db.slave);
				*slavedata_arr = nodelist_db;
				//printf nodelist debug
				_nodelist_printf(nodelist_db.slave,nodelist_db.num_slave);
				return true;
			}
			break;
		} else {
			addr_slave_data = addr_slave_data + nodelist_db.num_slave * size_slave_data;
			nodelist_db.num_slave = num_slave_buf;
		}
	}
	return false;
}
bool fl_db_nodelist_save(fl_nodelist_db_t *_pnodelist){

	fl_nodelist_db_t nodelist_buf ;
	nodelist_buf = *_pnodelist;

//	nodelist_buf.num_slave= 3;
//	nodelist_buf.slave[0].slaveid = 0;
//	nodelist_buf.slave[0].mac_u32 = 0x2F2245D1; //
//	nodelist_buf.slave[1].slaveid = 1;
//	nodelist_buf.slave[1].mac_u32 = 0xC643B3D1; //
//	nodelist_buf.slave[2].slaveid = 2;
//	nodelist_buf.slave[2].mac_u32 = 0x532FF477; //

	unsigned long addr_slave_data = ADDR_NODELIST_DATA;
	unsigned long addr_slave_data_end = 0;
	const u8 size_slave_data = sizeof(fl_node_data_t)/sizeof(u8);

	u8 num_slave_buf = 0;
	for (u8 var = 0; var < NODELIST_NUMSLAVE_SIZE; ++var) {
		flash_dread(ADDR_NODELIST_NUMSLAVE + var,1,&num_slave_buf);
		if (num_slave_buf == 0xFF) {
			addr_slave_data_end = addr_slave_data + nodelist_buf.num_slave * size_slave_data;
			LOGA(FLA,"Save NODELIST at slot(%d)-addr(%08lX - %08lX)\r\n",var,addr_slave_data,addr_slave_data_end);
			if(addr_slave_data_end > ADDR_NODELIST_START+NODELIST_SIZE)goto RE_STORE;
//			flash_page_program(ADDR_NODELIST_NUMSLAVE + var,1,(u8*) &nodelist_buf.num_slave);
//			flash_page_program(addr_slave_data,nodelist_buf.num_slave * size_slave_data,(u8*) &nodelist_buf.slave);
			if (_db_recheck_write(ADDR_NODELIST_NUMSLAVE + var,1,(u8*) &nodelist_buf.num_slave)
				&& _db_recheck_write(addr_slave_data,nodelist_buf.num_slave * size_slave_data,(u8*) &nodelist_buf.slave)) {
				return true;
			}
		} else {
			addr_slave_data = addr_slave_data + num_slave_buf * size_slave_data;
		}
	}
	RE_STORE:
	//fully memory -> clear and re-store
	ERR(FLA,"NODELIST overload memory!!!\r\n");
//	flash_erase_sector(ADDR_NODELIST_START);
	_db_earse_sector_full(ADDR_NODELIST_START,NODELIST_SIZE/SECTOR_FLASH_SIZE);
	fl_db_nodelist_save(_pnodelist);
	return false;
}
void fl_db_nodelist_clearAll(void){
//	flash_erase_sector(ADDR_NODELIST_START);
	_db_earse_sector_full(ADDR_NODELIST_START,NODELIST_SIZE/SECTOR_FLASH_SIZE);
}

/******************** MASTER PROFILE FUNCTIONs *******************/
/***************************************************
 * @brief 		: read profile in flash
 *
 * @param[in] 	:none
 *
 * @return	  	:profile struct
 *
 ***************************************************/
fl_db_master_profile_t fl_db_masterprofile_load(void) {
	fl_db_master_profile_t entry = { .magic = 0xFFFFFFFF};
	for (s16 i = MASTER_PROFILE_MAX_ENTRIES - 1; i >= 0; i--) {
		flash_dread(ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
		if (entry.magic == MASTER_PROFILE_MAGIC) {
			LOGA(FLA,"MASTER PROFILE Load(0x%X):%d|%d|%d\r\n",ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,
					entry.nwk.chn[0],entry.nwk.chn[1],entry.nwk.chn[2]);
			return entry;
		}
	}
	return entry;
}

void fl_db_generate_private_key(u8 *private_key_out) {
    u32 timestamp = clock_time();  //
    u32 rand_val = trng_rand();    //
    u8 mac[6];
    extern u8* 	blc_ll_get_macAddrPublic(void);
    memcpy(mac,blc_ll_get_macAddrPublic(),6);
    for (int i = 0; i < 16; i++) {
        private_key_out[i] = mac[i % 6] ^ ((timestamp >> (i % 4) * 8) & 0xFF) ^ ((rand_val >> (i % 4) * 8) & 0xFF);
    }
}
/***************************************************
 * @brief 		:store profile into the flash
 *
 * @param[in] 	:none
 *
 * @return	  	:profile struct
 *
 ***************************************************/
void fl_db_masterprofile_save(fl_db_master_profile_t entry) {
	entry.magic = MASTER_PROFILE_MAGIC;
	fl_db_master_profile_t check;
	for (u16 i = 0; i < MASTER_PROFILE_MAX_ENTRIES; i++) {
		check.magic = 0;
		flash_dread(ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &check);
		if (check.magic != MASTER_PROFILE_MAGIC) {
//			flash_page_program(ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
			if (_db_recheck_write(ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &entry)) {
				LOGA(FLA,"MASTER PROFILE Stored(0x%X):%d|%d|%d\r\n",ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,entry.nwk.chn[0],
						entry.nwk.chn[1],entry.nwk.chn[2]);
				return;
			}
		}
	}
//	flash_erase_sector(ADDR_MASTER_PROFILE_START);
	_db_earse_sector_full(ADDR_MASTER_PROFILE_START,MASTERPROFILE_SIZE/SECTOR_FLASH_SIZE);
//	flash_page_program(ADDR_MASTER_PROFILE_START,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
	fl_db_masterprofile_save(entry);
}
fl_db_master_profile_t fl_db_masterprofile_init(void) {
	fl_db_master_profile_t profile = { .magic = 0xFFFFFFFF };
	profile = fl_db_masterprofile_load();
	if (profile.magic != MASTER_PROFILE_MAGIC) {
		//clear all and write default profiles
//		flash_erase_sector(ADDR_MASTER_PROFILE_START);
		_db_earse_sector_full(ADDR_MASTER_PROFILE_START,MASTERPROFILE_SIZE/SECTOR_FLASH_SIZE);
		fl_db_generate_private_key(MASTER_PROFILE_DEFAULT.nwk.private_key);
		fl_db_masterprofile_save(MASTER_PROFILE_DEFAULT);
		profile = fl_db_masterprofile_load();
	}
	//for debugging
	LOGA(FLA,"Private_key:0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
			profile.nwk.private_key[0],profile.nwk.private_key[1],profile.nwk.private_key[2],profile.nwk.private_key[3],
			profile.nwk.private_key[4],profile.nwk.private_key[5],profile.nwk.private_key[6],profile.nwk.private_key[7],
			profile.nwk.private_key[8],profile.nwk.private_key[9],profile.nwk.private_key[10],profile.nwk.private_key[11],
			profile.nwk.private_key[12]);
	LOGA(FLA,"NWK channel:%d |%d |%d \r\n",profile.nwk.chn[0],profile.nwk.chn[1],profile.nwk.chn[2]);
	return profile;
}
#endif
/******************** SLAVE PROFILE FUNCTIONs *******************/
#ifndef MASTER_CORE

/***************************************************
 * @brief 		: read profile in flash
 *
 * @param[in] 	:none
 *
 * @return	  	:profile struct
 *
 ***************************************************/
fl_slave_profiles_t fl_db_slaveprofile_load(void) {
	fl_slave_profiles_t entry = {.slaveid = 0xFF};
	for (s16 i = SLAVE_PROFILE_MAX_ENTRIES - 1; i >= 0; i--) {
		flash_dread(ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
		if (entry.magic == SLAVE_PROFILE_MAGIC) {
			LOGA(FLA,"SLAVE PROFILE Load(0x%X):%d\r\n",ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,entry.slaveid);
			return entry;
		}
	}
	return entry;
}
/***************************************************
 * @brief 		:store profile into the flash
 *
 * @param[in] 	:none
 *
 * @return	  	:profile struct
 *
 ***************************************************/
void fl_db_slaveprofile_save(fl_slave_profiles_t entry) {
	entry.magic = SLAVE_PROFILE_MAGIC;
	fl_slave_profiles_t check;
	for (u16 i = 0; i < SLAVE_PROFILE_MAX_ENTRIES; i++) {
		check.magic = 0;
		flash_dread(ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &check);
		if (check.magic != SLAVE_PROFILE_MAGIC) {
			//flash_page_program(ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
			if (_db_recheck_write(ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &entry)) {
				LOGA(FLA,"SLAVE PROFILE Stored(0x%X):%d\r\n",ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,entry.slaveid);
				return;
			}
		}
	}
//	flash_erase_sector(ADDR_SLAVE_PROFILE_START);
	_db_earse_sector_full(ADDR_SLAVE_PROFILE_START,SLAVEPROFILE_SIZE/SECTOR_FLASH_SIZE);
	fl_db_slaveprofile_save(entry);
}

/***************************************************
 * @brief 		: initialization slave profile database
 *
 * @param[in] 	: none
 *
 * @return	  	: none
 *
 ***************************************************/
fl_slave_profiles_t fl_db_slaveprofile_init(void){
	fl_slave_profiles_t profile = {.slaveid = 0xFF};
	profile = fl_db_slaveprofile_load();
	if(profile.magic != SLAVE_PROFILE_MAGIC){
		//clear all and write default profiles
//		flash_erase_sector(ADDR_SLAVE_PROFILE_START);
		_db_earse_sector_full(ADDR_SLAVE_PROFILE_START,SLAVEPROFILE_SIZE/SECTOR_FLASH_SIZE);
		fl_db_slaveprofile_save(SLAVE_PROFILE_DEFAULT);
		profile = fl_db_slaveprofile_load();
	}
	//for debugging
	LOGA(FLA,"SlaveID:%d\r\n",profile.slaveid);
	LOGA(FLA,"NWK channel:%d |%d |%d \r\n",profile.nwk.chn[0],profile.nwk.chn[1],profile.nwk.chn[2]);
	LOGA(FLA,"NWK Parent(%d):0x%X\r\n",profile.run_stt.join_nwk,profile.nwk.mac_parent);
//	P_PRINTFHEX_A(FLA,profile.parameters,sizeof(profile.parameters)/sizeof(profile.parameters[0]),"Parameters(%d}:",sizeof(profile.parameters)/sizeof(profile.parameters[0]));
	LOGA(FLA,"Factory Reset:%d\r\n",profile.run_stt.rst_factory);
	LOGA(FLA,"NWK Key:%s(%02X%02X)\r\n",(profile.nwk.private_key[0] != 0xFF && profile.nwk.private_key[1] != 0xFF )?"*****":"NULL",profile.nwk.private_key[0],profile.nwk.private_key[1]);

	return profile;
}
/***************************************************
 * @brief 		: read settings in flash
 *
 * @param[in] 	:none
 *
 * @return	  	:settings struct
 *
 ***************************************************/
fl_slave_settings_t fl_db_slavesettings_load(void) {
	fl_slave_settings_t entry;
	for (s16 i = SLAVE_SETTINGS_MAX_ENTRIES - 1; i >= 0; i--) {
		flash_dread(ADDR_SLAVE_SETTINGS_START + i * SLAVE_SETTINGS_ENTRY_SIZE,SLAVE_SETTINGS_ENTRY_SIZE,(uint8_t*) &entry);
		if (entry.magic == SLAVE_SETTINGS_MAGIC) {
			LOGA(FLA,"SLAVE SETTINGS Load(0x%X)\r\n",ADDR_SLAVE_SETTINGS_START + i * SLAVE_SETTINGS_ENTRY_SIZE);
			return entry;
		}
	}
	return entry;
}
/***************************************************
 * @brief 		:store settings into the flash
 *
 * @param[in] 	:none
 *
 * @return	  	:settings struct
 *
 ***************************************************/

void fl_db_slavesettings_save(u8 *_data,u8 _size) {
	fl_slave_settings_t entry;
	memset(entry.setting_arr,0,sizeof(entry.setting_arr));
	memcpy(entry.setting_arr,_data,_size);
	entry.magic = SLAVE_SETTINGS_MAGIC;
	fl_slave_settings_t check;
	for (u16 i = 0; i < SLAVE_SETTINGS_MAX_ENTRIES; i++) {
		check.magic = 0;
		flash_dread(ADDR_SLAVE_SETTINGS_START + i * SLAVE_SETTINGS_ENTRY_SIZE,SLAVE_SETTINGS_ENTRY_SIZE,(uint8_t*) &check);
		if (check.magic != SLAVE_SETTINGS_MAGIC) {
//			flash_page_program(ADDR_SLAVE_SETTINGS_START + i * SLAVE_SETTINGS_ENTRY_SIZE,SLAVE_SETTINGS_ENTRY_SIZE,(uint8_t*) &entry);
			if (_db_recheck_write(ADDR_SLAVE_SETTINGS_START + i * SLAVE_SETTINGS_ENTRY_SIZE,SLAVE_SETTINGS_ENTRY_SIZE,(uint8_t*) &entry)) {
				LOGA(FLA,"SLAVE SETTINGS Stored(0x%X)\r\n",ADDR_SLAVE_SETTINGS_START + i * SLAVE_SETTINGS_ENTRY_SIZE);
				return;
			}
		}
	}
//	flash_erase_sector(ADDR_SLAVE_SETTINGS_START);
	_db_earse_sector_full(ADDR_SLAVE_SETTINGS_START,SLAVESETTINGS_SIZE/SECTOR_FLASH_SIZE);
	fl_db_slavesettings_save(_data,_size);
}

/***************************************************
 * @brief 		: initialization slave settings database
 *
 * @param[in] 	: none
 *
 * @return	  	: none
 *
 ***************************************************/
fl_slave_settings_t fl_db_slavesettings_init(void){
	fl_slave_settings_t settings;
	settings = fl_db_slavesettings_load();
	if(settings.magic != SLAVE_SETTINGS_MAGIC){
		//clear all and write default settings
//		flash_erase_sector(ADDR_SLAVE_SETTINGS_START);
		_db_earse_sector_full(ADDR_SLAVE_SETTINGS_START,SLAVESETTINGS_SIZE/SECTOR_FLASH_SIZE);
		memset(SLAVE_SETTINGS_DEFAULT.setting_arr,0,sizeof(SLAVE_SETTINGS_DEFAULT.setting_arr));
		fl_db_slavesettings_save(SLAVE_SETTINGS_DEFAULT.setting_arr,sizeof(SLAVE_SETTINGS_DEFAULT.setting_arr));
		settings = fl_db_slavesettings_load();
	}
	return settings;
}

/***************************************************
 * @brief 		:store userdata into the flash
 *
 * @param[in] 	:none
 *
 * @return	  	:userdata struct
 *
 ***************************************************/
void fl_db_slaveuserdata_save(u8 *_data,u8 _size) {
	fl_slave_userdata_t entry;
	memset(entry.data.payload,0,sizeof(entry.data.payload));
	entry.data.len = _size;
	memcpy(entry.data.payload,_data,_size);
	entry.magic = SLAVE_USERDATA_MAGIC;
	fl_slave_userdata_t check;
	for (u16 i = 0; i < SLAVE_USERDATA_MAX_ENTRIES; i++) {
		check.magic = 0;
		flash_dread(ADDR_SLAVE_USERDATA_START+ i * SLAVE_USERDATA_ENTRY_SIZE,SLAVE_USERDATA_ENTRY_SIZE,(uint8_t*) &check);
		if (check.magic != SLAVE_USERDATA_MAGIC) {
			//flash_page_program(ADDR_SLAVE_USERDATA_START + i * SLAVE_USERDATA_ENTRY_SIZE,SLAVE_USERDATA_ENTRY_SIZE,(uint8_t*) &entry);
			if (_db_recheck_write(ADDR_SLAVE_USERDATA_START + i * SLAVE_USERDATA_ENTRY_SIZE,SLAVE_USERDATA_ENTRY_SIZE,(uint8_t*) &entry)) {
				LOGA(FLA,"SLAVE USERDATA Stored(0x%X)\r\n",ADDR_SLAVE_USERDATA_START + i * SLAVE_USERDATA_ENTRY_SIZE);
				return;
			}
		}
	}
//	flash_erase_sector(ADDR_SLAVE_USERDATA_START);
	_db_earse_sector_full(ADDR_SLAVE_USERDATA_START,SLAVEUSERDATA_SIZE/SECTOR_FLASH_SIZE);
	fl_db_slaveuserdata_save(_data,_size);
}
/***************************************************
 * @brief 		: read userdata in flash
 *
 * @param[in] 	:none
 *
 * @return	  	:userdata struct
 *
 ***************************************************/
fl_slave_userdata_t fl_db_slaveuserdata_load(void) {
	fl_slave_userdata_t entry = {};
	memset(entry.data.payload,0,sizeof(entry.data.payload));
	entry.data.len = 0;
	for (s16 i = SLAVE_USERDATA_MAX_ENTRIES - 1; i >= 0; i--) {
		flash_dread(ADDR_SLAVE_USERDATA_START + i * SLAVE_USERDATA_ENTRY_SIZE,SLAVE_USERDATA_ENTRY_SIZE,(uint8_t*) &entry);
		if (entry.magic == SLAVE_USERDATA_MAGIC) {
			LOGA(FLA,"SLAVE USERDATA Load(0x%X)\r\n",ADDR_SLAVE_USERDATA_START + i * SLAVE_USERDATA_ENTRY_SIZE);
			return entry;
		}
	}
	return entry;
}
/***************************************************
 * @brief 		: initialization slave userdata database
 *
 * @param[in] 	: none
 *
 * @return	  	: none
 *
 ***************************************************/
fl_db_userdata_t fl_db_slaveuserdata_init(void){
	fl_slave_userdata_t userdata;
	userdata = fl_db_slaveuserdata_load();
	if(userdata.magic != SLAVE_USERDATA_MAGIC){
		//clear all and write default userdata
//		flash_erase_sector(ADDR_SLAVE_USERDATA_START);
		_db_earse_sector_full(ADDR_SLAVE_USERDATA_START,SLAVEUSERDATA_SIZE/SECTOR_FLASH_SIZE);
		memset((u8*)SLAVE_USERDATA_DEFAULT.data.payload,0,sizeof(SLAVE_USERDATA_DEFAULT.data.payload));
		fl_db_slaveuserdata_save(SLAVE_USERDATA_DEFAULT.data.payload,sizeof(SLAVE_USERDATA_DEFAULT.data.payload));
		userdata = fl_db_slaveuserdata_load();
	}
	return userdata.data;
}
/***************************************************
 * @brief 		: read tbs profile in flash
 *
 * @param[in] 	:none
 *
 * @return	  	:tbsdata struct
 *
 ***************************************************/
fl_tbs_data_t fl_db_tbsprofile_load(void) {
	fl_tbs_data_t entry = {};
	memset(entry.data,0,sizeof(entry.data));
	for (s16 i = TBS_PROFILE_MAX_ENTRIES - 1; i >= 0; i--) {
		flash_dread(ADDR_TBS_PROFILE_START + i * TBS_PROFILE_ENTRY_SIZE,TBS_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
		if (entry.magic == TBS_PROFILE_MAGIC) {
			LOGA(FLA,"TBS_PROFILE Load(0x%X)\r\n",ADDR_TBS_PROFILE_START + i * TBS_PROFILE_ENTRY_SIZE);
			return entry;
		}
	}
	return entry;
}
/***************************************************
 * @brief 		: initialization tbs profile database
 *
 * @param[in] 	: none
 *
 * @return	  	: none
 *
 ***************************************************/
fl_tbs_data_t fl_db_tbsprofile_init(void){
	fl_tbs_data_t tbsdata;
	tbsdata = fl_db_tbsprofile_load();
	if(tbsdata.magic != TBS_PROFILE_MAGIC){
		//clear all and write default userdata
//		flash_erase_sector(ADDR_TBS_PROFILE_START);
		_db_earse_sector_full(ADDR_TBS_PROFILE_START,TBSPROFILE_SIZE/SECTOR_FLASH_SIZE);
		memset((u8*)TBS_PROFILE_DEFAULT.data,0,sizeof(TBS_PROFILE_DEFAULT.data));
		fl_db_tbsprofile_save(TBS_PROFILE_DEFAULT.data,sizeof(TBS_PROFILE_DEFAULT.data));
		tbsdata = fl_db_tbsprofile_load();
	}
	return tbsdata;
}

/***************************************************
 * @brief 		:store tbsprofile into the flash
 *
 * @param[in] 	:none
 *
 * @return	  	:tbsdata struct
 *
 ***************************************************/
void fl_db_tbsprofile_save(u8 *_data,u8 _size){
	fl_tbs_data_t entry;
	memset(entry.data,0,sizeof(entry.data));
	if(_size>sizeof(entry.data)){
		ERR(FLA,"TBS profile oversize (max:12 bytes)!!!\r\n");
		return;
	}
	memcpy(entry.data,_data,_size);
	entry.magic = TBS_PROFILE_MAGIC;
	fl_tbs_data_t check;
	for (u16 i = 0; i < TBS_PROFILE_MAX_ENTRIES; i++) {
		check.magic = 0;
		flash_dread(ADDR_TBS_PROFILE_START+ i * TBS_PROFILE_ENTRY_SIZE,TBS_PROFILE_ENTRY_SIZE,(uint8_t*) &check);
		if (check.magic != TBS_PROFILE_MAGIC) {
			if (_db_recheck_write(ADDR_TBS_PROFILE_START + i * TBS_PROFILE_ENTRY_SIZE,TBS_PROFILE_ENTRY_SIZE,(uint8_t*) &entry)) {
				LOGA(FLA,"TBS_PROFILE Stored(0x%X)\r\n",ADDR_TBS_PROFILE_START + i * TBS_PROFILE_ENTRY_SIZE);
				return;
			}
		}
	}
//	flash_erase_sector(ADDR_TBS_PROFILE_START);
	_db_earse_sector_full(ADDR_TBS_PROFILE_START,TBSPROFILE_SIZE/SECTOR_FLASH_SIZE);
	fl_db_tbsprofile_save(_data,_size);
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
void fl_db_init(void) {
	//First load firmware to device
	//check flash -> clear all -> first init
	u8 initialized_db[QUANTITY_FIELD_STORED_DB];
	memset(initialized_db,0xFF,QUANTITY_FIELD_STORED_DB);
	flash_dread(ADDR_DATABASE_INITIALIZATION,QUANTITY_FIELD_STORED_DB,(uint8_t*) &initialized_db);
	for (u8 i = 0; i < QUANTITY_FIELD_STORED_DB; i++) {
		if (initialized_db[i] == 0xFF) {
			ERR(FLA,"Factory all DB....\r\n");
			flash_erase_sector(ADDR_USERAREA_END - SECTOR_FLASH_SIZE);
			fl_db_clearAll();
			memset(initialized_db,0x55,QUANTITY_FIELD_STORED_DB);
			//flash_page_program(ADDR_DATABASE_INITIALIZATION,QUANTITY_FIELD_STORED_DB,(uint8_t*) &initialized_db);
			if(_db_recheck_write(ADDR_DATABASE_INITIALIZATION,QUANTITY_FIELD_STORED_DB,(uint8_t*) &initialized_db)){
				break;
			}
		}
	}

	LOGA(FLA,"RTC ADDR      (%d):0x%X-0x%X\r\n",RTC_ENTRY_SIZE,ADDR_RTC_START,ADDR_RTC_START+RTC_SIZE);
#ifdef MASTER_CORE
	LOGA(FLA,"NODELIST      (%d):0x%X-0x%X\r\n",NODELIST_NUMSLAVE_SIZE,ADDR_NODELIST_START,ADDR_NODELIST_START+NODELIST_SIZE);
	LOGA(FLA,"MASTERPROFILE (%d):0x%X-0x%X\r\n",MASTER_PROFILE_ENTRY_SIZE,ADDR_MASTER_PROFILE_START,ADDR_MASTER_PROFILE_START+MASTERPROFILE_SIZE);
#else
	LOGA(FLA,"SLAVE_PROFILE  (%d):0x%X-0x%X\r\n",SLAVE_PROFILE_ENTRY_SIZE,ADDR_SLAVE_PROFILE_START,ADDR_SLAVE_PROFILE_START+SLAVEPROFILE_SIZE);
	LOGA(FLA,"SLAVE_SETTINGS (%d):0x%X-0x%X\r\n",SLAVE_SETTINGS_ENTRY_SIZE,ADDR_SLAVE_SETTINGS_START,ADDR_SLAVE_SETTINGS_START+SLAVESETTINGS_SIZE);
	LOGA(FLA,"SLAVE_USERDATA (%d):0x%X-0x%X\r\n",SLAVE_USERDATA_ENTRY_SIZE,ADDR_SLAVE_USERDATA_START,ADDR_SLAVE_USERDATA_START+SLAVEUSERDATA_SIZE);
	LOGA(FLA,"TBS_PROFILE    (%d):0x%X-0x%X\r\n",TBS_PROFILE_ENTRY_SIZE,ADDR_TBS_PROFILE_START,ADDR_TBS_PROFILE_START+TBSPROFILE_SIZE);
#endif
}
void fl_db_all_save(void){
	fl_rtc_set(0); // storage currently time
}
#ifndef MASTER_CORE
void fl_db_Pairing_Clear(void){
//	flash_erase_sector(ADDR_SLAVE_PROFILE_START);
	_db_earse_sector_full(ADDR_SLAVE_PROFILE_START,SLAVEPROFILE_SIZE/SECTOR_FLASH_SIZE);
}
#endif
void fl_db_clearAll(void){
//	flash_erase_sector(ADDR_RTC_START);
	_db_earse_sector_full(ADDR_RTC_START,RTC_SIZE/SECTOR_FLASH_SIZE);
#ifdef MASTER_CORE
//	flash_erase_sector(ADDR_NODELIST_START);
//	flash_erase_sector(ADDR_MASTER_PROFILE_START);
	_db_earse_sector_full(ADDR_NODELIST_START,NODELIST_SIZE/SECTOR_FLASH_SIZE);
	_db_earse_sector_full(ADDR_MASTER_PROFILE_START,MASTERPROFILE_SIZE/SECTOR_FLASH_SIZE);
#else
//	flash_erase_sector(ADDR_SLAVE_PROFILE_START);
	_db_earse_sector_full(ADDR_SLAVE_PROFILE_START,SLAVEPROFILE_SIZE/SECTOR_FLASH_SIZE);
//	flash_erase_sector(ADDR_SLAVE_SETTINGS_START);
	_db_earse_sector_full(ADDR_SLAVE_SETTINGS_START,SLAVESETTINGS_SIZE/SECTOR_FLASH_SIZE);
//	flash_erase_sector(ADDR_SLAVE_USERDATA_START);
	_db_earse_sector_full(ADDR_SLAVE_USERDATA_START,SLAVEUSERDATA_SIZE/SECTOR_FLASH_SIZE);
	_db_earse_sector_full(ADDR_TBS_PROFILE_START,TBSPROFILE_SIZE/SECTOR_FLASH_SIZE);

#endif
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
