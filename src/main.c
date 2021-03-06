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
#include <limits.h>
#include "stm32f30x_rcc.h"
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"

#define COOLER_POWER_SPL_CONTROL_VOLATEG_MIN 9
#define COOLER_POWER_SPL_CONTROL_VOLATEG_MAX 33

/**
 * Tem. diff between heat side and cool side should be about COOLER_PELTIER_DELTA_T_OPTIMUM/10 C
 * but no more than COOLER_PELTIER_DELTA_T_MAX/10 C
 */
#define COOLER_PELTIER_DELTA_T_OPTIMUM 250
#define COOLER_PELTIER_DELTA_T_MAX 350

#define COOLER_PELTIER_REQUESTED_TMP 175


/*
 * AMPLIFIER x100
 */
#define AMPLIFIER 680

#define MAX_POWER_SUPP_VOLTAGE_INPUT 5

/*
 *
 */
#define MAX_FAN_INTEGRAL 20000

/**
 * PinOut -> https://raw.githubusercontent.com/wiki/RIOT-OS/RIOT/images/nucleo-f303_pinout.png
 * DataSheet -> https://www.st.com/resource/en/datasheet/stm32f303re.pdf
 * A5 -> PC0 -> ADC1 Channel 6
 * A4 -> PC1 -> ADC2 Channel 7
 * A3 -> PB0 -> ADC3 Channel 12
 * A2 -> PA.4 -> DAC
 * Dioda -> PA.5 -> PWM
 * A1 -> PA1 - > PWM
 */

void delay(int time) {
	int i;
	for (i = 0; i < time * 4000; i++) {
	}
}

volatile uint32_t timer_ms = 0;

void SysTick_Handler() {
	if (timer_ms) {
		timer_ms--;
	}
}

void delay_ms(int time) {
	timer_ms = time;
	while (timer_ms) {
	};
}

/*72 MHz div by Prescaler div by fan period should be > 20 kHz*/
int COOLER_BASE_PWM_FAN_PERIOD = 340;
int COOLER_BASE_PWM_FAN_PRESCALER = 7;

int main(void) {

	SystemInit();
	SysTick_Config(SystemCoreClock / 1000);

	ADC_InitTypeDef adc1;
	ADC_InitTypeDef adc2;
	ADC_InitTypeDef adc3;

	/*PWM GPIO*/
	GPIO_InitTypeDef gpio_d, gpio_a;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_StructInit(&gpio_d);
	GPIO_StructInit(&gpio_a);
	gpio_d.GPIO_Pin = GPIO_Pin_5;
	gpio_d.GPIO_Mode = GPIO_Mode_AF;
	gpio_a.GPIO_Pin = GPIO_Pin_1;
	gpio_a.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(GPIOA, &gpio_d);
	GPIO_Init(GPIOA, &gpio_a);

	TIM_TimeBaseInitTypeDef tim;

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


	/*Analog GPIO, Main temperature*/
	GPIO_InitTypeDef gpio_b;
	/*Init pins for temperature sensors*/
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	gpio_b.GPIO_Pin = GPIO_Pin_0;
	gpio_b.GPIO_Mode = GPIO_Mode_AN;
	GPIO_Init(GPIOB, &gpio_b);

	/*
	 * ADC's modules init (temperature1 and temperature2 (hot and cool peltier sides) for fans controlling)
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

	/*
	* ADC modules init (main temperature for main power supplier controlling)
	*/

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC34, ENABLE);
	RCC_ADCCLKConfig(RCC_ADC34PLLCLK_Div10);

	ADC_StructInit(&adc3);
	adc3.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Enable;
	adc3.ADC_NbrOfRegChannel = 1;
	adc3.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
	adc3.ADC_Resolution = ADC_Resolution_12b;
	ADC_Init(ADC3, &adc3);
	ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, ADC_SampleTime_1Cycles5);
	ADC_Cmd(ADC3, ENABLE);

	/*PWM and Timer Initialization*/
	int period = COOLER_BASE_PWM_FAN_PERIOD;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = COOLER_BASE_PWM_FAN_PRESCALER;
	tim.TIM_Period = period;
	TIM_TimeBaseInit(TIM2, &tim);

	//TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	TIM_OCInitTypeDef oc_struct;
	TIM_OCStructInit(&oc_struct);
	oc_struct.TIM_OCMode = TIM_OCMode_PWM1;
	oc_struct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC1Init(TIM2, &oc_struct);
	TIM_OC2Init(TIM2, &oc_struct);

	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_1);

	TIM_Cmd(TIM2, ENABLE);
	//TIM_SetCompare1(TIM2, period);
	//TIM_SetCompare2(TIM2, period);

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
			& ADC_GetFlagStatus(ADC2, ADC_FLAG_RDY)
			& ADC_GetFlagStatus(ADC3, ADC_FLAG_RDY))))
		;
	ADC_StartConversion(ADC1);
	ADC_StartConversion(ADC2);
	ADC_StartConversion(ADC3);

	int peltier_hot_tmp_voltage = 0;
	int peltier_cool_tmp_pelvoltage = 0;

	int main_tmp_voltage = 0;

	uint32_t DAC_signal_value = 255;

	/*
	 * Set signal value for DAC (Peltier voltage)
	 */
	DAC1_Set_Signal_Value(DAC_signal_value);


	int32_t pi_signal;
	int32_t pi_peltier_signal;
	short low_pass_filter_counter = 0;
	uint16_t dac_value = 0;
	while (1) {
		delay_ms(50);
		//TIM_SetCompare1(TIM2, period);
		//TIM_SetCompare2(TIM2, period);

		peltier_hot_tmp_voltage += ADC_GetConversionValue(ADC1);
		peltier_cool_tmp_pelvoltage += ADC_GetConversionValue(ADC2);
		main_tmp_voltage += ADC_GetConversionValue(ADC3);
		low_pass_filter_counter++;




		if(low_pass_filter_counter >= 100){
			int delta_T = (int)(peltier_hot_tmp_voltage/low_pass_filter_counter - peltier_cool_tmp_pelvoltage/low_pass_filter_counter);
			pi_signal = pi_fan_regulator(delta_T);
			set_fan_pwm(pi_signal);

			pi_peltier_signal = pi_peltier_regulator(main_tmp_voltage/low_pass_filter_counter);
			DAC1_Set_Signal_Value(pi_peltier_signal);

			low_pass_filter_counter = 0;
			peltier_hot_tmp_voltage = 0;
			peltier_cool_tmp_pelvoltage = 0;
			main_tmp_voltage = 0;

		}


	}
}

