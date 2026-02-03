/******************************************************************************/
/* Header Files */
#include "SPI_FLASH.h"

/******************************************************************************/
volatile uint8_t   Flash_Type = 0x00;                                           /* FLASH chip: 0: W25XXXseries */
volatile uint32_t  Flash_ID = 0x00;                                             /* FLASH ID */
volatile uint32_t  Flash_Sector_Count = 0x00;                                   /* FLASH sector number */
volatile uint16_t  Flash_Sector_Size = 0x00;                                    /* FLASH sector size */

#define USING_HSPI 1

/*******************************************************************************
* Function Name  : Software SPI
*******************************************************************************/
void __delay_us(unsigned int us)
{
	delay_us(us);

//	uint32_t i;
//	for(i = 0; i < (2*us); i++)
//	{
//		__asm__("nop");
//	}
}
unsigned char writeSPIByte(unsigned char transmit)
{
  //make the transmission
  unsigned char ret = 0;                    //Initialize read byte with 0

#if !USING_HSPI
  unsigned char mask = 0x80;                //Initialize to write and read bit 7
  PIN_CLEAR(PIN_CLK);
  do
  {
    //Clock out current bit onto SPI Out line
    if(transmit & mask)
    {
    	PIN_SET(PIN_MOSI);
    }
    else
    {
		PIN_CLEAR(PIN_MOSI);
    }
    PIN_SET(PIN_CLK);
    if(PIN_READ(PIN_MISO))
    {
    	ret |= mask;      //Read current bit fromSPI In line
    }
    __delay_us(1);      //Ensure minimum delay of 500nS between SPI Clock high and SPI Clock Low
//    __asm__("nop");
    PIN_CLEAR(PIN_CLK);
    mask = mask >> 1;   //Shift mask so that next bit is written and read from SPI lines
    __delay_us(1);      //Ensure minimum delay of 1000ns between bits
//    __asm__("nop");
  }while (mask != 0);

#else
	spi_tx_dma_dis(HSPI_MODULE);
	spi_rx_dma_dis(HSPI_MODULE);
	spi_tx_fifo_clr(HSPI_MODULE);
	spi_rx_fifo_clr(HSPI_MODULE);
	spi_tx_cnt(HSPI_MODULE, 1);
	spi_rx_cnt(HSPI_MODULE, 1);
	spi_set_transmode(HSPI_MODULE, SPI_MODE_WRITE_AND_READ);
	spi_set_cmd(HSPI_MODULE, 0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
	spi_write(HSPI_MODULE,&transmit,1);
	spi_read(HSPI_MODULE,&ret,1);
	while (spi_is_busy(HSPI_MODULE));
#endif

  return ret;
}

int writeSPIWord(unsigned short int setting)
{
  int data;
  unsigned char b1, b2;
  b1 = writeSPIByte(setting >> 8);
  b2 = writeSPIByte(setting);
  data = b1 << 8 | b2;
  __delay_us(50);
  return data;
}

int readSPIWord(void)
{
  int data;
  unsigned char b1, b2;
  b1 = writeSPIByte(0x00);
  b2 = writeSPIByte(0x00);
  data = b1 << 8 | b2;
  return data;
}
unsigned char readSPIByte(void)
{
  unsigned char data;

  data = writeSPIByte(0x00);
  return data;
}

/*******************************************************************************
* Function Name  : FLASH_Port_Init
* Description    : FLASH chip operation related pins and hardware initialization
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_Port_Init(void)
{
#if !USING_HSPI
	gpio_function_en(PIN_CS);
	gpio_set_output(PIN_CS,1); 		//enable output
	gpio_set_up_down_res(PIN_CS,GPIO_PIN_PULLUP_10K);
	PIN_SET(PIN_CS);

	gpio_function_en(PIN_WP);
	gpio_set_output(PIN_WP,1); 		//enable output
	gpio_set_up_down_res(PIN_WP,GPIO_PIN_PULLUP_10K);
	PIN_SET(PIN_WP);

	gpio_function_en(PIN_MISO);
	gpio_set_output(PIN_MISO,0); 		//enable output
	gpio_set_input(PIN_MISO,1);
	gpio_set_up_down_res(PIN_MISO,GPIO_PIN_PULLUP_10K);

	gpio_function_en(PIN_MOSI);
	gpio_set_output(PIN_MOSI,1); 		//enable output
	gpio_set_up_down_res(PIN_MOSI,GPIO_PIN_PULLUP_10K);
	PIN_CLEAR(PIN_MOSI);

	gpio_function_en(PIN_CLK);
	gpio_set_output(PIN_CLK,1); 		//enable output
	gpio_set_up_down_res(PIN_CLK,GPIO_PIN_PULLUP_10K);
	PIN_CLEAR(PIN_CLK);
#else
	//
	PERI_SPI_PIN_INIT(GPIO_PE4);
	PERI_SPI_PIN_INIT(GPIO_PE5);
	PERI_SPI_PIN_INIT(GPIO_PE6);
	PERI_SPI_PIN_INIT(GPIO_PB1);//
	delay_ms(100);
	//

	hspi_pin_config_t config;

	config.hspi_clk_pin = PIN_CLK;
	config.hspi_mosi_io0_pin = PIN_MOSI;
	config.hspi_miso_io1_pin = PIN_MISO;

    spi_master_init(HSPI_MODULE,4, SPI_MODE3);
    hspi_set_pin(&config);

	gpio_function_en(PIN_CS);
	gpio_set_output(PIN_CS,1); 		//enable output
	gpio_set_up_down_res(PIN_CS,GPIO_PIN_PULLUP_10K);
	PIN_SET(PIN_CS);

	gpio_function_en(PIN_WP);
	gpio_set_output(PIN_WP,1); 		//enable output
	gpio_set_up_down_res(PIN_WP,GPIO_PIN_PULLUP_10K);
	PIN_SET(PIN_WP);
#endif

	printf("FLASH_Port_Init\r\n");
}

/*********************************************************************
 * @fn      SPI1_ReadWriteByte
 *
 * @brief   SPI1 read or write one byte.
 *
 * @param   TxData - write one byte data.
 *
 * @return  Read one byte data.
 */
uint8_t SPI1_ReadWriteByte(uint8_t TxData) {
	return 0;
}

/*******************************************************************************
* Function Name  : SPI_FLASH_SendByte
* Description    : SPI send a byte
* Input          : byte: byte to send
* Output         : None
* Return         : None
*******************************************************************************/
uint8_t SPI_FLASH_SendByte(uint8_t byte)
{
    return writeSPIByte(byte);
}

/*******************************************************************************
* Function Name  : SPI_FLASH_ReadByte
* Description    : SPI receive a byte
* Input          : None
* Output         : None
* Return         : byte received
*******************************************************************************/
uint8_t SPI_FLASH_ReadByte(void)
{
    return readSPIByte();
}

/*******************************************************************************
* Function Name  : FLASH_ReadID
* Description    : Read FLASH ID
* Input          : None
* Output         : None
* Return         : chip id
*******************************************************************************/
uint32_t FLASH_ReadID(void)
{
    uint32_t dat;
    PIN_CLEAR(PIN_CS);
    SPI_FLASH_SendByte(CMD_FLASH_JEDEC_ID);
    dat = (uint32_t)SPI_FLASH_SendByte(DEF_DUMMY_BYTE) << 16;
    dat |= (uint32_t)SPI_FLASH_SendByte(DEF_DUMMY_BYTE) << 8;
    dat |= SPI_FLASH_SendByte(DEF_DUMMY_BYTE);
    PIN_SET(PIN_CS);
    return(dat);
}

/*******************************************************************************
* Function Name  : FLASH_WriteEnable
* Description    : FLASH Write Enable
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_WriteEnable(void)
{
    PIN_CLEAR(PIN_CS);
    SPI_FLASH_SendByte(CMD_FLASH_WREN);
    PIN_SET(PIN_CS);
}

/*******************************************************************************
* Function Name  : FLASH_WriteDisable
* Description    : FLASH Write Disable
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_WriteDisable(void)
{
    PIN_CLEAR(PIN_CS);
    SPI_FLASH_SendByte(CMD_FLASH_WRDI);
    PIN_SET(PIN_CS);
}

/*******************************************************************************
* Function Name  : FLASH_ReadStatusReg
* Description    : FLASH Read Status
* Input          : None
* Output         : None
* Return         : status
*******************************************************************************/
uint8_t FLASH_ReadStatusReg(void)
{
    uint8_t  status;
    PIN_CLEAR(PIN_CS);
    SPI_FLASH_SendByte(CMD_FLASH_RDSR);
    status = SPI_FLASH_ReadByte();
    PIN_SET(PIN_CS);
    return(status);
}

/*******************************************************************************
* Function Name  : FLASH_Erase_Sector
* Description    : FLASH Erase Sector
* Input          : None 
* Output         : None
* Return         : None
*******************************************************************************/ 
void FLASH_Erase_Sector(uint32_t address)
{
    uint8_t  temp;
    FLASH_WriteEnable();
    PIN_CLEAR(PIN_CS);
    SPI_FLASH_SendByte(CMD_FLASH_SECTOR_ERASE);
    SPI_FLASH_SendByte((uint8_t)(address >> 16));
    SPI_FLASH_SendByte((uint8_t)(address >> 8));
    SPI_FLASH_SendByte((uint8_t)address);
    PIN_SET(PIN_CS);
    do
    {
        temp = FLASH_ReadStatusReg();
    }while(temp & 0x01);
}

/*******************************************************************************
* Function Name  : FLASH_RD_Block_Start
* Description    : FLASH start block read
* Input          : None 
* Output         : None
* Return         : None
*******************************************************************************/  
void FLASH_RD_Block_Start(uint32_t address)
{
    PIN_CLEAR(PIN_CS);
    SPI_FLASH_SendByte(CMD_FLASH_READ);
    SPI_FLASH_SendByte((uint8_t)(address >> 16));
    SPI_FLASH_SendByte((uint8_t)(address >> 8));
    SPI_FLASH_SendByte((uint8_t)address);
}

/*******************************************************************************
* Function Name  : FLASH_RD_Block
* Description    : FLASH read block
* Input          : None 
* Output         : None
* Return         : None
*******************************************************************************/  
void FLASH_RD_Block(uint8_t *pbuf, uint32_t len)
{
    while(len--)
    {
        *pbuf++ = SPI_FLASH_ReadByte();
    }
}

/*******************************************************************************
* Function Name  : FLASH_RD_Block_End
* Description    : FLASH end block read
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_RD_Block_End(void)
{
    PIN_SET(PIN_CS);
}

/*******************************************************************************
* Function Name  : W25XXX_Read
* Description    : Flash page program
* Input          : address
*                  len
*                  *pbuf
* Output         : None
* Return         : None
*******************************************************************************/
void W25XXX_Read(uint8_t *pbuf, uint32_t address, uint32_t len)
{
    FLASH_RD_Block_Start(address);
    FLASH_RD_Block((uint8_t*)pbuf , len);
    FLASH_RD_Block_End();
}

/*******************************************************************************
* Function Name  : W25XXX_WR_Page
* Description    : Flash page program
* Input          : address
*                  len
*                  *pbuf
* Output         : None
* Return         : None
*******************************************************************************/
void W25XXX_WR_Page(uint8_t *pbuf, uint32_t address, uint32_t len)
{
    uint8_t  temp;
    FLASH_WriteEnable();
    PIN_CLEAR(PIN_CS);
    SPI_FLASH_SendByte(CMD_FLASH_BYTE_PROG);
    SPI_FLASH_SendByte((uint8_t)(address >> 16));
    SPI_FLASH_SendByte((uint8_t)(address >> 8));
    SPI_FLASH_SendByte((uint8_t)address);
    if(len > SPI_FLASH_PerWritePageSize)
    {
        len = SPI_FLASH_PerWritePageSize;
    }
    while(len--)
    {
        SPI_FLASH_SendByte(*pbuf++);
    }
    PIN_SET(PIN_CS);
    do
    {
        temp = FLASH_ReadStatusReg();
    }while(temp & 0x01);
}

/*******************************************************************************
* Function Name  : W25XXX_WR_Block
* Description    : W25XXX block write 
* Input          : address
*                  len
*                  *pbuf
* Output         : None
* Return         : None
*******************************************************************************/
void W25XXX_WR_Block(uint8_t *pbuf, uint32_t address, uint32_t len)
{
    uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

    Addr = address % SPI_FLASH_PageSize;
    count = SPI_FLASH_PageSize - Addr;
    NumOfPage =  len / SPI_FLASH_PageSize;
    NumOfSingle = len % SPI_FLASH_PageSize;

//    LOGA(DRV,"address: %d\n",address);
    if(Addr == 0)
    {
        if(NumOfPage == 0)
        {
            W25XXX_WR_Page(pbuf, address, len);
        }
        else
        {
            while(NumOfPage--)
            {
                W25XXX_WR_Page(pbuf, address, SPI_FLASH_PageSize);
                address +=  SPI_FLASH_PageSize;
                pbuf += SPI_FLASH_PageSize;
            }
            W25XXX_WR_Page(pbuf, address, NumOfSingle);
        }
    }
    else
    {
        if(NumOfPage == 0)
        {
            if(NumOfSingle > count)
            {
                temp = NumOfSingle - count;
                W25XXX_WR_Page(pbuf, address, count);
                address +=  count;
                pbuf += count;
                W25XXX_WR_Page(pbuf, address, temp);
            }
            else
            {
                W25XXX_WR_Page(pbuf, address, len);
            }
        }
        else
        {
            len -= count;
            NumOfPage =  len / SPI_FLASH_PageSize;
            NumOfSingle = len % SPI_FLASH_PageSize;
            W25XXX_WR_Page(pbuf, address, count);
            address +=  count;
            pbuf += count;
            while(NumOfPage--)
            {
                W25XXX_WR_Page(pbuf, address, SPI_FLASH_PageSize);
                address += SPI_FLASH_PageSize;
                pbuf += SPI_FLASH_PageSize;
            }
            if(NumOfSingle != 0)
            {
                W25XXX_WR_Page(pbuf, address, NumOfSingle);
            }
        }
    }
}



/*******************************************************************************
* Function Name  : FLASH_IC_Check
* Description    : check flash type
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_IC_Check(void)
{
    uint32_t count;

    /* Read FLASH ID */    
    Flash_ID = FLASH_ReadID();
//    P_INFO("Flash_ID: %d\n",(uint32_t)Flash_ID);

    Flash_Type = 0x00;                                                                
    Flash_Sector_Count = 0x00;
    Flash_Sector_Size = 0x00;

    switch(Flash_ID)
    {
        /* W25XXX */
        case W25X10_FLASH_ID:                                                   /* 0xEF3011-----1M bit */
            count = 1;
            break;

        case W25X20_FLASH_ID:                                                   /* 0xEF3012-----2M bit */
            count = 2;
            break;

        case W25X40_FLASH_ID:                                                   /* 0xEF3013-----4M bit */
            count = 4;
            break;

        case W25X80_FLASH_ID:                                                   /* 0xEF4014-----8M bit */
            count = 8;
            break;

        case W25Q16_FLASH_ID1:                                                  /* 0xEF3015-----16M bit */
        case W25Q16_FLASH_ID2:                                                  /* 0xEF4015-----16M bit */
            count = 16;
            break;

        case W25Q32_FLASH_ID1:                                                  /* 0xEF4016-----32M bit */
        case W25Q32_FLASH_ID2:                                                  /* 0xEF6016-----32M bit */
            count = 32;
            break;

        case W25Q64_FLASH_ID1:                                                  /* 0xEF4017-----64M bit */
        case W25Q64_FLASH_ID2:                                                  /* 0xEF6017-----64M bit */
            count = 64;
            break;

        case W25Q128_FLASH_ID1:                                                 /* 0xEF4018-----128M bit */
        case W25Q128_FLASH_ID2:                                                 /* 0xEF6018-----128M bit */
            count = 128;
            break;

        case W25Q256_FLASH_ID1:                                                 /* 0xEF4019-----256M bit */
        case W25Q256_FLASH_ID2:                                                 /* 0xEF6019-----256M bit */
            count = 256;
            break;
        default:
            if((Flash_ID != 0xFFFFFFFF) && (Flash_ID != 0x00000000))
            {
                count = 16;
            }
            else
            {
                count = 0x00;
            }
            break;
    }
    count = ((uint32_t)count * 1024) * ((uint32_t)1024 / 8);

    if(count)
    {
        Flash_Sector_Count = count / DEF_UDISK_SECTOR_SIZE;
        Flash_Sector_Size = DEF_UDISK_SECTOR_SIZE;

//        LOGA(DRV,"Count: %d, %d, %d\n",count,(uint32_t)Flash_Sector_Count,(uint32_t)Flash_Sector_Size);
    }
    else
    {
    	P_INFO("External Flash not connected\r\n");
        while(1);
    }
}

