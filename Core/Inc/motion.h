/*
 * motion.h
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#ifndef INC_MOTION_H_
#define INC_MOTION_H_

#include <stdbool.h>
#include <stdint.h>

void init_dynamixels(void);
float cnt_to_deg360(int32_t pos_cnt);
int32_t deg0to360_to_cnt(float deg);
uint32_t pv_raw_from_deg_s(float deg_per_sec);
int8_t idx_of_id(uint8_t id);
bool live_slowdown_until_reached(uint8_t target_count, const uint8_t* target_ids, const float* target_degs, uint32_t timeout);
void parse_and_control(char* input);
void init_move_all_to_180(void);
void update_all_motors(void);

extern volatile bool done;

#endif /* INC_MOTION_H_ */
