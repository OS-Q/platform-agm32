
#ifndef HAL_H
#define HAL_H

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

#define GDO0_LOW_WAIT()   while(1==GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11))  //wait untill CC1100's SO to be cleared
#define GDO0_HIGH_WAIT()  while(0==GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11))  //wait untill CC1100's SO to be set

#define GDO2_LOW_WAIT()   GDO0_LOW_WAIT()  //wait untill CC1100's SO to be cleared
#define GDO2_HIGH_WAIT()  GDO0_HIGH_WAIT()  //wait untill CC1100's SO to be set


//-------------------------------------------------------------------------------------------------------
// Definitions to support burst/single access:

void Hal_Init(void);
u8 GDO0_IRQ(void);
u8 GDO2_IRQ(void);
void halSpiByteWrite(u8 cData);
u8 halSpiByteRead(void);


#endif
