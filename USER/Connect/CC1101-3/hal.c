

/*******************************************************************************
* Name  :  Qitas soft for CC1101
*******************************************************************************/
#include "stm32f10x_spi.h"
#include <stdio.h>
#include  "regs.h"
#include  "hal.h"
/*******************************************************************************
* Function Name  : RF_HalInit
*******************************************************************************/
void Hal_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;	
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
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GDO_PORT, &GPIO_InitStructure);			
	  //GDO0_IRQ();
	 // GDO2_IRQ();
}
/*******************************************************************************
* Function Name  :
*******************************************************************************/
u8 GDO0_IRQ(void) 
{	
		EXTI_InitTypeDef EXTI_InitStructure;   
    NVIC_InitTypeDef NVIC_InitStructure; 	
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource11);
    EXTI_ClearITPendingBit(EXTI_Line11);
    EXTI_InitStructure.EXTI_Line = EXTI_Line11;    
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);  	
	
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
		NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;	 
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	  
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure); 
}
/*******************************************************************************
* Function Name  :
*******************************************************************************/
u8 GDO2_IRQ(void) 
{	
		EXTI_InitTypeDef EXTI_InitStructure;   
    NVIC_InitTypeDef NVIC_InitStructure; 	

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);
    EXTI_ClearITPendingBit(EXTI_Line8);
    EXTI_InitStructure.EXTI_Line = EXTI_Line8;    
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);  	
	
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
		NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;	 
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	  
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure); 
}


void halWait(u16 timeout)
{
    int k = 0;
    do {
        for ( k = 0; k < 10; k++) ;
    } while (--timeout);
}// halWait

void halWaitMS(int nCount)	
{
	int i,j;
  for(j=0;j<nCount;j++)
  {
     for(i=0;i<20000;i++);
  }
} 
/*******************************************************************************
* Function Name  : halSpiByteWrite
* Description    : send 8 bit data to SDO line
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
            result |= 0x01;     // Êý¾Ý'1'Èë
        }
        halWait(1);
        halWait(1);
        //SCLK_OUT &= ~SCLK;
        SCLK_LOW();  // SCLK LOW
        halWait(1); halWait(1);
    }
    return result;
}