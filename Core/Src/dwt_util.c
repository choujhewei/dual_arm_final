/*
 * dwt_util.c
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#include "dwt_util.h"
#include "stm32f4xx_hal.h"

void DWT_Init(void)
{
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
