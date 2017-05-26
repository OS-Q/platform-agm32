
#ifndef HAL_H
#define HAL_H

//#include "regs.h"
#include "MAP.h"

#define	  SPI_PORT 	  GPIOB
#define	  SPI_CS  	  GPIO_Pin_12
#define	  SPI_SCK  	  GPIO_Pin_13
#define	  SPI_MISO  	GPIO_Pin_14
#define	  SPI_MOSI  	GPIO_Pin_15
#define	  GDO_PORT 	  GPIOA
#define	  GDO0_Pin    GPIO_Pin_11
#define	  GDO2_Pin    GPIO_Pin_8

//SCLK
#define SCLK_LOW()   GPIO_ResetBits(SPI_PORT, SPI_SCK)
#define SCLK_HIGH()  GPIO_SetBits(SPI_PORT, SPI_SCK)
//MISO
#define SDI_IN()       GPIO_ReadInputDataBit(SPI_PORT,SPI_MISO)
#define SDI_LOW_WAIT()   while(1==SDI_IN()) 
#define SDI_HIGH_WAIT()  while(0==SDI_IN())  

#define SDO_LOW()   GPIO_ResetBits(SPI_PORT, SPI_MOSI)
#define SDO_HIGH()  GPIO_SetBits(SPI_PORT, SPI_MOSI)

#define CSn_LOW   GPIO_ResetBits(SPI_PORT, SPI_CS)
#define CSn_HIGH  GPIO_SetBits(SPI_PORT, SPI_CS)


// #define SCLK_LOW()   GPIO_ResetBits(GPIOB, GPIO_Pin_13)
// #define SCLK_HIGH()  GPIO_SetBits(GPIOB, GPIO_Pin_13)

// #define SDI_IN()         GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)
// #define SDI_LOW_WAIT()   while(1==SDI_IN())  //wait untill CC1100's SO to be cleared
// #define SDI_HIGH_WAIT()  while(0==SDI_IN())  //wait untill CC1100's SO to be set


// #define SDO_LOW()   GPIO_ResetBits(GPIOB, GPIO_Pin_15)
// #define SDO_HIGH()  GPIO_SetBits(GPIOB, GPIO_Pin_15)


// #define CSn_LOW()   GPIO_ResetBits(GPIOB, GPIO_Pin_12)
// #define CSn_HIGH()  GPIO_SetBits(GPIOB, GPIO_Pin_12)


// #define GDO0_LOW_WAIT()   while(1==GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11))  //wait untill CC1100's SO to be cleared
// #define GDO0_HIGH_WAIT()  while(0==GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11))  //wait untill CC1100's SO to be set

// //#define GDO2_LOW_WAIT()   while(1==GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5))  //wait untill CC1100's SO to be cleared
// //#define GDO2_HIGH_WAIT()  while(0==GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5))  //wait untill CC1100's SO to be set

// #define GDO2_LOW_WAIT()   GDO0_LOW_WAIT()  //wait untill CC1100's SO to be cleared
// #define GDO2_HIGH_WAIT()  GDO0_HIGH_WAIT()  //wait untill CC1100's SO to be set


void GDO0_HIGH_WAIT();
void GDO0_LOW_WAIT();

void RF_HalInit(void);
u8 halSpiByteRead(void);
u8 halSpiReadStatus(u8 addr);
u8 halSpiReadReg(u8 addr);
void halSpiReadBurstReg(u8 addr, u8 *buffer, u8 count);

void halSpiStrobe(u8 strobe);
void halRfWriteReg(u8 addr, u8 value);
void halSpiWriteBurstReg(u8 addr, u8 *buffer,u8 count);

// //-------------------------------------------------------------------------------------------------------
// // Definitions to support burst/single access:
// #define WRITE_BURST     0x40
// #define READ_SINGLE     0x80
// #define READ_BURST      0xC0

// void halSpiByteWrite(u8 cData);

// u8 halSpiByteRead(void);

// void halSpiStrobe(u8 strobe);

// u8 halSpiReadStatus(u8 addr);

// void halSpiWriteReg(u8 addr, u8 value);

// u8 halSpiReadReg(u8 addr);

// void halSpiWriteBurstReg(u8 addr, u8 *buffer, u8 count);

// void halSpiReadBurstReg(u8 addr, u8 *buffer, u8 count);

// void RESET_CCxxx0(void);

// void POWER_UP_RESET_CCxxx0(void);

// void halRfWriteRfSettings(void);

// void halRfSendPacket(u8 *txBuffer, u8 size);

// //u8 halRfReceivePacket(u8 *rxBuffer, UINT8 *length);

// void halWait(u16 timeout);

// void RFInit(void);
// void RF_RX(void);

#endif
