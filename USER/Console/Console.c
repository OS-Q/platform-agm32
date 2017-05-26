
#include "tlv.h"
#include "Console.h"
#include "usart1.h"

uint32_t Flash_ID;
uint64_t Flash_SN;

u8 H_addr=0x23;
u8 S_addr=0x23;
u8 S_time=10;

u8  TxBuf[61]={0};
u8  RxBuf[61]={0}; 
u8  TLV[60]={0}; 
u8  leng =60; 
char Debug_msg[256]={0}; 
/*******************************************************************************
* Function Name  : Delay_ms
* Description    :
*******************************************************************************/
void Delay_us(u16 i) 
{    
	 u8 x=0;  
	 while(i--)
	 {
				x=10;  
				while(x--) ;    
	 }
}
void Delay_ms(u16 i) 
{
	u16 x=0;  
	 while(i--)
	 {
			x=8000;  
			while(x--);    
	 }
}


/*******************************************************************************
* Function Name  :Power_Init
* Description    :
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
u8 Power_ON(void)
{	
		GPIO_InitTypeDef GPIO_InitStructure;
	
		RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC| RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO , ENABLE); //
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
		GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
		GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	  //GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); 
	  //GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); 
//	  GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
	
//		FLASH_Unlock();
//		FLASH_ErasePage(ID_ADDR);	
//		FLASH_Lock();	
//		FLASH_Unlock();		
//		FLASH_ProgramWord(ID_ADDR,0x12);
//		FLASH_ProgramWord(SN_ADDR,0x88776655); 		
//		FLASH_ProgramWord(SN_ADDR+4,0x44332211); 
//		FLASH_Lock();

			Flash_ID=*(vu32*)(ID_ADDR);
			Flash_SN=*(vu32*)(SN_ADDR);
			Flash_SN <<=32;
			Flash_SN |=*(vu32*)(SN_ADDR+4);	
			return BACK_SUCCESS;
}


/*******************************************************************************
* Function Name  :Pair_Mode
* Description    :
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
 
void Init(void)
{
		//Init_SI7021();
		CC1101_Init();  
}

/*******************************************************************************
* Function Name  : RF_Send_DHT11
* Description    : Send dht11 data to one.
*******************************************************************************/
u8 Get_Data(void)
{	
		DATA_TLV_S* firstTlv;
		DATA_TLV_S* nextTlv;
		u8 check;	
		Convert_ADC(2); 
		
		firstTlv = getFirstTlv(TxBuf+1);
		firstTlv->type = 1;
		firstTlv->len = 1;
		firstTlv->value[0] = 0;
	
		nextTlv = getNextTlv(firstTlv);
		nextTlv->type = 254;
		nextTlv->len = 8;
		memcpy(nextTlv->value,&Flash_SN,8);
		
		nextTlv = getNextTlv(nextTlv);
		nextTlv->type = 9;
		nextTlv->len = 1;	
	  nextTlv->value[0] = 0x99;		
		
		nextTlv = getNextTlv(nextTlv);
		nextTlv->type = 0;	
		
		computCheckSum(firstTlv,&check);
		firstTlv->value[0] = check;

		check =checkDatagramValid(firstTlv);		
		if(TLV_SUCCESS != check)  return BACK_ERROR; 
		return BACK_SUCCESS;
}
/*******************************************************************************
* Function Name  : RTC_IRQHandler
* Description    : This function handles RTC global interrupt request.
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void RTC_IRQHandler(void)
{
   static uint8_t Display;
   if (RTC_GetITStatus(RTC_IT_SEC) != RESET)
		{
			/* Clear the RTC Second interrupt */
			RTC_ClearITPendingBit(RTC_IT_SEC);  
			Time_Display();
		}
}
