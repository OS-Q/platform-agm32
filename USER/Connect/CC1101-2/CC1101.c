

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "stm32f10x_spi.h"
#include  "regs.h"
#include  "hal.h"
#include  "CC1101.h"

/*******************************************************************************
* Function Name  : WriteRfSettings
* Description    :
*******************************************************************************/
void WriteRfSettings(void) {
    halSpiWriteReg(TI_CCxxx0_FSCTRL1,  0x06);//FSCTRL1   Frequency synthesizer control.
    halSpiWriteReg(TI_CCxxx0_FREQ2,    0x10);//FREQ2     Frequency control word, high byte.
    halSpiWriteReg(TI_CCxxx0_FREQ1,    0xa7);//FREQ1     Frequency control word, middle byte
    halSpiWriteReg(TI_CCxxx0_FREQ0,    0x62);//FREQ0     Frequency control word, low byte
    halSpiWriteReg(TI_CCxxx0_MDMCFG4,  0xf6);//MDMCFG4   Modem configuration
    halSpiWriteReg(TI_CCxxx0_MDMCFG3,  0x83);//MDMCFG3   Modem configuration.
    halSpiWriteReg(TI_CCxxx0_MDMCFG2,  0x03);//0b, MDMCFG2   Modem configuration.
    halSpiWriteReg(TI_CCxxx0_MDMCFG1,  0xa2);//MDMCFG1   Modem configuration.
    halSpiWriteReg(TI_CCxxx0_MDMCFG0,  0xf8);//MDMCFG0   Modem configuration.               ------default value
    halSpiWriteReg(TI_CCxxx0_CHANNR,   0x00);//CHANNR    Channel number.                      ------default value
    halSpiWriteReg(TI_CCxxx0_DEVIATN,  0x15);//250k   Modem deviation setting (when FSK modulation is enabled).

    halSpiWriteReg(TI_CCxxx0_FREND1,   0x56);//FREND1    Front end RX configuration
    halSpiWriteReg(TI_CCxxx0_FREND0,   0x10);//FREND0    Front end RX configuration         ------default value
    halSpiWriteReg(TI_CCxxx0_MCSM0 ,   0x18);//MCSM0     Main Radio Control State Machine configuration
    halSpiWriteReg(TI_CCxxx0_FOCCFG,   0x16);//FOCCFG    Frequency Offset Compensation Configuration
    halSpiWriteReg(TI_CCxxx0_BSCFG,    0x6c);//BSCFG     Bit synchronization Configuration.   ------default value
    halSpiWriteReg(TI_CCxxx0_AGCCTRL2, 0x03);//AGCCTRL2  AGC control                          ------default value
    halSpiWriteReg(TI_CCxxx0_AGCCTRL1, 0x40);//AGCCTRL1  AGC control                          ------default value
    halSpiWriteReg(TI_CCxxx0_AGCCTRL0, 0x91);//AGCCTRL0  AGC control.                       ------default value
    halSpiWriteReg(TI_CCxxx0_FSCAL3,   0xe9);//FSCAL3    Frequency synthesizer calibration.
    halSpiWriteReg(TI_CCxxx0_FSCAL2,   0x2a);//FSCAL2    Frequency synthesizer calibration. ------default value
    halSpiWriteReg(TI_CCxxx0_FSCAL1,   0x00);//FSCAL1    Frequency synthesizer calibration.
    halSpiWriteReg(TI_CCxxx0_FSCAL0,   0x1f);//FSCAL0    Frequency synthesizer calibration.
    halSpiWriteReg(TI_CCxxx0_FSTEST,   0x59);
    halSpiWriteReg(TI_CCxxx0_TEST2,    0x81);
    halSpiWriteReg(TI_CCxxx0_TEST1,    0x35);
    halSpiWriteReg(TI_CCxxx0_TEST0,    0x09);
    halSpiWriteReg(TI_CCxxx0_IOCFG0,   0x06);//IOCFG2    GDO2 output pin configuration.
		
	halSpiWriteReg(TI_CCxxx0_PKTCTRL1, 0x05);//PKTCTRL1  Packet automation control. 
    halSpiWriteReg(TI_CCxxx0_PKTCTRL0, 0x44);//PKTCTRL0  Packet automation control.
    halSpiWriteReg(TI_CCxxx0_ADDR,     0X12);//ADDR      Device address.
    halSpiWriteReg(TI_CCxxx0_PKTLEN,   30);//PKTLEN    Packet length.
}
/*******************************************************************************
* Function Name  : RFInit
* Description    : 
*******************************************************************************/
void CC1101_Init(void)
{
    unsigned char paTable[]= {0xc0};    //0xc0--  10 dbm
    unsigned paTableLen = 1;
	  Hal_Init();
  
    POWER_UP_RESET_CCxxx0();
    WriteRfSettings();                       // Write RF settings to config reg
    halSpiWriteBurstReg(TI_CCxxx0_PATABLE, paTable, paTableLen);//Write PATABLE
}
/*******************************************************************************
* Function Name  : RF_HalInit
*******************************************************************************/
void Hal_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStructure;
	  EXTI_InitTypeDef EXTI_InitStructure;   
    NVIC_InitTypeDef NVIC_InitStructure; 	
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    // CSn + SCLK + MOSI
    GPIO_InitStructure.GPIO_Pin   = SPI_CS | SPI_SCK | SPI_MISO;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_PORT, &GPIO_InitStructure);
    // MISO
    GPIO_InitStructure.GPIO_Pin   = SPI_MISO;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GDO0_Pin | GDO2_Pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GDO_PORT, &GPIO_InitStructure);		
	
    // GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource11);
    // EXTI_ClearITPendingBit(EXTI_Line11);
    // EXTI_InitStructure.EXTI_Line = EXTI_Line11;    
    // EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    // EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    // EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    // EXTI_Init(&EXTI_InitStructure);  
		
		// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
		// NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;	 
		// NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		// NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	  
		// NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		// NVIC_Init(&NVIC_InitStructure); 	
}


