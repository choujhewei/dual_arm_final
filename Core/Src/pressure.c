/*
 * pressure.c
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#include "pressure.h"
#include "adc.h"
#include <stdio.h>

uint16_t Read_Pressure_Right(void)
{
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
  uint32_t val = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);
  return (uint16_t)val;
}

uint16_t Read_Pressure_Left(void)
{
  HAL_ADC_Start(&hadc2);
  HAL_ADC_PollForConversion(&hadc2, HAL_MAX_DELAY);
  uint32_t val = HAL_ADC_GetValue(&hadc2);
  HAL_ADC_Stop(&hadc2);
  return (uint16_t)val;
}

void sample_and_print(void)
{
  static uint32_t last_print_ms = 0;
  if (HAL_GetTick() - last_print_ms > 500) {
    last_print_ms = HAL_GetTick();
    uint16_t adc_val_1 = Read_Pressure_Right();
    float percent_1 = ((float)adc_val_1 / 4095.0f) * 100.0f;
    printf("ADC1: %u, Pressure: %.1f %%\r\n", adc_val_1, percent_1);
    uint16_t adc_val_2 = Read_Pressure_Left();
    float percent_2 = ((float)adc_val_2 / 4095.0f) * 100.0f;
    printf("ADC2: %u, Pressure: %.1f %%\r\n", adc_val_2, percent_2);
  }
}
