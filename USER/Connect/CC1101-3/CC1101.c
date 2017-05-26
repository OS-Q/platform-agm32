
/*******************************************************************************
            YuenJee wirless sensor for CC1101
*******************************************************************************/

#include <stdio.h>
#include "stm32f10x_spi.h"
#include  "regs.h"
#include  "hal.h"
#include  "CC1101.h"
#define  PKT_LEN   40
/*******************************************************************************
* Function Name  : WriteRfSettings
* Description    :
*******************************************************************************/
void WriteRfSettings(void) {
    halRfWriteReg(FSCTRL1,  0x06);//FSCTRL1   Frequency synthesizer control.
    halRfWriteReg(FREQ2,    0x10);//FREQ2     Frequency control word, high byte.
    halRfWriteReg(FREQ1,    0xa7);//FREQ1     Frequency control word, middle byte
    halRfWriteReg(FREQ0,    0x62);//FREQ0     Frequency control word, low byte
    halRfWriteReg(MDMCFG4,  0xf6);//MDMCFG4   Modem configuration
    halRfWriteReg(MDMCFG3,  0x83);//MDMCFG3   Modem configuration.
    halRfWriteReg(MDMCFG2,  0x03);//0b, MDMCFG2   Modem configuration.
    halRfWriteReg(MDMCFG1,  0xa2);//MDMCFG1   Modem configuration.
    halRfWriteReg(MDMCFG0,  0xf8);//MDMCFG0   Modem configuration.               ------default value
    halRfWriteReg(CHANNR,   0x00);//CHANNR    Channel number.                      ------default value
    halRfWriteReg(DEVIATN,  0x15);//250k   Modem deviation setting (when FSK modulation is enabled).

    halRfWriteReg(FREND1,   0x56);//FREND1    Front end RX configuration
    halRfWriteReg(FREND0,   0x10);//FREND0    Front end RX configuration         ------default value
    halRfWriteReg(MCSM0 ,   0x18);//MCSM0     Main Radio Control State Machine configuration
    halRfWriteReg(FOCCFG,   0x16);//FOCCFG    Frequency Offset Compensation Configuration
    halRfWriteReg(BSCFG,    0x6c);//BSCFG     Bit synchronization Configuration.   ------default value
    halRfWriteReg(AGCCTRL2, 0x03);//AGCCTRL2  AGC control                          ------default value
    halRfWriteReg(AGCCTRL1, 0x40);//AGCCTRL1  AGC control                          ------default value
    halRfWriteReg(AGCCTRL0, 0x91);//AGCCTRL0  AGC control.                       ------default value
    halRfWriteReg(FSCAL3,   0xe9);//FSCAL3    Frequency synthesizer calibration.
    halRfWriteReg(FSCAL2,   0x2a);//FSCAL2    Frequency synthesizer calibration. ------default value
    halRfWriteReg(FSCAL1,   0x00);//FSCAL1    Frequency synthesizer calibration.
    halRfWriteReg(FSCAL0,   0x1f);//FSCAL0    Frequency synthesizer calibration.
    halRfWriteReg(FSTEST,   0x59);
		
    halRfWriteReg(TEST2,    0x81);
    halRfWriteReg(TEST1,    0x35);
    halRfWriteReg(TEST0,    0x09);
    halRfWriteReg(IOCFG0,   0x06);//IOCFG2    GDO2 output pin configuration.
		halRfWriteReg(IOCFG2,   0x06);
	  halRfWriteReg(PKTCTRL1, 0x05);//PKTCTRL1  Packet automation control. 
    halRfWriteReg(PKTCTRL0, 0x44);//PKTCTRL0  Packet automation control.
   // halRfWriteReg(ADDR,     0X12);//ADDR      Device address.
   // halRfWriteReg(PKTLEN,   29);//PKTLEN    Packet length.
}
/*******************************************************************************
* Function Name  : RFInit
* Description    : 
*******************************************************************************/
void CC1101_Init(void)
{
	  u8 addr[3]={0x12,0,0};
    Hal_Init(); 
    POWER_RESET();
    WriteRfSettings(); 
	  RF_SetPWR(0x18);	
	  RF_SetADR(addr);
}
/*******************************************************************************
* Function Name  : RF_SetPA
*******************************************************************************/
void RF_SetPWR(u8 cofig)
{
		u8 LS,HS;
		u8 paTable[10]= {0x03,0x17,0x1D,0x26,0x37,0x50,0x86,0xCD,0xC5,0xC0}; 
		HS=cofig>>4;
		LS=(cofig & 0x0F);
			if(HS<1) HS=1;
		halSpiWriteBurstReg(PATABLE, paTable+LS, HS);   
		if(HS>1)  halRfWriteReg(FREND0, 0x17);
}
/*******************************************************************************
* Function Name  : RF_SetAddr
*******************************************************************************/
void RF_SetADR(u8 *addr)
{ 
   if(*addr!=0)
	 {
 		  halRfWriteReg(ADDR, *addr); 
		  halRfWriteReg(PKTCTRL1, 0x0D);
	 }
   else  halRfWriteReg(PKTCTRL1, 0x0C);  	
   if(*(addr+1)!=0) halRfWriteReg(SYNC0, *(addr+1));  	
   if(*(addr+2)!=0) halRfWriteReg(SYNC1, *(addr+2));  		
}

/*******************************************************************************
* Function Name  : RF_RX
* Description    : 
*******************************************************************************/