/*******************************************************************************
* Function Name  : RF_RX
* Description    : 
* Input          : None
* Output         : None
* Return         : None
* Attention			 : None
*******************************************************************************/

void RF_RX(void)
{
    halSpiStrobe(TI_CCxxx0_SIDLE);
    halSpiStrobe(TI_CCxxx0_SRX);
    //RFFlag |= RFRxState;
}

/*******************************************************************************
* Function Name  : halSpiByteWrite
* Description    : send 8 bit data to SDO line
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void halSpiByteWrite(u8 cData)
{
    u8 i;
    for(i=0; i<8; i++)
    { 
         SCLK_LOW();  
         if((cData&0x80)==0x80)
         {
             SDO_HIGH();    
         }
         else
         {
             SDO_LOW();  
         }
         cData<<=1;
         halWait(1); halWait(1); halWait(1);
         SCLK_HIGH();    
         halWait(1); halWait(1);
    }
    SCLK_LOW();        
}

/*******************************************************************************
* Function Name  : halSpiByteRead
* Description    : get 8 bit data from SDI line
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
u8 halSpiByteRead(void)
{
    u8 i, result;
    result = 0;
    //SCLK_OUT &= ~SCLK;
    SCLK_LOW();  // SCLK LOW
    halWait(1);
    halWait(1);
    halWait(1);
    for(i=0; i<8; i++)
    {
        //SCLK_OUT |= SCLK;
        SCLK_HIGH();// SCLK HIGH
        result<<=1;
        if(SDI_IN())
        {
            result |= 0x01;     // 数据'1'入
        }
        halWait(1);
        halWait(1);
        //SCLK_OUT &= ~SCLK;
        SCLK_LOW();  // SCLK LOW
        halWait(1); halWait(1);
    }
    return result;
}
/*******************************************************************************
* Function Name  : RESET_CCxxx0
* Description    : reset the CCxxx0 and wait for it to be ready
* Input          : None
* Output         : None
* Return         : None
* Attention			 : None
*******************************************************************************/
void RESET_CCxxx0()
{
    CSn_LOW;
    //SDI_LOW_WAIT();
    halSpiByteWrite(TI_CCxxx0_SRES);
    //SDI_LOW_WAIT();
    CSn_HIGH;
}
/*******************************************************************************
* Function Name  : halSpiWriteReg
* Description    : 
* Input          : None
* Output         : None
* Return         : None
* Attention			 : None
*******************************************************************************/
void POWER_UP_RESET_CCxxx0()
{
    CSn_HIGH;
    halWait(1);
    CSn_LOW;
    halWait(1);
    CSn_HIGH;
    halWait(41);
    RESET_CCxxx0();
}

