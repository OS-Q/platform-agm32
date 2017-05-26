
#ifndef CC1101_H
#define CC1101_H

#include "map.h"


#define  RF_MODE_FIFO        0
#define  RF_MODE_HEAR     0X01
#define  RF_MODE_SYNC     0X06
#define  RF_MODE_CRC    	0X07
#define  RF_MODE_SERA     0X0B

#define  RF_RX_Flag       0X11
#define  RF_TX_Flag       0X15
#define  RF_IDLE_Flag     0X17
#define  RF_TX_Finish     0x19

extern u8 TxBuf[61];
extern u8 RxBuf[61]; 
extern u8 Status[8];
extern u8  CC1101_Mode;

void RF_RX(void);
void RF_TX(void);
void RF_END(void);
void RF_IDLE(void);
void POWER_RESET(void);
void CC1101_Init(void);
void WriteRfSettings(void) ;

void RF_SetPWR(u8 cofig);
void RF_SetADR(u8 *addr);
void RF_SetPKT(u8 mode,u16 leng);
void RF_SetGDO(u8 fifo);
void ADC_GDO(void);
u16 RF_GetTemp(void);
u8 GetRSSI(u8 rssi_dec);

void halRfStrobe(u8 strobe);
void halRfWriteReg(u8 addr, u8 value);
u8 halRfReadStatus(u8 addr);
void halRfReadBurstReg(u8 addr, u8 *buffer, u8 count);
void halRfSendPacket(u8 *txBuffer, u8 size);
u8 RfReceivePacket(u8 *rxBuffer, u8 length) ;
u8 halRfReceivePacket(u8 *rxBuffer,u8 *status,u8 length);

void halSpiStrobe(u8 strobe);
void halSpiWriteReg(u8 addr, u8 value);
u8 halSpiReadStatus(u8 addr);
u8 halSpiReadReg(u8 addr);
void halSpiWriteBurstReg(u8 addr, u8 *buffer, u8 count);
void halSpiReadBurstReg(u8 addr, u8 *buffer, u8 count);
void halSpiSendPacket(u8 *txBuffer, u8 size);




#endif
