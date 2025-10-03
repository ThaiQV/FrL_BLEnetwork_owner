/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_database.h
 *Created on		: Jul 15, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef VENDOR_FRL_NETWORK_FL_NWK_DATABASE_H_
#define VENDOR_FRL_NETWORK_FL_NWK_DATABASE_H_

#define SECTOR_FLASH_SIZE				0x1000 //4096
#define ADDR_USERAREA_START				0x20040000 + SECTOR_FLASH_SIZE //
#define ADDR_USERAREA_END				0x200FFFFF //END FLASH

//////// ======================================================================

#ifdef MASTER_CORE
#define QUANTITY_FIELD_STORED_DB		3 //RTC + NODELIST(master) + MASTER PROFILE
#else
#define QUANTITY_FIELD_STORED_DB		2 //RTC + SLAVE PROFILE
#endif
#define ADDR_DATABASE_INITIALIZATION	(ADDR_USERAREA_END - QUANTITY_FIELD_STORED_DB)	//initialization db
//////// ======================================================================

//#define SYSTEM_RUNTIME_SIZE				SECTOR_FLASH_SIZE
#define RTC_SIZE						SECTOR_FLASH_SIZE

#define NODELIST_SIZE					SECTOR_FLASH_SIZE

#define SLAVEPROFILE_SIZE				SECTOR_FLASH_SIZE

#define SLAVESETTINGS_SIZE				SECTOR_FLASH_SIZE

#define SLAVEUSERDATA_SIZE				SECTOR_FLASH_SIZE

#define MASTERPROFILE_SIZE				SECTOR_FLASH_SIZE

//////// ======================================================================
#define ADDR_RTC_START					(ADDR_USERAREA_START)

/******************** RTC ********************/
#define RTC_ORIGINAL_TIMETAMP			1752473460 // 14/07/2025-11:31:00
#define RTC_MAGIC 						0xFAFAFAFA
#define RTC_ENTRY_SIZE          		8       // 4 bytes timestamp + 4 bytes magic
#define RTC_MAX_ENTRIES         		(RTC_SIZE / RTC_ENTRY_SIZE -1)

#define NWK_PRIVATE_KEY_SIZE 			13
#ifdef MASTER_CORE
/******************** NODE LIST ********************/
#define NODELIST_SLAVE_MAX				200

typedef struct {
	uint8_t slaveid;
	uint8_t mac[6];
	uint8_t dev_type;
}__attribute__((packed)) fl_node_data_t;
typedef struct {
	u8 num_slave;
	fl_node_data_t slave[NODELIST_SLAVE_MAX];
}__attribute__((packed)) fl_nodelist_db_t;

typedef struct {
	struct {
		u8 chn[3];
		//key
		u8 private_key[NWK_PRIVATE_KEY_SIZE];
	} nwk;
	//Don't change
	u32 magic;
}__attribute__((packed)) fl_db_master_profile_t;

#define ADDR_NODELIST_START				(ADDR_RTC_START+RTC_SIZE)
#define ADDR_NODELIST_NUMSLAVE			ADDR_NODELIST_START
#define NODELIST_NUMSLAVE_SIZE			0x14 //20 bytes
#define ADDR_NODELIST_DATA				(ADDR_NODELIST_START + NODELIST_NUMSLAVE_SIZE)

#define ADDR_MASTER_PROFILE_START 		(ADDR_NODELIST_START + NODELIST_SIZE)
#define MASTER_PROFILE_MAGIC			0xECECECEC
#define MASTER_PROFILE_ENTRY_SIZE		(sizeof(fl_db_master_profile_t)/sizeof(u8))
#define MASTER_PROFILE_MAX_ENTRIES		(MASTERPROFILE_SIZE / MASTER_PROFILE_ENTRY_SIZE - 1) //for sure

fl_db_master_profile_t fl_db_masterprofile_init(void);
void fl_db_masterprofile_save(fl_db_master_profile_t entry);
fl_db_master_profile_t fl_db_masterprofile_load(void);
#else