/*******************************************************************************
* Function Name  : halSpiWriteBurstReg
* Description    : 
* Input          : None
* Output         : None
* Return         : None
* Attention			 : None
*******************************************************************************/
void halSpiWriteBurstReg(u8 addr, u8 *buffer, u8 count)
{
    u8 i, SPI_DATA;

    CSn_LOW;
    //SDI_LOW_WAIT();

    SPI_DATA = addr | WRITE_BURST;
    halSpiByteWrite(SPI_DATA);

    for (i = 0; i < count; i++)
    {
        SPI_DATA = buffer[i];
        halSpiByteWrite(SPI_DATA);
    }

    CSn_HIGH;
}// halSpiWriteBurstReg
/*******************************************************************************
* Function Name  : halSpiWriteReg
* Description    : 
* Input          : None
* Output         : None
* Return         : None
* Attention			 : None
*******************************************************************************/
void halSpiWriteReg(u8 addr, u8 value)
{
    u8 SPI_DATA;
	
    CSn_LOW;
    //SDI_LOW_WAIT();

    SPI_DATA = addr;
    halSpiByteWrite(SPI_DATA);

    SPI_DATA = value;
    halSpiByteWrite(SPI_DATA);

    CSn_HIGH;
}// halSpiWriteReg

/*******************************************************************************
* Function Name  : halSpiWriteReg
* Description    : 
* Input          : None
* Output         : None
* Return         : None
* Attention			 : None
*******************************************************************************/

void halSpiStrobe(u8 strobe)
{
    u8 SPI_DATA;
    CSn_LOW;
    //SDI_LOW_WAIT();

    SPI_DATA = strobe;
    halSpiByteWrite(SPI_DATA);

    CSn_HIGH;
}// halSpiStrobe

/*******************************************************************************
* Function Name  : halSpiWriteReg
* Description    : 
* Input          : None
* Output         : None
* Return         : None
* Attention			 : None
*******************************************************************************/
u8 halSpiReadStatus(u8 addr)
{
    u8 SPI_DATA;

    CSn_LOW;
    //SDI_LOW_WAIT();
    SPI_DATA = (addr | READ_BURST);
    halSpiByteWrite(SPI_DATA);
    SPI_DATA = halSpiByteRead();
    CSn_HIGH;
    return SPI_DATA;
}// halSpiReadStatus

/*******************************************************************************
* Function Name  : halSpiReadReg
* Description    : This function gets the value of a single specified CCxxx0 register.
* Input          : Address of the CCxxx0 register to be accessed.
* Output         : None
* Return         : Value of the accessed CCxxx0 register
* Attention			 : None
*******************************************************************************/
u8 halSpiReadReg(u8 addr)
{
    u8 SPI_DATA;

    CSn_LOW;
    //SDI_LOW_WAIT();

    SPI_DATA = (addr | READ_SINGLE);
    halSpiByteWrite(SPI_DATA);

    SPI_DATA = halSpiByteRead();

    CSn_HIGH;
    return SPI_DATA;
}
/*******************************************************************************
* Function Name  : halSpiReadBurstReg
* Description    : This function reads multiple CCxxx0 register, using SPI burst access.
* Input          : Address of the first CCxxx0 register to be accessed.
* Output         : None
* Return         : Value of the accessed CCxxx0 register
* Attention			 : None
*******************************************************************************/
void halSpiReadBurstReg(u8 addr, u8 *buffer, u8 count)
{
    u8 i, SPI_DATA;

    CSn_LOW;

    //SDI_LOW_WAIT();
    SPI_DATA = (addr | READ_BURST);
    halSpiByteWrite(SPI_DATA);

    for (i = 0; i < count; i++)
    {
        SPI_DATA = halSpiByteRead();

        buffer[i] = SPI_DATA;
    }

    CSn_HIGH;
}

