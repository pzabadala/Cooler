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

    /* ADC Initialization */
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_AN;
    GPIO_Init(GPIOC, &gpio);


    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC12, ENABLE);
    RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div10);

    ADC_StructInit(&adc);
    adc.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Enable;
    adc.ADC_NbrOfRegChannel = 1;
    adc.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
    adc.ADC_Resolution = ADC_Resolution_12b;
    ADC_Init(ADC1, &adc);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_1Cycles5);

    ADC_Cmd(ADC1, ENABLE);

    while(!(ADC_GetFlagStatus(ADC1, ADC_FLAG_RDY)));
    ADC_StartConversion(ADC1);
    //while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET){}
    float v;
    uint16_t dupa;

	while (1) {
    	dupa = ADC_GetConversionValue(ADC1);
    	v = (float)dupa * 3.3f / 4096.0f;


    }
}
