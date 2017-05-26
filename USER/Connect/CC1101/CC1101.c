
/*******************************************************************************
                         Qitas CC1101 for STM32
*******************************************************************************/
#include <stdio.h>
#include  "regs.h"
#include  "CC1101.h"
#include  "hal.h"

#define  PKT_LEN   60
#define  RX_FLAG  (halSpiReadStatus(RXBYTES)& NUM_RXBYTES)

u8  Status[8];
u8  CC1101_Mode;
u32 RF_CNT[4];
u8  RF_RX_MODE = 0; 
u8  RF_RX_STEP = 0; 
u8  RF_TX_STAT = 0; 
/*******************************************************************************
* Function Name  : RFInit
* Description    : 
*******************************************************************************/
void CC1101_Init(void)
{
	  u8 addr[3]={1,0X34,0X56};
    //halSpi_Init(); 
		halRf_Init(); 
		
    POWER_RESET();
    WriteRfSettings(); 
	  RF_SetPWR(0x18);	
	  RF_SetADR(addr);
		RF_RX();
		Delay_ms(1);
		GDO0_IRQ();
		GDO1_IRQ();
		GDO2_IRQ();
}
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
	
	  halRfWriteReg(MCSM1 ,   0x3F);//MCSM0     Main Radio Control State Machine configuration
    halRfWriteReg(MCSM0 ,   0x18);//MCSM0     Main Radio Control State Machine configuration
	  halRfWriteReg(FIFOTHR,  0x47); 
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
		
    halRfWriteReg(IOCFG0,   0x0E);
		halRfWriteReg(IOCFG2,   0x0E);	
	  halRfWriteReg(PKTCTRL1, 0x05);
    halRfWriteReg(PKTCTRL0, 0x44);
    //halRfWriteReg(PKTLEN,   PKT_LEN);//PKTLEN    Packet length.
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
* Description    : 
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
	   halSpiStrobe(SFRX);
		 halSpiStrobe(SIDLE);	 
		 Delay_ms(1);  
		 halSpiStrobe(SRX);
	   CC1101_Mode=RF_RX_Flag;
}
/*******************************************************************************
* Function Name  : RF_RX
* Description    : 
*******************************************************************************/

void RF_IDLE(void)
{
//		 halSpiStrobe(SFRX);
//	   halSpiStrobe(SFTX);
		 halSpiStrobe(SIDLE);
     CC1101_Mode=RF_IDLE_Flag;
}
/*******************************************************************************
* Function Name  : RF_RX
* Description    : 
*******************************************************************************/

u8 RF_GET_STAT(void)
{
   return  halSpiReadStatus(MARCSTATE);
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
    SPI_DATA = addr;
    halSpiByteWrite(SPI_DATA);
    SPI_DATA = value;
    halSpiByteWrite(SPI_DATA);
    CSn_HIGH;
}

/*******************************************************************************
* Function Name  : halRfWriteReg
* Description    : 
*******************************************************************************/

void halSpiStrobe(u8 strobe)
{
    u8 SPI_DATA;
    CSn_LOW;
    SPI_DATA = strobe;
    halSpiByteWrite(SPI_DATA);
    CSn_HIGH;
}

