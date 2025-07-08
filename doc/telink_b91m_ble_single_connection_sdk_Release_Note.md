## V3.3.0.1

### Features
   * Normalizes the naming of content related to private schemas.

### Bug Fixes
   * N/A.

### BREAKING CHANGES
   * N/A.

### Features
   * 规范化与私有模式相关内容的命名

### Bug Fixes
   * N/A.

### BREAKING CHANGES
   * N/A.

## V3.3.0.0

### Dependency Updates
   * telink_b91m_driver_src SHA-1: 9fa37f2092a741dafaef3d2434c641caaf5e19d1

### Features
   * Support B91
   * Support freeRTOS
   * Support 4-wire JTAG
   * Add the function to get version information
   * Add a boot that all global variables are in the retention area
   * Support advertising in slave connection
   * Support passive scanning in slave connection
   * Support passive scanning in advertising
   * Adjust the structure of the code related to battery power detection
   * Update b91 to support protection code

### Bug Fixes
   * N/A.

### BREAKING CHANGES
   * N/A.

### CodeSize
   * Flash： 54.4k  Bytes
   * RAM:
     - I-RAM: 26.0k Bytes
     - D-RAM: 0.1k Bytes
   * The values above are obtained by compiling B91_ble_sample with default configuration.
   
   * Flash： 86.47k  Bytes
   * RAM:
     - I-RAM: 48.5k Bytes
     - D-RAM: 0.1k Bytes
   * The values above are obtained by compiling B91_ble_sample with FreeRTOS.    

### Dependency Updates
   *  telink_b91m_driver_src SHA-1: 9fa37f2092a741dafaef3d2434c641caaf5e19d1

### Features
   * 支持B91
   * 支持freeRTOS
   * 支持4线JTAG
   * 增加API获取SDK版本
   * 增加默认全局变量放入retention区域的boot
   * 支持在连接中广播
   * 支持在连接中被动扫描
   * 支持在广播中被动扫描
   * 调整电池电量检测代码的结构
   * 更新b91以支持保护码

### Bug Fixes
   * N/A.

### BREAKING CHANGES
   * N/A.

### CodeSize
   * Flash： 54.4k  Bytes
   * RAM:
     - I-RAM: 26.0k Bytes
     - D-RAM: 0.1k Bytes
   * 以上大小使用默认配置编译B91_ble_sample得到

   * Flash： 86.47k  Bytes
   * RAM:
     - I-RAM: 48.5k Bytes
     - D-RAM: 0.1k Bytes
   * 以上大小使用基于FreeRTOS的B91_ble_sample得到.