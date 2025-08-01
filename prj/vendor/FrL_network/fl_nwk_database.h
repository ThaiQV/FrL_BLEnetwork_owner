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
#define ADDR_USERAREA_START				0x20040000  //

#define SYSTEM_RUNTIME_SIZE				SECTOR_FLASH_SIZE

#define NODELIST_SIZE					SECTOR_FLASH_SIZE
//////// ======================================================================
#define ADDR_RTC_START					(ADDR_USERAREA_START+SECTOR_FLASH_SIZE)

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

#define ADDR_NODELIST_START				(ADDR_RTC_START+SYSTEM_RUNTIME_SIZE)
#define ADDR_NODELIST_NUMSLAVE			ADDR_NODELIST_START
#define NODELIST_NUMSLAVE_SIZE			0x14 //20 bytes
#define ADDR_NODELIST_DATA				(ADDR_NODELIST_START + NODELIST_NUMSLAVE_SIZE)
#endif
//////// ======================================================================
void fl_db_rtc_factory(void);
void fl_db_rtc_init(void);
void fl_db_rtc_save(u32 _timetamp);
u32 fl_db_rtc_load(void) ;
void fl_db_all_save(void);
#ifdef MASTER_CORE
void fl_db_nodelist_init(void);
bool fl_db_nodelist_save(fl_nodelist_db_t *_pnodelist);
bool fl_db_nodelist_load(fl_nodelist_db_t* slavedata_arr);
void fl_db_nodelist_clearAll(void);
#endif
#endif /* VENDOR_FRL_NETWORK_FL_NWK_DATABASE_H_ */
