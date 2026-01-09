# FrL_BLEnetwork_owner
This repository is for the Freelux network, which uses the BLE protocol combined with a repeater method to create a long-range data transmission network.

powwermeter:
- uart protocol: 
- pmt driver
- loop: + stpm_monitoring_loop: đọc data từ pmt mỗi 10ms tính toán
        + pmt_report: req server mỗi 60s
-các hàm hỗ trợ send cmd uart pmt_setcalib, pmt_getcalib, pmt_setcalibr, pmt_getcalibr, pmt_read_U, pmt_read_I, pmt_read_P, pmt_print_info 