/**
 * Temperature diff between heater and cooler
 */
int pi_fan_regulator(int delta_T_voltage){

	/** LM35 http://www.ti.com/lit/ds/symlink/lm35.pdf
	 * 10-mV/�C Scale Factor
	 * Assumption
	 * - peltier delta T optimum 20C -> COOLER_PELTIER_DELTA_T_OPTIMUM
	 */

	static int32_t integral_delta_T = 0;
	int32_t fan_control_signal = 0;
	int16_t current_state = 0;

	current_state = delta_T_voltage - COOLER_PELTIER_DELTA_T_OPTIMUM;

	if( abs(integral_delta_T)< MAX_FAN_INTEGRAL){
		integral_delta_T += current_state;
	}


	fan_control_signal = (0.3)*current_state + (0.7)*integral_delta_T;
	return fan_control_signal;
}



int pi_peltier_regulator(int current_tmp){

	static int32_t integral_delta_T = 0;
	int32_t peltier_control_signal = 0;
	int16_t current_state = 0;

	current_state = current_tmp - COOLER_PELTIER_REQUESTED_TMP;

	if( abs(integral_delta_T)< MAX_FAN_INTEGRAL){
		integral_delta_T += current_state;
	}


	peltier_control_signal = (0.3)*current_state + (0.7)*integral_delta_T;

	/*
	 * DAC is 8 bit value between 0 and 255 (0 - 3.3V)
	 * Power suppler accepts input 0 - 5.0V
	 * Voltage Apmlifier -> x2
	 */


	return peltier_control_signal;
}



/**
 * @brief
 * fan speed - controlled by 8 bit pwm signal
 */
void set_fan_pwm(int pi_control_signal) {

	uint32_t pwm = 0;
	if(pi_control_signal < 0){
			pwm = 0;
	}else if(pi_control_signal >= COOLER_PELTIER_DELTA_T_MAX){
		pwm = 255; //max 8 bit value (fan full power)
	}else{
		pwm = (float)pi_control_signal/(float)COOLER_PELTIER_DELTA_T_MAX  * 255;
	}

	TIM_SetCompare2(TIM2, pwm);
}

/*
 * Set digit value [8 bit conversion (0 - 255)] to convert into analog signal [0 - 3.3 V]
 * Amplifier = x6.8
 * bit_resolution*x = MAX_POWER_SUPPLIER_INPUT/Amplifier*3.3
 *
 */
void DAC1_Set_Signal_Value(uint32_t value) {

	/*
	 * 3.3/255
	 * Amplifier is x6.8 so max signal value is 58 (58 - > 5V)
	 *
	 * max = MAX_POWER_SUPPLIER_INPUT/Amplifier * DAC_reesolution / procesor_max_output
	 *
	 */
	int max = MAX_POWER_SUPP_VOLTAGE_INPUT / AMPLIFIER / 100 * 255 / 3.3;

	if (value < 0){
		value = 0;
	} else if (value > max){
		value = max;
	}

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
