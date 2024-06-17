/*
 * main_task.c
 *
 *  Created on: Mar 21, 2024
 *      Author: andrew
 */

#include <stdbool.h>

#include "main.h"

static int period = 0;

//#define DEBUGGING

#ifdef DEBUGGING
const uint32_t coffee_period = (10);
const uint32_t pizza_period = (20);
const uint32_t alarm_repeat = 5;
#else
const uint32_t coffee_period = (4*60);
const uint32_t pizza_period = (25*60);
const uint32_t alarm_repeat = 100;
#endif
/** */
static void disable_NRST(void)
{
	FLASH_OBProgramInitTypeDef obInit;

	HAL_FLASH_Unlock();
	HAL_FLASH_OB_Unlock();
	HAL_FLASHEx_OBGetConfig(&obInit);

	if((obInit.USERConfig & FLASH_OPTR_NRST_MODE) != FLASH_OPTR_NRST_MODE_1)
	{
		obInit.OptionType = OPTIONBYTE_USER;
		obInit.USERConfig = (obInit.USERConfig & ~FLASH_OPTR_NRST_MODE) | FLASH_OPTR_NRST_MODE_1;

		HAL_FLASHEx_OBProgram(&obInit);
	}

	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();
}

/** */
static void buzz(uint32_t period)
{
	HAL_LPTIM_Init(&hlptim2);
	HAL_LPTIM_TimeOut_Start(&hlptim2, 0x0c, 0x06);
	HAL_Delay(period);
	HAL_LPTIM_PWM_Stop(&hlptim2);
}

/** */
static void flash(uint8_t period)
{
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
	HAL_Delay(10);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 1);
}

/** */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	// pushbutton detected: set the period accordingly
	if(GPIO_Pin == SW1_Pin)
	{
		period = coffee_period;
	}

	if(GPIO_Pin == SW2_Pin)
	{
		period = pizza_period;
	}
}

/** */
static void low_power_1s_wait(void)
{
	// set the wakeup timer to 1s: clock source is 32768Hz, prescaler is 128,
	// counter is 250, so period is 1 second
	HAL_LPTIM_Init(&hlptim1);
	HAL_LPTIM_TimeOut_Start_IT(&hlptim1, 0xfa, 0xfa);

	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERMODE_STOP1, PWR_STOPENTRY_WFI);
	HAL_ResumeTick();

	HAL_LPTIM_PWM_Stop(&hlptim1);
}

/** */
void main_task(void)
{
	// NRST functionality must be disabled in order for  SW1 pin to work
	disable_NRST();

	// must delay to have a chance to re-program before STOP mode entered
	for(int i = 0; i < 20; i++)
	{
		flash(100);
		buzz(100);
		HAL_Delay(100);
	}

	while(true)
	{
		// wait for a button press
		HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
		HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
		HAL_SuspendTick();
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERMODE_STOP1, PWR_STOPENTRY_WFI);
		HAL_ResumeTick();
		HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
		HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);

		buzz(100);

		// wait for the chosen period
		for(int delay = 0; delay < period; delay++)
		{
			low_power_1s_wait();
			flash(10);
		}

		// make some noise !!
		for(int alarm = 0; alarm < alarm_repeat; alarm++)
		{
			flash(100);
			buzz(100);
			HAL_Delay(100);
		}
	}
}
