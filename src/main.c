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
 * A2 -> PA.4 -> DAC
 * Dioda -> PA.5 -> PWM
 *
 */

void delay(int time)
{
    int i;
    for (i = 0; i < time * 4000; i++) {}
}

int main(void) {

	SystemInit();
	ADC_InitTypeDef adc1;
	ADC_InitTypeDef adc2;

	/*Digit ADC GPIO*/
	GPIO_InitTypeDef gpio_d;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_StructInit(&gpio_d);
	gpio_d.GPIO_Pin = GPIO_Pin_5;
	gpio_d.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(GPIOA, &gpio_d);

	TIM_TimeBaseInitTypeDef tim;
	NVIC_InitTypeDef nvic;


	/*
	 * DAC Pin 4 Init
	 */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*Analog GPIO, Temperature 1 and 2 Input*/
	GPIO_InitTypeDef gpio;
	/*Init pins for temperature sensors*/
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	gpio.GPIO_Mode = GPIO_Mode_AN;
	GPIO_Init(GPIOC, &gpio);

	/*
	 * ADC's modules init (temperature1 and temperature2)
	 */
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

	/*PWM and Timer Initialization*/
	int period = 500 -1;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 7;
	tim.TIM_Period = period;
	TIM_TimeBaseInit(TIM2, &tim);

	//TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	TIM_OCInitTypeDef oc_struct;
	TIM_OCStructInit(&oc_struct);
	oc_struct.TIM_OCMode = TIM_OCMode_PWM1;
	oc_struct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC1Init(TIM2, &oc_struct);

	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_1);

	TIM_Cmd(TIM2, ENABLE);
	TIM_SetCompare1(TIM2, period);


	/*
	 * DAC Configuration (works on PA4)
	 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	DAC_InitTypeDef DAC_InitStructure;
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_Buffer_Switch = DAC_BufferSwitch_Enable;
	DAC_Init(DAC1, DAC_Channel_1, &DAC_InitStructure);
	DAC_Cmd(DAC1, DAC_Channel_1, ENABLE);


	while (!((ADC_GetFlagStatus(ADC1, ADC_FLAG_RDY)
			& ADC_GetFlagStatus(ADC2, ADC_FLAG_RDY))))
		;
	ADC_StartConversion(ADC1);
	ADC_StartConversion(ADC2);

	uint16_t voltage1;
	uint16_t voltage2;

	uint16_t DAC_signal_value = 1;

	/*
	 * Set signal value for DAC (Peltier voltage)
	 */
	DAC1_Set_Signal_Value(DAC_signal_value);

	while (1) {
		delay(20);
		if (period <= 0)
			period = 500-1;
		period --;
		TIM_SetCompare1(TIM2, period);
		voltage1 = ADC_GetConversionValue(ADC1);
		voltage2 = ADC_GetConversionValue(ADC2);
	}
}



/*
 * Set digit value [8 bit conversion (0 - 255)] to convert into analog signal [2.4 - 3.6 V]
 */
void DAC1_Set_Signal_Value(uint16_t value) {

	DAC_SoftwareTriggerCmd(DAC1, DAC_Channel_1, DISABLE);
	DAC_SetChannel1Data(DAC1, DAC_Align_8b_R, value);
	DAC_SoftwareTriggerCmd(DAC1, DAC_Channel_1, ENABLE);
}

/*
 * Get current DAC analog signal value
 */
uint16_t DAC1_Get_Signal_Value(void) {
	uint16_t value;
	value = DAC_GetDataOutputValue(DAC1, DAC_Channel_1);
	return value;
}
