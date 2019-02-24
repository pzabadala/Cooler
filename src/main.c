/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f30x_rcc.h"
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"


void delay(int time)
{
    int i;
    for (i = 0; i < time * 4000; i++) {}
}

int main(void)
{
    SystemInit();


	GPIO_InitTypeDef gpio; // obiekt gpio z konfiguracja portow GPIO
	//RCC_AHBPeriphResetCmd(RCC_AHBPeriph_GPIOA, ENABLE);


	RCC_APB2PeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);



    GPIO_StructInit(&gpio); // domyslna konfiguracja
    gpio.GPIO_Pin = GPIO_Pin_5; // konfigurujemy pin 5
    gpio.GPIO_Mode = GPIO_Mode_OUT; // jako wyjscie
    //gpio.GPIO_OType = GPIO_OType_PP;
    //GPIO_Mode_Out_PP
    GPIO_Init(GPIOA, &gpio); // inicjalizacja modulu GPIOA

    while (1) {
        GPIO_SetBits(GPIOA, GPIO_Pin_5); // zapalenie diody
        delay(100);
        GPIO_ResetBits(GPIOA, GPIO_Pin_5); // zgaszenie diody
        delay(400);
    }
}
