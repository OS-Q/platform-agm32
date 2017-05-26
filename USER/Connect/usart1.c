
#include "usart1.h"
#define   USART1_BUFF_LEN   1023

typedef struct USART1_BUF {
int    step;
char   buff[1024];
} USART1_BUF;

USART1_BUF USART1_comBuff1 = {0};

void USART1_Config()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    USART_InitStructure.USART_BaudRate = 115200;//一般设置为9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
    //USART_ITConfig(USART1, USART_IT_IDLE, ENABLE); 
    USART_Cmd(USART1, ENABLE); //使能串口 
}

void NVIC_ConfigurationUSART1(void)
{
    NVIC_InitTypeDef NVIC_InitStructure; 
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    
    /* 使能串口1中断 */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;     
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    /*IRQ通道使能*/
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    /*根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器USART1*/
    NVIC_Init(&NVIC_InitStructure);
}


void USART1_IRQHandler(void)
{
    unsigned char code;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {     
        code=USART_ReceiveData(USART1);
        USART1_comBuff1.buff[USART1_comBuff1.step % USART1_BUFF_LEN] = code;
        USART1_comBuff1.step = USART1_comBuff1.step + 1;
    }
}

int writeCom1Char(int ch)
{
    USART_SendData(USART1, (unsigned char) ch);
    while( USART_GetFlagStatus(USART1,USART_FLAG_TC)!= SET);    
    return (ch);
}

void writeCom1(char *p)
{
    int i;
		u32 x;
    for (i = 0; i < 1000; i = i +1 )
	{
        if (*(p+i) == 0) {
            writeCom1Char('\0');
            break;
        }
        writeCom1Char(*(p+i));
			x=16000;  
			while(x--);    
    }
}

