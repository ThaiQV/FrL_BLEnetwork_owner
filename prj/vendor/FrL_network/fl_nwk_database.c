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
											.nwk = {.chn = {37,38,39},.mac_parent = 0xFF},
											};
#endif

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

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
		flash_read_page(ADDR_RTC_START + i * RTC_ENTRY_SIZE,RTC_ENTRY_SIZE,(uint8_t*) &check);
//		LOGA(FLA,"RTC DB(0x%X):0x%X\r\n",ADDR_RTC_START + i * RTC_ENTRY_SIZE,check.magic);
		if (check.magic != RTC_MAGIC) {
			flash_write_page(ADDR_RTC_START + i * RTC_ENTRY_SIZE,RTC_ENTRY_SIZE,(uint8_t*) &entry);
//			LOGA(FLA,"RTC backup(0x%X):%ld\r\n",ADDR_RTC_START + i * RTC_ENTRY_SIZE,entry.timestamp);
			return;
		}
	}
	flash_erase_sector(ADDR_RTC_START);
	flash_write_page(ADDR_RTC_START,RTC_ENTRY_SIZE,(uint8_t*) &entry);
}

u32 fl_db_rtc_load(void) {
	fl_rtc_entry_t entry;
	for (s16 i = RTC_MAX_ENTRIES - 1; i >= 0; i--) {
		flash_read_page(ADDR_RTC_START + i * RTC_ENTRY_SIZE,RTC_ENTRY_SIZE,(uint8_t*) &entry);
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
		flash_erase_sector(ADDR_RTC_START);
		fl_db_rtc_save(RTC_ORIGINAL_TIMETAMP);
	}
}
void fl_db_rtc_factory(void){
	flash_erase_sector(ADDR_RTC_START);
}
/******************** NODELIST FUNCTIONs *******************/
#ifdef MASTER_CORE
void _nodelist_printf(fl_node_data_t *_node, u8 _size){
	LOG_P(FLA,"******** NODELIST ********\r\n");
	for (u8 var = 0; var < _size; ++var) {
		LOGA(FLA,"[%d]Mac:0x%lX\r\n",_node[var].slaveid,_node[var].mac_u32);
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
		flash_read_page(ADDR_NODELIST_NUMSLAVE + var,1,&num_slave_buf);
		if (num_slave_buf == 0xFF) {
			LOGA(FLA,"Num_slave:%d\r\n",nodelist_db.num_slave);
			if (nodelist_db.num_slave && nodelist_db.num_slave != 0xFF) {
				LOGA(FLA,"Load NODELIST at slot(%d)-addr(%08lX)\r\n",var -1,addr_slave_data);
				/* get data slave */
				flash_read_page(addr_slave_data,nodelist_db.num_slave * size_slave_data,(u8*) &nodelist_db.slave);
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
		flash_read_page(ADDR_NODELIST_NUMSLAVE + var,1,&num_slave_buf);
		if (num_slave_buf == 0xFF) {
			addr_slave_data_end = addr_slave_data + nodelist_buf.num_slave * size_slave_data;
			LOGA(FLA,"Save NODELIST at slot(%d)-addr(%08lX - %08lX)\r\n",var,addr_slave_data,addr_slave_data_end);
			if(addr_slave_data_end > ADDR_NODELIST_START+NODELIST_SIZE)goto RE_STORE;
			flash_write_page(ADDR_NODELIST_NUMSLAVE + var,1,(u8*) &nodelist_buf.num_slave);
			flash_write_page(addr_slave_data,nodelist_buf.num_slave * size_slave_data,(u8*) &nodelist_buf.slave);
			return true;
		} else {
			addr_slave_data = addr_slave_data + num_slave_buf * size_slave_data;
		}
	}
	RE_STORE:
	//fully memory -> clear and re-store
	ERR(FLA,"NODELIST overload memory!!!\r\n");
	flash_erase_sector(ADDR_NODELIST_START);
	fl_db_nodelist_save(_pnodelist);
	return false;
}
void fl_db_nodelist_clearAll(void){
	flash_erase_sector(ADDR_NODELIST_START);
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
		flash_read_page(ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
		if (entry.magic == MASTER_PROFILE_MAGIC) {
			LOGA(FLA,"MASTER PROFILE Load(0x%X):%d|%d|%d\r\n",ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,
					entry.nwk.chn[0],entry.nwk.chn[1],entry.nwk.chn[2]);
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
void fl_db_masterprofile_save(fl_db_master_profile_t entry) {
	entry.magic = MASTER_PROFILE_MAGIC;
	fl_db_master_profile_t check;
	for (u16 i = 0; i < MASTER_PROFILE_MAX_ENTRIES; i++) {
		check.magic = 0;
		flash_read_page(ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &check);
		if (check.magic != MASTER_PROFILE_MAGIC) {
			flash_write_page(ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
			LOGA(FLA,"MASTER PROFILE Stored(0x%X):%d|%d|%d\r\n",ADDR_MASTER_PROFILE_START + i * MASTER_PROFILE_ENTRY_SIZE,
					entry.nwk.chn[0],entry.nwk.chn[1],entry.nwk.chn[2]);
			return;
		}
	}
	flash_erase_sector(ADDR_MASTER_PROFILE_START);
	flash_write_page(ADDR_MASTER_PROFILE_START,MASTER_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
}
fl_db_master_profile_t fl_db_masterprofile_init(void) {
	fl_db_master_profile_t profile = { .magic = 0xFFFFFFFF};
	profile = fl_db_masterprofile_load();
	if (profile.magic != MASTER_PROFILE_MAGIC) {
		//clear all and write default profiles
		flash_erase_sector(ADDR_MASTER_PROFILE_START);
		fl_db_masterprofile_save(MASTER_PROFILE_DEFAULT);
		profile = fl_db_masterprofile_load();
	}
	//for debugging
	LOGA(FLA,"Magic: 0x%X\r\n",profile.magic);
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
		flash_read_page(ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
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
		flash_read_page(ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &check);
		if (check.magic != SLAVE_PROFILE_MAGIC) {
			flash_write_page(ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
			LOGA(FLA,"SLAVE PROFILE Stored(0x%X):%d\r\n",ADDR_SLAVE_PROFILE_START + i * SLAVE_PROFILE_ENTRY_SIZE,entry.slaveid);
			return;
		}
	}
	flash_erase_sector(ADDR_SLAVE_PROFILE_START);
	flash_write_page(ADDR_SLAVE_PROFILE_START,SLAVE_PROFILE_ENTRY_SIZE,(uint8_t*) &entry);
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
		flash_erase_sector(ADDR_SLAVE_PROFILE_START);
		fl_db_slaveprofile_save(SLAVE_PROFILE_DEFAULT);
		profile = fl_db_slaveprofile_load();
	}
	//for debugging
	LOGA(FLA,"Magic: 0x%X\r\n",profile.magic);
	LOGA(FLA,"SlaveID:%d\r\n",profile.slaveid);
	LOGA(FLA,"NWK channel:%d |%d |%d \r\n",profile.nwk.chn[0],profile.nwk.chn[1],profile.nwk.chn[2]);
	LOGA(FLA,"NWK Parent(%d):0x%X\r\n",profile.run_stt.join_nwk,profile.nwk.mac_parent);
	LOGA(FLA,"Factory Reset:%d\r\n",profile.run_stt.rst_factory);

	return profile;
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
void fl_db_init(void){
	LOGA(FLA,"RTC ADDR      :0x%X-0x%X\r\n",ADDR_RTC_START,ADDR_RTC_START+RTC_SIZE);
#ifdef MASTER_CORE
	LOGA(FLA,"NODELIST      :0x%X-0x%X\r\n",ADDR_NODELIST_START,ADDR_NODELIST_START+NODELIST_SIZE);
	LOGA(FLA,"MASTERPROFILE :0x%X-0x%X\r\n",ADDR_MASTER_PROFILE_START,ADDR_MASTER_PROFILE_START+MASTERPROFILE_SIZE);
#else
	LOGA(FLA,"SLAVEPROFILE  :0x%X-0x%X\r\n",ADDR_SLAVE_PROFILE_START,ADDR_SLAVE_PROFILE_START+SLAVEPROFILE_SIZE);
#endif
}
void fl_db_all_save(void){
	fl_rtc_set(0); // storage currently time
}

void fl_db_clearAll(void){
	flash_erase_sector(ADDR_RTC_START);
#ifdef MASTER_CORE
	flash_erase_sector(ADDR_NODELIST_START);
	flash_erase_sector(ADDR_MASTER_PROFILE_START);
#else
	flash_erase_sector(ADDR_SLAVE_PROFILE_START);
#endif
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
