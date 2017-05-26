
#ifndef CC1101_H
#define CC1101_H

#include "map.h"


extern uint8_t TxBuf[61];
extern uint8_t RxBuf[61]; 



void RF_RX(void);
void RF_TX(void);
void RF_END(void);
u8 RF_CHCK(void);

void CC1101_Init(void);
void RF_HalInit(void);

void RF_SetPWR(u8 cofig);
void RF_SetADR(u8 addr);
void RF_SetPKT(u8 mode,u16 leng);
void RF_SetGDO(u8 fifo);

void WriteRfSettings(void) ;


void  RESET_CC1101(void);
void RF_GetTemp(void);
int GetRSSI(void);
void halRfSendPacket(u8*txBuffer, u8 size);
u8 RfReceivePacket(u8 *rxBuffer, u8 length) ;
u8 halRfReceivePacket(u8 *rxBuffer,u8 *status,u8 length) ;


#endif
