/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include <stdio.h>
#include <stdint.h>
#include "stm32f30x_rcc.h"
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"




void send_char(char c)
{
 while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
 USART_SendData(USART2, c);
}

int __io_putchar(int c)
{
 if (c=='\n')
 send_char('\r');
 send_char(c);
 return c;
}

int main(void)
{
    SystemInit();
    ADC_InitTypeDef adc;
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;

    /* ADC Initialization */
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_AN;
    GPIO_Init(GPIOA, &gpio);


    USART_StructInit(&uart);
    uart.USART_BaudRate = 115200;
    USART_Init(USART2, &uart);
    USART_Cmd(USART2, ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC12, ENABLE);
    RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div8);

    ADC_StructInit(&adc);
    adc.ADC_ContinuousConvMode = ENABLE;
    adc.ADC_NbrOfRegChannel = 1;
    ADC_Init(ADC1, &adc);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_61Cycles5);
    ADC_Cmd(ADC1, ENABLE);

    ADC_StartConversion(ADC1);

    while (1) {
    	uint16_t adc = ADC_GetConversionValue(ADC1);
    	float v = (float)adc * 3.3f / 4096.0f;
    	//printf("ADC = %d (%.3fV)\n", adc, v);

    }
}