typedef struct {
	u8 slaveid;
	struct {
		u8 chn[3];
		u8 private_key[NWK_PRIVATE_KEY_SIZE];//todo: encrypt
		u32 mac_parent;
	} nwk;
//	u16 parameters[4]; //change size if we need more parametes
	struct {
		u8 rst_factory;
		u8 join_nwk;
	} run_stt;
	//Don't change
	u32 magic; // constant for LSB
}__attribute__((packed)) fl_slave_profiles_t;

#define ADDR_SLAVE_PROFILE_START  		(ADDR_RTC_START + RTC_SIZE)
#define SLAVE_PROFILE_MAGIC 			0xEDEDEDED
#define SLAVE_PROFILE_ENTRY_SIZE        (sizeof(fl_slave_profiles_t)/sizeof(u8))
#define SLAVE_PROFILE_MAX_ENTRIES       (SLAVEPROFILE_SIZE / SLAVE_PROFILE_ENTRY_SIZE - 1)

fl_slave_profiles_t fl_db_slaveprofile_init(void);
void fl_db_slaveprofile_save(fl_slave_profiles_t entry);
fl_slave_profiles_t fl_db_slaveprofile_load(void);

typedef struct {
	u8 setting_arr[10 * (22 +1)]; //MAX size = num of slot LCD * (max size characters + 1 byte f_new)
	//Don't change
	u32 magic; // constant for LSB
}__attribute__((packed)) fl_slave_settings_t;

#define ADDR_SLAVE_SETTINGS_START		(ADDR_SLAVE_PROFILE_START + SLAVEPROFILE_SIZE)
#define SLAVE_SETTINGS_MAGIC 			0xEEEEEEEE
#define SLAVE_SETTINGS_ENTRY_SIZE       (sizeof(fl_slave_settings_t)/sizeof(u8))
#define SLAVE_SETTINGS_MAX_ENTRIES      (SLAVESETTINGS_SIZE / SLAVE_SETTINGS_ENTRY_SIZE - 1)

fl_slave_settings_t fl_db_slavesettings_init(void);
void fl_db_slavesettings_save(u8 *_data,u8 _size);
fl_slave_settings_t fl_db_slavesettings_load(void);

typedef struct {
	u8 len;
	u8 payload[50];
}__attribute__((packed)) fl_db_userdata_t;

typedef struct {
	fl_db_userdata_t data;
	//Don't change
	u32 magic; // constant for LSB
}__attribute__((packed)) fl_slave_userdata_t;

#define ADDR_SLAVE_USERDATA_START		(ADDR_SLAVE_SETTINGS_START + SLAVESETTINGS_SIZE)
#define SLAVE_USERDATA_MAGIC 			0xDDDDDDDD
#define SLAVE_USERDATA_ENTRY_SIZE       (sizeof(fl_slave_userdata_t)/sizeof(u8))
#define SLAVE_USERDATA_MAX_ENTRIES      (SLAVEUSERDATA_SIZE / SLAVE_USERDATA_ENTRY_SIZE - 1)

fl_db_userdata_t fl_db_slaveuserdata_init(void);
void fl_db_slaveuserdata_save(u8 *_data,u8 _size);
fl_slave_userdata_t fl_db_slaveuserdata_load(void);

#endif
//////// ======================================================================
uint32_t fl_db_crc32(uint8_t *data, size_t len);
void fl_db_rtc_factory(void);
void fl_db_rtc_init(void);
void fl_db_rtc_save(u32 _timetamp);
u32 fl_db_rtc_load(void);
void fl_db_init(void);
void fl_db_all_save(void);
void fl_db_clearAll(void);
void fl_db_Pairing_Clear(void);
#ifdef MASTER_CORE
void fl_db_nodelist_init(void);
bool fl_db_nodelist_save(fl_nodelist_db_t *_pnodelist);
bool fl_db_nodelist_load(fl_nodelist_db_t* slavedata_arr);
void fl_db_nodelist_clearAll(void);
#endif
#endif /* VENDOR_FRL_NETWORK_FL_NWK_DATABASE_H_ */