void RF_RX(void)
{
    halSpiStrobe(SIDLE);
    halSpiStrobe(SRX);
}

/*******************************************************************************
* Function Name  : halRfWriteReg
* Description    : 
*******************************************************************************/
void POWER_RESET(void)
{
    CSn_HIGH;
    Delay_us(3);
    CSn_LOW;
    Delay_us(3);
    CSn_HIGH;
    Delay_us(45);
    CSn_LOW;
    halSpiStrobe(SRES);
    CSn_HIGH;
}
/*******************************************************************************
* Function Name  : halSpiWriteBurstReg
* Description    : 
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
}
/*******************************************************************************
* Function Name  : halRfWriteReg
* Description    : 
*******************************************************************************/
void halRfWriteReg(u8 addr, u8 value)
{
    u8 SPI_DATA;
	
    CSn_LOW;
    //SDI_LOW_WAIT();
    SPI_DATA = addr;
    halSpiByteWrite(SPI_DATA);
    SPI_DATA = value;
    halSpiByteWrite(SPI_DATA);

    CSn_HIGH;
}// halRfWriteReg

/*******************************************************************************
* Function Name  : halRfWriteReg
* Description    : 
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
* Function Name  : halRfWriteReg
* Description    : 
*******************************************************************************/
u8 halSpiReadStatus(u8 addr)
{
    u8 SPI_DATA;

    CSn_LOW;
    SPI_DATA = (addr | READ_BURST);
    halSpiByteWrite(SPI_DATA);
    SPI_DATA = halSpiByteRead();
    CSn_HIGH;
    return SPI_DATA;
}

/*******************************************************************************
* Function Name  : halSpiReadReg
* Description    : This function gets the value of a single specified CCxxx0 register.
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

/*******************************************************************************
* Function Name  : halRfSendPacket
* Description    :
*******************************************************************************/
void halRfSendPacket(u8 *txBuffer, u8 size)
{

    halSpiStrobe(SIDLE); 
    halWait(400);
    halSpiWriteBurstReg(TXFIFO, txBuffer, size);
    halSpiStrobe(STX);
    GDO2_HIGH_WAIT();

    // Wait for GDO2 to be cleared -> end of packet
    GDO2_LOW_WAIT();
    halWait(100);
}
/*******************************************************************************
* Function Name  : halRfReceivePacket
* Description    :
*******************************************************************************/
u8  RfReceivePacket(u8 *rxBuffer, u8 length)
{
    u8 status[2];
    u8 packetLength;
    GDO0_HIGH_WAIT();
    GDO0_LOW_WAIT();
    if ((halSpiReadStatus(RXBYTES) & NUM_RXBYTES))
    {
        if (length <=PKT_LEN)
        {
          halSpiReadBurstReg(RXFIFO, rxBuffer, PKT_LEN);
          halSpiReadBurstReg(RXFIFO, status, 4);
          halSpiStrobe(SFRX);
            return 1;
        }
        else
        {
            halSpiStrobe(SFRX);
            RF_RX();
            return 0;
        }
    }
    else
        return FALSE;
}


/*******************************************************************************
* Function Name  : halRfReceivePacket
*******************************************************************************/
u8 halRfReceivePacket(u8 *rxBuffer,u8 *status,u8 length) 
{
    GDO0_HIGH_WAIT();
    GDO0_LOW_WAIT();
    if((halSpiReadStatus(RXBYTES) & NUM_RXBYTES))
    {
			  if (length <=62)
        {
						halSpiReadBurstReg(RXFIFO, rxBuffer, PKT_LEN);
						halSpiReadBurstReg(RXFIFO, status, 2);
						halSpiStrobe(SFRX);
						return 1;
        }
        else
        {
            halSpiStrobe(SFRX);
            RF_RX();
            return 0;
        }
    }
    else  return FALSE;
}


/*******************************************************************************
* Function Name  : halRfSendPacket
*******************************************************************************/
int GetRSSI(void)
{
     int rssi_dec; 
		 int rssi_dBm; 
		 u8 rssi_offset = 74; 
		 rssi_dec = halSpiReadStatus(RSSI); 
		if (rssi_dec >= 128) rssi_dBm = (long)((long)( rssi_dec - 256) / 2) - rssi_offset; 
		else rssi_dBm = (rssi_dec / 2) - rssi_offset;	
		return rssi_dBm;		  
}
/*******************************************************************************
* Function Name  : halRfReceivePacket
*******************************************************************************/
u8 GetPacket(void) 
{	
    if((halSpiReadStatus(RXBYTES) & NUM_RXBYTES))
    {
          halSpiReadBurstReg(RXFIFO, RxBuf, PKT_LEN);		  	       
	        return BACK_SUCCESS;
    }
    return BACK_ERROR;
}

/*******************************************************************************

 ******************************************************************************/
void EXTI15_10_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line11) != RESET ) 
	{
		  EXTI_ClearITPendingBit(EXTI_Line11);
		  while(SDI_IN());
			//GetPacket();
			//RF_RX();
	   //	GPIOC->ODR^=0X2000;
	}	
}
//void EXTI9_5_IRQHandler(void)
//{
//	u8 cnt,mark=1;
//	if (EXTI_GetITStatus(EXTI_Line8) != RESET ) 
//	{
//		  EXTI_ClearITPendingBit(EXTI_Line8);
//			GetPacket();
//	}	
//}
