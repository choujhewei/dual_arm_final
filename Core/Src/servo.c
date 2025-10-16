/*
 * servo.c
 *
 *  Created on: Oct 9, 2025
 *      Author: jeffr
 */

#include "grip.h"
#include "tim.h"
#include "adc.h"
#include <stdio.h>

#ifndef SERVO_DEFAULT_SPEED_DPS
#define SERVO_DEFAULT_SPEED_DPS  120.0f   // 預設最大角速度（deg/s）
#endif

#ifndef SERVO_DEFAULT_EMA_ALPHA
#define SERVO_DEFAULT_EMA_ALPHA  0.35f    // 預設 EMA 係數（0~1）
#endif

ServoHandle_t s1, s2;
struct GripStop g1, g2;

// ---------- 小工具 ----------
static inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

static inline uint16_t clamp_u16(uint16_t v, uint16_t lo, uint16_t hi) {
  if (v < lo) { return lo; }
  if (v > hi) { return hi; }
  return v;
}

static inline uint16_t deg_to_us(const ServoHandle_t *s, float deg) {
  float d = clampf(deg, 0.0f, 180.0f);
  float usf = (float)s->us_min + (float)(s->us_max - s->us_min) * (d / 180.0f);
  if (usf < 0.0f) { usf = 0.0f; }
  return (uint16_t)(usf + 0.5f);
}

static inline void set_ccr_us(const ServoHandle_t *s, uint16_t us) {
  __HAL_TIM_SET_COMPARE(s->htim, s->channel, us);
}

// ---------- 對外 API ----------
void Servo_Attach(ServoHandle_t *s, TIM_HandleTypeDef *htim, uint32_t channel,
                  uint16_t us_min, uint16_t us_max)
{
  s->htim     = htim;
  s->channel  = channel;
  s->us_min   = (us_min < 200) ? 200 : us_min;
  s->us_max   = (us_max > 30000) ? 30000 : us_max;
  if (s->us_max <= s->us_min + 10) {
    s->us_max = s->us_min + 10;
  }

  s->cur_deg        = 90.0f;
  s->tgt_deg        = 90.0f;
  s->max_speed_dps  = SERVO_DEFAULT_SPEED_DPS;
  s->ema_alpha      = SERVO_DEFAULT_EMA_ALPHA;
  s->ema_us_f       = (float)deg_to_us(s, s->cur_deg);
  s->ema_us         = (uint16_t)(s->ema_us_f + 0.5f);
  s->last_ms        = HAL_GetTick();
  s->attached       = 1U;

  HAL_TIM_PWM_Start(s->htim, s->channel);
  set_ccr_us(s, s->ema_us);
}

void Servo_WriteMicroseconds(ServoHandle_t *s, uint16_t us)
{
  if (!s || !s->attached) return;
  uint16_t clipped = clamp_u16(us, s->us_min, s->us_max);

  float alpha = clampf(s->ema_alpha, 0.0f, 1.0f);
  s->ema_us_f = (1.0f - alpha) * s->ema_us_f + alpha * (float)clipped;
  if (s->ema_us_f < 0.0f) s->ema_us_f = 0.0f;

  s->ema_us = (uint16_t)(s->ema_us_f + 0.5f);
  set_ccr_us(s, s->ema_us);
}

void Servo_WriteDegrees(ServoHandle_t *s, float deg)
{
  if (!s || !s->attached) { return; }
  uint16_t us = deg_to_us(s, deg);
  s->cur_deg  = clampf(deg, 0.0f, 360.0f); // 立即更新目前角（無限速）
  Servo_WriteMicroseconds(s, us);
}

void Servo_SetTargetDegrees(ServoHandle_t *s, float target_deg)
{
  if (!s) { return; }
  s->tgt_deg = clampf(target_deg, 0.0f, 180.0f);
}

void Servo_SetMaxSpeedDps(ServoHandle_t *s, float max_speed_dps)
{
  if (!s) { return; }
  if (max_speed_dps < 1.0f)   { max_speed_dps = 1.0f;   }
  if (max_speed_dps > 1000.f) { max_speed_dps = 1000.f; }
  s->max_speed_dps = max_speed_dps;
}

void Servo_SetEmaAlpha(ServoHandle_t *s, float alpha)
{
  if (!s) { return; }
  s->ema_alpha = clampf(alpha, 0.0f, 1.0f);
}

void Servo_Update(ServoHandle_t *s)
{
  if (!s || !s->attached) { return; }

  uint32_t now = HAL_GetTick();
  uint32_t dt_ms = now - s->last_ms;
  if (dt_ms == 0U) { return; } // 同一個tick內就先不更新
  s->last_ms = now;

  float dt = (float)dt_ms / 1000.0f;
  float err = s->tgt_deg - s->cur_deg;

  // 限速器：一步最多移動 (max_speed_dps * dt)
  float max_step = s->max_speed_dps * dt;
  float step;
  if (err > 0.0f) {
    step = (err > max_step) ? max_step : err;
  } else {
    step = (err < -max_step) ? -max_step : err;
  }
  s->cur_deg += step;

  // 角度轉 μs，做 EMA 後輸出
  uint16_t us = deg_to_us(s, s->cur_deg);
  Servo_WriteMicroseconds(s, us);
}

uint8_t Servo_IsBusy(ServoHandle_t *s, float tol_deg)
{
  if (!s) { return 0U; }
  float tol = (tol_deg < 0.1f) ? 0.1f : tol_deg;
  float err = s->tgt_deg - s->cur_deg;
  if (err < 0.0f) { err = -err; }
  return (err > tol) ? 1U : 0U;
}

bool Servo_MoveToBlocking(ServoHandle_t* s, float deg, float tol_deg, float dps, uint32_t timeout_ms)
{
  if (!s || !s->attached) return false;

  Servo_SetTargetDegrees(s, deg);
  Servo_SetMaxSpeedDps(s, dps);

  uint32_t start_ms = HAL_GetTick();
  while(Servo_IsBusy(s, tol_deg)){
    Servo_Update(s);
    if (HAL_GetTick() - start_ms > timeout_ms) {
      return false;
    }
    HAL_Delay(5);
  }
  float original_alpha = s->ema_alpha;

  Servo_SetEmaAlpha(s, 1.0f);

  Servo_WriteDegrees(s, deg);

  Servo_SetEmaAlpha(s, original_alpha);

  s->cur_deg = deg;

  return true;
}

void ServoSystem_Init(void)
{
  Servo_Attach(&s1, &htim3, TIM_CHANNEL_1, 800, 2200);
  Servo_Attach(&s2, &htim3, TIM_CHANNEL_2, 800, 2200);

  Servo_SetMaxSpeedDps(&s1, 80.0f);
  Servo_SetMaxSpeedDps(&s2, 80.0f);
  Servo_SetEmaAlpha(&s1, 0.35f);
  Servo_SetEmaAlpha(&s2, 0.35f);

  Servo_WriteDegrees(&s1, 100.0f);
  HAL_Delay(300);
  Servo_WriteDegrees(&s2, 100.0f);
  HAL_Delay(300);

  GripStop_Init(&g1, &s1, PRESS_RIGHT, 100.0f, 20.0f, 100.0f, 2, 80.0f, 0.35f, 1.0f, 5000);
  GripStop_Init(&g2, &s2, PRESS_LEFT,  100.0f, 20.0f, 200.0f, 1, 80.0f, 0.35f, 1.0f, 5000);

  GripStop_Start(&g1);
  GripStop_Start(&g2);
}
