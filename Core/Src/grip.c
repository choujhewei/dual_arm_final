/*
 * grip.c
 *
 *  Created on: Oct 9, 2025
 *      Author: jeffr
 */

#include "grip.h"
#include <stdio.h>

uint16_t Read_Pressure_Right(void);
uint16_t Read_Pressure_Left(void);

static inline uint16_t read_pressure_raw(PressSide s)
{
	return (s==PRESS_RIGHT) ? Read_Pressure_Right() : Read_Pressure_Left();
}

void GripStop_Init(GripStop* g,
                   ServoHandle_t* servo,
                   PressSide side,
                   float start_deg,
                   float target_deg,
                   float press_thresh_raw,
                   uint8_t debounce_cnt_req,
                   float max_speed_dps,
                   float ema_alpha,
                   float tol_deg,
                   uint32_t timeout_ms)
{
  g->servo             = servo;
  g->side              = side;
  g->start_deg         = start_deg;
  g->target_deg        = target_deg;
  g->press_thresh_raw  = press_thresh_raw;
  g->debounce_cnt_req  = (debounce_cnt_req == 0) ? 1 : debounce_cnt_req;
  g->max_speed_dps     = (max_speed_dps < 1.0f) ? 1.0f : max_speed_dps;
  g->ema_alpha         = (ema_alpha < 0.f) ? 0.f : (ema_alpha > 1.f ? 1.f : ema_alpha);
  g->tol_deg           = (tol_deg < 0.1f) ? 0.1f : tol_deg;
  g->timeout_ms        = timeout_ms;

  g->hold_deg          = start_deg;
  g->last_pressure_pct = 0.f;

  g->stopped = 0;
  g->deb_cnt = 0;
  g->state   = GRIP_RUNNING;

  Servo_SetMaxSpeedDps(g->servo, g->max_speed_dps);
  Servo_SetEmaAlpha   (g->servo, g->ema_alpha);
}

void GripStop_Start(GripStop* g)
{
  if (!g || !g->servo) return;

  (void)Servo_MoveToBlocking(g->servo, g->start_deg, g->tol_deg,
                             g->max_speed_dps, g->timeout_ms ? g->timeout_ms : 5000);

  Servo_SetMaxSpeedDps(g->servo, g->max_speed_dps);
  Servo_SetTargetDegrees(g->servo, g->target_deg);

  g->stopped = 0;
  g->deb_cnt = 0;
  g->state   = GRIP_RUNNING;
}

GripStopState GripStop_Update(GripStop* g)
{
  Servo_Update(g->servo);

  uint16_t raw = read_pressure_raw(g->side);
  // printf("Press=%u, deb_cnt=%u, stopped=%d\r\n", raw, g->deb_cnt, g->stopped);

  if (g->stopped) return g->state;

  if (raw >= g->press_thresh_raw) {
    if (++g->deb_cnt >= g->debounce_cnt_req) {
      const float hold = g->servo->cur_deg;
      Servo_SetEmaAlpha(g->servo, 1.0f);
      Servo_SetMaxSpeedDps(g->servo, 999.0f);
      Servo_SetTargetDegrees(g->servo, hold);

      g->stopped  = 1;
      g->state    = GRIP_TRIGGERED;
      g->hold_deg = hold;
      printf("[STOP] Triggered at press=%u, angle=%.1f°\r\n", raw, hold);
      return g->state;
    }
  } else {
    g->deb_cnt = 0;
  }

  if (!Servo_IsBusy(g->servo, g->tol_deg)) {
    static uint16_t empty_cnt = 0;
    if (raw < (g->press_thresh_raw - 10)) {
      if (++empty_cnt >= g->debounce_cnt_req) {
        g->stopped = 1;
        g->state   = GRIP_REACHED;
        return g->state;
      }
    } else {
      empty_cnt = 0;
    }
  }

  return GRIP_RUNNING;
}

GripStopState GripStop_RunBlocking(GripStop* g)
{
  if (!g) return GRIP_RUNNING;
  uint32_t t0 = HAL_GetTick();

  GripStop_Start(g);
  while (1) {
    GripStopState st = GripStop_Update(g);
    if (st == GRIP_TRIGGERED || st == GRIP_REACHED) return st;

    if (g->timeout_ms && (HAL_GetTick() - t0) > g->timeout_ms) {
      return GRIP_RUNNING;
    }
    HAL_Delay(2);
  }
}