//-------------------------------------------------------------------------------------------------------
//  void halWait(UINT16 timeout)
//
//  DESCRIPTION:
//      Runs an idle loop for [timeout] microseconds.
//
//  ARGUMENTS:
//      UINT8 timeout
//          The timeout in microseconds
//-------------------------------------------------------------------------------------------------------
void halWait(u16 timeout)
{
    int k = 0;
    do {
        for ( k = 0; k < 10; k++) ;
    } while (--timeout);
}// halWait

void halWaitMS(int nCount)	 //?òμ￥μ??óê±oˉêy
{
	int i,j;
  for(j=0;j<nCount;j++)
  {
     for(i=0;i<20000;i++);
  }
} 
/*******************************************************************************
* Function Name  : halRfReceivePacket
* Description    :
* Input          :
* Output         : None
* Return         : 
* Attention			 : None
*******************************************************************************/
u8  RfReceivePacket(u8 *rxBuffer, u8 length)
//u8  halRfReceivePacket(u8 *rxBuffer, u8 *length) 
{
    u8 status[2];
    u8 packetLength;

    // TODO: 判断接收信号输出电平
    GDO0_HIGH_WAIT();
    GDO0_LOW_WAIT();
    if ((halSpiReadStatus(TI_CCxxx0_RXBYTES) & TI_CCxxx0_NUM_RXBYTES))
    {
        // Read length byte
//        packetLength = TI_CC_SPIReadReg(TI_CCxxx0_RXFIFO);
      packetLength = length;

        // Read data from RX FIFO and store in rxBuffer
        if (packetLength <=62)
        {
//            TI_CC_SPIReadBurstReg(TI_CCxxx0_RXFIFO, rxBuffer, packetLength);
          halSpiReadBurstReg(TI_CCxxx0_RXFIFO, rxBuffer, packetLength);
//            *length = packetLength;

            // Read the 2 appended status bytes (status[0] = RSSI, status[1] = LQI)
//            TI_CC_SPIReadBurstReg(TI_CCxxx0_RXFIFO, status, 2);
          halSpiReadBurstReg(TI_CCxxx0_RXFIFO, status, 4);

            // Flush RX FIFO
//            TI_CC_SPIStrobe(TI_CCxxx0_SFRX);
          halSpiStrobe(TI_CCxxx0_SFRX);

            // MSB of LQI is the CRC_OK bit
//            return (status[LQI] & CRC_OK);

            return 1;
        }
        else
        {
            //*length = packetLength;

//          *length = 0; //err process

            // Flush RX FIFO
//            TI_CC_SPIStrobe(TI_CCxxx0_SFRX);
            halSpiStrobe(TI_CCxxx0_SFRX);
            RF_RX();
            return FALSE;
        }
    }
    else
        return FALSE;
}// halRfReceivePacket

/*******************************************************************************
* Function Name  : halRfSendPacket
* Description    :
* Input          : txBuffer  size
* Output         : None
* Return         : 
* Attention			 : None
*******************************************************************************/
void halRfSendPacket(u8 *txBuffer, u8 size)
{
    //_DINT();
    //P1IE  &= ~(BIT3);
    halSpiStrobe(TI_CCxxx0_SIDLE); //for long time transmit reliable, demo program no this
    halWait(400);
    halSpiWriteBurstReg(TI_CCxxx0_TXFIFO, txBuffer, size);
    halSpiStrobe(TI_CCxxx0_STX);
    // Wait for GDO2 to be set -> sync transmitted
    GDO2_HIGH_WAIT();

    // Wait for GDO2 to be cleared -> end of packet
    GDO2_LOW_WAIT();

    halWait(100);
   // uart_len_temp = Buffer[0] + 10;
   // while(uart_len_temp>0);
    //P1IFG = 0;
    //P1IE  |= BIT3;
    //_EINT();
}




