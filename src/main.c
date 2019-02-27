/**
 ******************************************************************************
 * @file    main.c
 * @author  Piotr Z
 * @version V1.0
 * @date    20-Feb-2019
 * @brief   Cooler main function.
 ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include "stm32f30x_rcc.h"
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"

	/**
	 * PinOut -> https://raw.githubusercontent.com/wiki/RIOT-OS/RIOT/images/nucleo-f303_pinout.png
	 * DataSheet -> https://www.st.com/resource/en/datasheet/stm32f303re.pdf
	 * A6 -> PC0 -> ADC1 Channel 6
	 * A5 -> PC1 -> ADC2 Channel 7
	 *
	 */

int main(void) {

	SystemInit();
	ADC_InitTypeDef adc1;
	ADC_InitTypeDef adc2;

	GPIO_InitTypeDef gpio;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	gpio.GPIO_Mode = GPIO_Mode_AN;
	GPIO_Init(GPIOC, &gpio);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC12, ENABLE);
	RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div10);

	ADC_StructInit(&adc1);
	adc1.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Enable;
	adc1.ADC_NbrOfRegChannel = 1;
	adc1.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
	adc1.ADC_Resolution = ADC_Resolution_12b;
	ADC_Init(ADC1, &adc1);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_1Cycles5);
	ADC_Cmd(ADC1, ENABLE);

	ADC_StructInit(&adc2);
	adc2.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Enable;
	adc2.ADC_NbrOfRegChannel = 1;
	adc2.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
	adc2.ADC_Resolution = ADC_Resolution_12b;
	ADC_Init(ADC2, &adc2);
	ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_1Cycles5);
	ADC_Cmd(ADC2, ENABLE);

	while (!((ADC_GetFlagStatus(ADC1, ADC_FLAG_RDY)&ADC_GetFlagStatus(ADC2, ADC_FLAG_RDY))));
	ADC_StartConversion(ADC1);
	ADC_StartConversion(ADC2);

	uint16_t voltage1;
	uint16_t voltage2;

	while (1) {
		voltage1 = ADC_GetConversionValue(ADC1);
		voltage2 = ADC_GetConversionValue(ADC2);
	}
}
