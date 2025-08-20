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

//#define SYSTEM_RUNTIME_SIZE				SECTOR_FLASH_SIZE
#define RTC_SIZE						SECTOR_FLASH_SIZE

#define NODELIST_SIZE					SECTOR_FLASH_SIZE

#define SLAVEPROFILE_SIZE				SECTOR_FLASH_SIZE

#define MASTERPROFILE_SIZE				SECTOR_FLASH_SIZE
//////// ======================================================================
#define ADDR_RTC_START					(ADDR_USERAREA_START)

/******************** RTC ********************/
#define RTC_ORIGINAL_TIMETAMP			1752473460 // 14/07/2025-11:31:00
#define RTC_MAGIC 						0xFAFAFAFA
#define RTC_ENTRY_SIZE          		8       // 4 bytes timestamp + 4 bytes magic
#define RTC_MAX_ENTRIES         		(RTC_SIZE / RTC_ENTRY_SIZE -1)

#ifdef MASTER_CORE

/******************** NODE LIST ********************/
#define NODELIST_SLAVE_MAX				200
typedef struct {
	uint8_t slaveid;
	uint32_t mac_u32;
}__attribute__((packed)) fl_node_data_t;
typedef struct {
	u8 num_slave;
	fl_node_data_t slave[NODELIST_SLAVE_MAX];
}__attribute__((packed)) fl_nodelist_db_t;

typedef struct {
	struct {
		u8 chn[3];
		//key
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
//		u8 key[];//todo: encrypt
		u32 mac_parent;
	} nwk;
	struct {
		u8 rst_factory;
		u8 join_nwk;
	} run_stt;
	//Don't change
	u32 magic; // constant for LSB
}__attribute__((packed)) fl_slave_profiles_t;

#define ADDR_SLAVE_PROFILE_START  		(ADDR_RTC_START+RTC_SIZE)
#define SLAVE_PROFILE_MAGIC 			0xEDEDEDED
#define SLAVE_PROFILE_ENTRY_SIZE        (sizeof(fl_slave_profiles_t)/sizeof(u8))
#define SLAVE_PROFILE_MAX_ENTRIES       (SLAVEPROFILE_SIZE / SLAVE_PROFILE_ENTRY_SIZE)

fl_slave_profiles_t fl_db_slaveprofile_init(void);
void fl_db_slaveprofile_save(fl_slave_profiles_t entry);
fl_slave_profiles_t fl_db_slaveprofile_load(void);

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
#ifdef MASTER_CORE
void fl_db_nodelist_init(void);
bool fl_db_nodelist_save(fl_nodelist_db_t *_pnodelist);
bool fl_db_nodelist_load(fl_nodelist_db_t* slavedata_arr);
void fl_db_nodelist_clearAll(void);
#endif
#endif /* VENDOR_FRL_NETWORK_FL_NWK_DATABASE_H_ */
