
#ifndef SPI_H
#define SPI_H

#include "MAP.h"


#define	  SPI_PORT 	  GPIOB
#define	  SPI_CS  	  GPIO_Pin_12
#define	  SPI_SCK  	  GPIO_Pin_13
#define	  SPI_MISO  	GPIO_Pin_14
#define	  SPI_MOSI  	GPIO_Pin_15

#define	  GDO0_PORT 	   GPIOB
#define	  GDO0_Pin    GPIO_Pin_10
#define	  GDO1_PORT 	 SPI_PORT
#define	  GDO1_Pin       SPI_MISO
#define	  GDO2_PORT 	  GPIOB
#define	  GDO2_Pin    GPIO_Pin_11



#define CSn_LOW   GPIO_ResetBits(SPI_PORT, SPI_CS)
#define CSn_HIGH  GPIO_SetBits(SPI_PORT, SPI_CS)

#define GDO0_LOW_WAIT()   while(1==GPIO_ReadInputDataBit(GDO0_PORT,GDO0_Pin))  
#define GDO0_HIGH_WAIT()  while(0==GPIO_ReadInputDataBit(GDO0_PORT,GDO0_Pin)) 

#define GDO2_LOW_WAIT()   while(1==GPIO_ReadInputDataBit(GDO2_PORT,GDO2_Pin))  
#define GDO2_HIGH_WAIT()  while(0==GPIO_ReadInputDataBit(GDO2_PORT,GDO2_Pin))  


//-------------------------------------------------------------------------------------------------------
// Definitions to support burst/single access:

void halSpi_Init(void);
void halSpiByteWrite(u8 cData);
u8 halSpiByteRead(void);

void halRf_Init(void);
void halRfByteWrite(u8 cData);
u8 halRfByteRead(void);

extern u8  GDO_Status;
u8 GDO0_IRQ(void);
u8 GDO1_IRQ(void); 
u8 GDO2_IRQ(void);



#endif
