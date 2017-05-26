#ifndef __USART1_H
#define	__USART1_H

#include "MAP.h"

void USART1_Config(void);
void NVIC_ConfigurationUSART1(void);
void writeCom1(char *p);
int writeCom1Char(int ch);

#endif /* __USART1_H */

