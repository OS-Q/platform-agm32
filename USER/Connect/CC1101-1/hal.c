

/*******************************************************************************
* Name  :  Qitas soft for CC1101
*******************************************************************************/
#include "stm32f10x_spi.h"
#include <stdio.h>
#include  "regs.h"
#include  "hal.h"

/*******************************************************************************
* Function Name  : time system
*******************************************************************************/
void GDO0_LOW_WAIT()
{
	u16 i=60000;
	while(i--)
	{ 
		if(1==GPIO_ReadInputDataBit(GDO_PORT,GDO0_Pin)) break; 
	}
}

void GDO0_HIGH_WAIT()
{
	u16 i=60000;
	while(i--)
	{ 
		if(0==GPIO_ReadInputDataBit(GDO_PORT,GDO0_Pin)) break;
	}
}

#define GDO2_LOW_WAIT()   GDO0_LOW_WAIT() 
#define GDO2_HIGH_WAIT()  GDO0_HIGH_WAIT()  

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
* Function Name  : RF_HalInit
*******************************************************************************/
void RF_HalInit(void)
{

    GPIO_InitTypeDef GPIO_InitStructure;
	  EXTI_InitTypeDef EXTI_InitStructure;   
    NVIC_InitTypeDef NVIC_InitStructure; 	
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    // CSn + SCLK + MOSI
    GPIO_InitStructure.GPIO_Pin   = SPI_CS | SPI_SCK | SPI_MOSI;
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
* Function Name  : halSpiByteWrite
* Description    : send 8 bit data to SDO line
*******************************************************************************/
// void halSpiByteWrite(u8 cData)
// {
    // u8 i;
    // for(i=0; i<8; i++)
    // { 
         // SCLK_LOW();  
         // if((cData&0x80)==0x80)
         // {
             // SDO_HIGH();    
         // }
         // else
         // {
             // SDO_LOW();  
         // }
         // cData<<=1;
         // Delay_us(1); Delay_us(1); Delay_us(1);
         // SCLK_HIGH();    
         // Delay_us(1); Delay_us(1);
    // }
    // SCLK_LOW();   
// }
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
*******************************************************************************/
// u8 halSpiByteRead(void)
// {
    // u8 i, result;
    // result = 0;
    // SCLK_LOW();  // SCLK LOW
    // Delay_us(1);
    // Delay_us(1);
    // Delay_us(1);
    // for(i=0; i<8; i++)
    // {
        // //SCLK_OUT |= SCLK;
        // SCLK_HIGH();// SCLK HIGH
        // result<<=1;
        // if(SDI_IN())
        // {
            // result |= 0x01;     // 数据'1'入
        // }
        // Delay_us(1);
        // Delay_us(1);
        // //SCLK_OUT &= ~SCLK;
        // SCLK_LOW();  // SCLK LOW
        // Delay_us(1); Delay_us(1);
    // }
    // return result;
// }
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

*******************************************************************************/
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
