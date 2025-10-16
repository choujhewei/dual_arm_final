/*
 * script.h
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#ifndef INC_SCRIPT_H_
#define INC_SCRIPT_H_

typedef struct {
    float R[7];
    float L[7];
} DualStep;

void execute_dual_script(const DualStep* steps, uint8_t step_count, uint32_t timeout_ms);

#endif /* INC_SCRIPT_H_ */
