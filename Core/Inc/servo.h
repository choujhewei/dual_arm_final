/*
 * servo.h
 *
 *  Created on: Oct 9, 2025
 *      Author: jeffr
 */

#ifndef INC_SERVO_H_
#define INC_SERVO_H_

#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

struct GripStop;

#ifdef SERVO_TICKS_PER_US
#define SERVO_TICKS_PER_US  1
#endif

typedef struct {
  TIM_HandleTypeDef *htim;
  uint32_t channel;

  uint16_t us_min;
  uint16_t us_max;

  float    cur_deg;
  float    tgt_deg;
  float    max_speed_dps;
  float    ema_alpha;
  float    ema_us_f;
  uint16_t ema_us;

  uint32_t last_ms;
  uint8_t  attached;
} ServoHandle_t;

extern ServoHandle_t s1, s2;
extern struct GripStop g1, g2;

void Servo_Attach(ServoHandle_t *s, TIM_HandleTypeDef *htim, uint32_t channel,
                  uint16_t us_min, uint16_t us_max);

void Servo_WriteMicroseconds(ServoHandle_t *s, uint16_t us);

void Servo_WriteDegrees(ServoHandle_t *s, float deg);

void Servo_SetTargetDegrees(ServoHandle_t *s, float target_deg);

void Servo_SetMaxSpeedDps(ServoHandle_t *s, float max_speed_dps);

void Servo_SetEmaAlpha(ServoHandle_t *s, float alpha);

void Servo_Update(ServoHandle_t *s);

uint8_t Servo_IsBusy(ServoHandle_t *s, float tol_deg);

bool Servo_MoveToBlocking(ServoHandle_t* s, float deg, float tol_deg, float dps, uint32_t timeout_ms);

void ServoSystem_Init(void);

#endif /* INC_SERVO_H_ */
