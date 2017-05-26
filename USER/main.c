
#include "usart1.h"
#include "CC1101.h"

u8  TxBuf[61]={0};
u8  RxBuf[61]={0}; 

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
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
* Function Name  : main
* Description    : Main program
*******************************************************************************/
uint8_t color[11][3];
uint8_t Recv_data[5];

int main(void)
{
	  u8 cnt;
    u16 buff;
		RCC->APB2ENR |= 1<<4;
		GPIOC->CRH&=0XFF0FFFFF;
		GPIOC->CRH|=0X00300000;
	  CC1101_Init(); 
	  while(1)
		{
//				Delay_ms(300);
//			  halRfSendPacket(RxBuf,30) ;
//			
		     //RF_RX();
				if(RxBuf[0]!=0)
				{
					GPIOC->ODR^=0X2000;
					RxBuf[0]=0;
					RxBuf[1]=0;
					RxBuf[2]=0;
					RxBuf[3]=0;
				}
			  Delay_ms(10);
		}
		NVIC_SystemReset();
}






#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif



/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
