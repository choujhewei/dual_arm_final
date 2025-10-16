/*
 * pressure.h
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#ifndef INC_PRESSURE_H_
#define INC_PRESSURE_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

uint16_t Read_Pressure_Right(void);
uint16_t Read_Pressure_Left(void);
void sample_and_print(void);

#endif /* INC_PRESSURE_H_ */
