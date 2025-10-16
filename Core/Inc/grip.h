/*
 * grip.h
 *
 *  Created on: Oct 9, 2025
 *      Author: jeffr
 */

#ifndef INC_GRIP_H_
#define INC_GRIP_H_

#pragma once
#include "servo.h"
#include "adc.h"
#include <stdint.h>

typedef enum {
  PRESS_RIGHT = 1,
  PRESS_LEFT  = 0
} PressSide;

typedef enum {
  GRIP_RUNNING = 0,   // 任務進行中
  GRIP_TRIGGERED,     // 壓力超標 → 停在當下角
  GRIP_REACHED        // 未超標 → 到目標角度停
} GripStopState;

typedef struct GripStop{
  // 伺服
  ServoHandle_t *servo;

  // 壓力來源
  PressSide side;             // 讀左/右壓力
  uint16_t press_thresh_raw;
  uint8_t   debounce_cnt_req; // 去抖：連續幾次超標才算

  // 運動設定
  float start_deg;
  float target_deg;
  float max_speed_dps;        // deg/s
  float ema_alpha;            // 0~1
  float tol_deg;              // 到位誤差
  uint32_t timeout_ms;        // 只在阻塞版使用

  // 觀察用（給你印 log）
  float    hold_deg;          // 觸發時停住的角度
  float    last_pressure_pct; // 最近一次壓力％

  // 私有狀態
  uint8_t  stopped;           // 是否已停
  uint8_t  deb_cnt;           // 去抖計數
  GripStopState state;
} GripStop;

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
                   uint32_t timeout_ms);

void GripStop_Start(GripStop* g);                 // 先到起點，再朝目標走（起點用阻塞等到位）
GripStopState GripStop_Update(GripStop* g);       // 非阻塞更新（主迴圈 1~5ms 呼叫一次）
GripStopState GripStop_RunBlocking(GripStop* g);  // 可選：阻塞到完成或逾時

#endif /* INC_GRIP_H_ */