/*******************************************************************************
* Function Name  : halSpiReadStatus
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
u8 halRfReadStatus(u8 addr)
{
    u8 SPI_DATA;
    CSn_LOW;
    SPI_DATA = (addr | READ_BURST);
    halRfByteWrite(SPI_DATA);
    SPI_DATA = halSpiByteRead();
    CSn_HIGH;
    return SPI_DATA;
}

/*******************************************************************************
* Function Name  : halSpiReadBurstReg
* Description    : This function reads multiple CCxxx0 register, using SPI burst access.
*******************************************************************************/
void halRfReadBurstReg(u8 addr, u8 *buffer, u8 count)
{
    u8 i, SPI_DATA;
    CSn_LOW;
    SPI_DATA = (addr | READ_BURST);
    halSpiByteWrite(SPI_DATA);
    for (i = 0; i < count; i++)
    {
        SPI_DATA = halSpiByteRead();

        buffer[i] = SPI_DATA;
    }
    CSn_HIGH;
}
void halSpiReadBurstReg(u8 addr, u8 *buffer, u8 count)
{
    u8 i, SPI_DATA;
    CSn_LOW;
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
* Description    :
*******************************************************************************/
void halRfSendPacket(u8 *txBuffer, u8 size)
{
    halSpiStrobe(SIDLE); 
    Delay_ms(1);
    halSpiWriteBurstReg(TXFIFO, txBuffer, size+2);
    halSpiStrobe(STX);
	  halRfWriteReg(IOCFG1,  0x05);  
	  CC1101_Mode=RF_TX_Flag;	  
}
/*******************************************************************************
* Function Name  : halRfSendPacket
*******************************************************************************/
u8 GetRSSI(u8 rssi_dec)
{
		 u8  rssi_dBm; 
		 u8  rssi_offset = 74;  
		 if (rssi_dec >= 128) rssi_dBm =rssi_offset-(256-rssi_dec)/2; 
		 else rssi_dBm = rssi_offset-(rssi_dec/2);	
		 return rssi_dBm;		  
}


/*******************************************************************************
* Function Name  : RF_RX
*******************************************************************************/
void RF_END(void)
{
   //CSn_LOW;
   halSpiStrobe(SIDLE);
   Delay_us(100);
   halSpiStrobe(SXOFF);
   Delay_us(900);
   //CSn_HIGH;
}//
/*******************************************************************************
* Function Name  : 
*******************************************************************************/
u8 RX_GDO(u8 Chan) 
{	  
		Status[RF_RX_STEP]=halSpiReadStatus(RSSI);	  
	  if(Status[RF_RX_STEP]>70)  halRfWriteReg(FIFOTHR, 0x37); 
		switch(RF_RX_STEP)
		{
					case 0:
					{
							if(Chan==0)
								halRfWriteReg(IOCFG0,  0x06);
							else if(Chan==1)
								halRfWriteReg(IOCFG1,  0x06);
							else if(Chan==2)
								halRfWriteReg(IOCFG2,  0x06);													
							RF_RX_STEP=1;
					}
					break;	
					case 1:	
					{									
							if (RF_RX_MODE==RF_MODE_HEAR)
							{
									halRfWriteReg(IOCFG0,  0x01);
									RF_RX_STEP=2;	
							}
							else if(RF_RX_MODE==RF_MODE_FIFO)
							{
								  halRfWriteReg(IOCFG0,  0);
									RF_RX_STEP=2;	
							}
							else if(RF_RX_MODE==RF_MODE_CRC)
							{
								  halRfWriteReg(IOCFG0,  0x07);
									RF_RX_STEP=2;	
							}
							else if (RF_RX_MODE==RF_MODE_SERA)
							{
									RF_RX_STEP=4;	
							}
					}							
					break;
					case 2:  
				  {			
								halSpiReadBurstReg(RXFIFO,RxBuf,PKT_LEN);						  
								halSpiReadBurstReg(RXFIFO, Status+3, 2);	          						
								RF_RX_STEP=0;					
					}
					break;	
	 }
	 RF_RX();
}

/*******************************************************************************

 ******************************************************************************/
void EXTI15_10_IRQHandler(void)
{
		RF_RX_MODE =RF_MODE_FIFO;// RF_MODE_CRC; 
		if (EXTI_GetITStatus(EXTI_Line10) != RESET ) 
		{
				 EXTI_ClearITPendingBit(EXTI_Line10);
				 if(CC1101_Mode==0)	CC1101_Mode=RF_TX_Finish;
				 else if(CC1101_Mode==RF_RX_Flag)  RX_GDO(0) ;				
		}	
		else if(EXTI_GetITStatus(EXTI_Line11) != RESET ) 
		{			
					EXTI_ClearITPendingBit(EXTI_Line11);
					if(CC1101_Mode==RF_RX_Flag) RX_GDO(2);
		}	
		else if(EXTI_GetITStatus(EXTI_Line14) != RESET ) 
		{		
					EXTI_ClearITPendingBit(EXTI_Line14);				
					if(CC1101_Mode==RF_TX_Flag)
					CC1101_Mode=RF_TX_Finish;
//					else if(CC1101_Mode==RF_RX_Flag)
//						RX_GDO(1);
		}	
}
