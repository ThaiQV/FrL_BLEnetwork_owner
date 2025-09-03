/**
 * @file data_storage.h
 * @author Nghia Hoang
 * @date 2025
 */

#ifndef DATA_STORAGE_H_
#define DATA_STORAGE_H_

typedef struct {
	union {
		struct {
			uint8_t  is_online;
			uint16_t product_pass;
			uint16_t product_error;
		};

		uint8_t data[5];
	};
}data_save_t;

typedef struct {
	uint32_t timestamp;
	uint8_t  is_online;
	uint16_t product_pass;
	uint16_t product_error;
} data_storage_share_data_t;

void user_datastorage_app_init(void);
void user_datastorage_app_task(void);

#endif /* DATA_STORAGE_H_ */
