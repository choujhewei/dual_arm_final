/*
 * motion.c
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#include "motion.h"
#include "dynamixel.h"
#include "link_comm.h"
#include "main.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"

extern const uint8_t DXL_ID_LIST[];
extern const float VEL_DPS[];
#define DXL_ID_CNT 14
static float g_target_deg[DXL_ID_CNT];
volatile bool done = false;

static bool dxl_write_u8(uint8_t id, uint16_t addr, uint8_t val, uint32_t tmo_ms){
    Dynamixel_write(id, addr, &val, 1);
    HAL_Delay(3);
    return true;
}

void init_dynamixels(void) {
	 for (uint8_t i = 0; i < DXL_ID_CNT; i++) {
	      uint8_t id = DXL_ID_LIST[i];
	      dxl_write_u8(id, 64, 0, 50);
	 }

	 for (uint8_t i = 0; i < DXL_ID_CNT; i++) {
          uint8_t id = DXL_ID_LIST[i];
          dxl_write_u8(id, 11, 3, 50);
     }

	 for (uint8_t i = 0; i < DXL_ID_CNT; i++) {
	      uint8_t id = DXL_ID_LIST[i];
	      dxl_write_u8(id, 64, 1, 50);
	      HAL_Delay(20);
	 }

	 uint16_t P = 1500, I = 60, D = 140;
	 Dynamixel_write(2, 84, (uint8_t*)&P, 2);
	 Dynamixel_write(2, 82, (uint8_t*)&I, 2);
	 Dynamixel_write(2, 80, (uint8_t*)&D, 2);
	 Dynamixel_write(9, 84, (uint8_t*)&P, 2);
	 Dynamixel_write(9, 82, (uint8_t*)&I, 2);
	 Dynamixel_write(9, 80, (uint8_t*)&D, 2);
     for (uint8_t i = 0; i < DXL_ID_CNT; ++i) {
          g_target_deg[i] = 180.0f;
     }

}

float cnt_to_deg360(int32_t pos_cnt) {
	return (float)pos_cnt / 4096.0f * 360.0f;
}

int32_t deg0to360_to_cnt(float deg) {
	return (int32_t)(deg / 360.0f * 4096.0f);
}

static inline float wrap360(float deg) {
  while (deg < 0.0f)   deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  return deg;
}

uint32_t pv_raw_from_deg_s(float deg_per_sec) {
    return (uint32_t)(deg_per_sec / 0.229f / 360.0f * 60.0f);
}

int8_t idx_of_id(uint8_t id) {
	for(int i = 0; i < DXL_ID_CNT; i++){
		if(DXL_ID_LIST[i] == id) return i;
	}
	return -1;
}

bool live_slowdown_until_reached(uint8_t target_count,
                                 const uint8_t* target_ids,
                                 const float*   target_degs,
                                 uint32_t timeout)
{
    uint32_t start_time = HAL_GetTick();
    const uint16_t ADDR_PRESENT_POSITION = 132;
    const uint16_t POS_DATA_LEN = 4;

    int32_t  current_positions[DXL_ID_CNT];
    for (uint8_t i = 0; i < DXL_ID_CNT; ++i) current_positions[i] = -1;

    uint8_t idsR[14], idsL[14];
    uint8_t nR = 0,   nL = 0;
    for (uint8_t i = 0; i < target_count; ++i) {
        uint8_t id = target_ids[i];
        if (id >= DXL_RIGHT_ID_MIN && id <= DXL_RIGHT_ID_MAX) idsR[nR++] = id;
        else if (id >= DXL_LEFT_ID_MIN && id <= DXL_LEFT_ID_MAX) idsL[nL++] = id;
    }

    printf(".--------------------------\r\n");
    while (HAL_GetTick() - start_time < timeout) {
        if (nR) {
            Dynamixel_SyncRead(ADDR_PRESENT_POSITION, POS_DATA_LEN, idsR, nR);
            for (uint8_t k = 0; k < nR; ++k) {
                uint8_t pkt[15];
                if (Dynamixel_receiveStatusPacket(pkt, sizeof(pkt), 60)) {
                    uint8_t rid = pkt[4];
                    int8_t  rix = idx_of_id(rid);
                    if (rix >= 0) {
                        current_positions[rix] = (int32_t)(pkt[9] |
                                                (pkt[10] << 8) |
                                                (pkt[11] << 16) |
                                                (pkt[12] << 24));
                    }
                }
            }
        }
        if (nL) {
            Dynamixel_SyncRead(ADDR_PRESENT_POSITION, POS_DATA_LEN, idsL, nL);
            for (uint8_t k = 0; k < nL; ++k) {
                uint8_t pkt[15];
                if (Dynamixel_receiveStatusPacket(pkt, sizeof(pkt), 60)) {
                    uint8_t lid = pkt[4];
                    int8_t  lix = idx_of_id(lid);
                    if (lix >= 0) {
                        current_positions[lix] = (int32_t)(pkt[9] |
                                                (pkt[10] << 8) |
                                                (pkt[11] << 16) |
                                                (pkt[12] << 24));
                    }
                }
            }
        }

        bool all_reached = true;
        printf("Looping (t=%ld):", (long)(HAL_GetTick() - start_time));
        for (uint8_t i = 0; i < target_count; ++i) {
            uint8_t id = target_ids[i];
            int8_t  ix = idx_of_id(id);
            if (ix < 0 || current_positions[ix] == -1) {
                printf(" [ID %d Read FAILED!]", id);
                all_reached = false;
            } else {
                float cur_deg = cnt_to_deg360(current_positions[ix]);
                float err_deg = target_degs[i] - cur_deg;
                printf(" [ID %d: %.1f, E:%.1f]", id, cur_deg, err_deg);
                if (fabsf(err_deg) > 1.5f) {
                    all_reached = false;
                }
            }
        }
        printf("\r\n");

        if (all_reached) {
            printf(".--- MOTORS REACHED ---\r\n");
            return true;
        }
        HAL_Delay(80);
    }

    printf(".--- WAIT TIMEOUT ---\r\n");
    return false;
}

void parse_and_control(char* input) {
    char *p = input;
    bool processed_any = false;

    while (1) {
        char *start = strchr(p, '[');
        if (!start) break;
        char *end   = strchr(start, ']');
        if (!end) break;

        char seg[160];
        size_t len = (size_t)(end - start - 1);
        if (len >= sizeof(seg)) len = sizeof(seg) - 1;
        memcpy(seg, start + 1, len);
        seg[len] = '\0';

        char *saveptr = NULL;
        char *tok = strtok_r(seg, ",", &saveptr);
        if (!tok) { p = end + 1; continue; }
        while (*tok == ' ' || *tok == '\t' || *tok == '\r' || *tok == '\n')
            ++tok;
        char hand = (char)tolower((unsigned char)tok[0]);
        bool is_right = (hand == 'r');

        uint8_t ids[7];
        float   ang_rl[7];
        uint8_t n = 0;

        for (uint8_t j = 0; j < 7; j++) {
            tok = strtok_r(NULL, ",", &saveptr);
            if (!tok) break;
            while (*tok==' ' || *tok=='\t' || *tok=='\r' || *tok=='\n') ++tok;
            char *endp = tok + strlen(tok);
            while (endp > tok && (endp[-1]==' ' || endp[-1]=='\t' || endp[-1]=='\r' || endp[-1]=='\n'))
                *--endp = '\0';
            if (tok[0]=='x' || tok[0]=='X') continue;
            bool has_digit = false;
            for (char *pchk = tok; *pchk; ++pchk) {
                if (*pchk >= '0' && *pchk <= '9') { has_digit = true; break; }
            }
            if (!has_digit) continue;

            float a = (float)atof(tok);
            if (a < -180.0f) a = -180.0f;
            if (a >  180.0f) a =  180.0f;
            uint8_t baseL = 1 + 7;
            uint8_t id = is_right ? (j + 1) : (baseL + j);
            ids[n]    = id;
            ang_rl[n] = a;
            n++;
        }

        if (n == 0) { p = end + 1; continue; }

        const uint16_t ADDR_GOAL_POSITION = 116;

        for (uint8_t k = 0; k < n; k++) {
            float f0_360 = wrap360(ang_rl[k] + 180.0f);
            int32_t goal_cnt = deg0to360_to_cnt(f0_360);

            printf("Moving ID %d to %.1f deg (cnt=%ld)\r\n", ids[k], f0_360, (long)goal_cnt);
            Dynamixel_write(ids[k], ADDR_GOAL_POSITION, (uint8_t*)&goal_cnt, 4);
            HAL_Delay(5);

            int8_t idx = idx_of_id(ids[k]);
            if (idx >= 0) {
                g_target_deg[idx] = f0_360;
            }
        }

        processed_any = true;
        p = end + 1;
    }

    if (processed_any) {
        float goal_all[DXL_ID_CNT];
        for (uint8_t i = 0; i < DXL_ID_CNT; i++)
            goal_all[i] = g_target_deg[i];

        const uint32_t TIMEOUT_MS = 8000;
        bool ok = live_slowdown_until_reached(DXL_ID_CNT, DXL_ID_LIST, goal_all, TIMEOUT_MS);

        if (ok)  {
        	print_to_link("Done\r\n");
        	if (link_mark == '$'){
        	done = true;
        	}
        }
        else     {
        	print_to_link("timeout\r\n");
        }
    }
}

void init_move_all_to_180(void) {
    const uint8_t ADDR_GOAL_POSITION = 116;
    const uint8_t POS_DATA_LEN = 4;
    int32_t goal_pos_raw[DXL_ID_CNT];
    float target_degs[DXL_ID_CNT];

    const uint8_t ADDR_PROFILE_VELOCITY = 112;
    const uint8_t VEL_DATA_LEN = 4;
    uint32_t vel_raw[DXL_ID_CNT];

    for (uint8_t i = 0; i < DXL_ID_CNT; i++) {
        target_degs[i] = 180.0f;
        goal_pos_raw[i] = deg0to360_to_cnt(target_degs[i]);
        vel_raw[i] = pv_raw_from_deg_s(20.0f);
    }

    Dynamixel_SyncWrite(ADDR_PROFILE_VELOCITY, VEL_DATA_LEN, DXL_ID_LIST, DXL_ID_CNT, (uint8_t*)vel_raw);
    HAL_Delay(20);
    Dynamixel_SyncWrite(ADDR_GOAL_POSITION, POS_DATA_LEN, DXL_ID_LIST, DXL_ID_CNT, (uint8_t*)goal_pos_raw);

    printf("All homing commands sent.\r\n--- WAITING FOR MOTORS ---\r\n");
    if(!live_slowdown_until_reached(DXL_ID_CNT, DXL_ID_LIST, target_degs, 15000)) {
    	printf("Timeout waiting for homing completion!\r\n");
    } else {
        printf("All motors reached home position.\r\n");
    }
}

void update_all_motors(void) {
    static uint32_t last_send_ms = 0;
    uint32_t now = HAL_GetTick();
    if (now - last_send_ms < 30) return;
    last_send_ms = now;

    const uint16_t ADDR_PROFILE_ACCEL     = 108;
    const uint16_t ADDR_PROFILE_VELOCITY  = 112;
    const uint16_t ADDR_GOAL_POSITION     = 116;

    uint32_t profile_accel[DXL_ID_CNT];
    uint32_t profile_vel[DXL_ID_CNT];
    int32_t  goal_pos[DXL_ID_CNT];

    const float accel_dps2 = 400.0f;
    const float vel_dps    = 60.0f;

    for (int i = 0; i < DXL_ID_CNT; i++) {
        profile_accel[i] = (uint32_t)(accel_dps2 / 214.577f);
        profile_vel[i]   = (uint32_t)(vel_dps / 0.229f / 360.0f * 60.0f);
        goal_pos[i]      = deg0to360_to_cnt(wrap360(g_target_deg[i])); // 固定例
    }

    Dynamixel_SyncWrite(ADDR_PROFILE_ACCEL, 4, DXL_ID_LIST, DXL_ID_CNT, (uint8_t*)profile_accel);
    HAL_Delay(5);
    Dynamixel_SyncWrite(ADDR_PROFILE_VELOCITY, 4, DXL_ID_LIST, DXL_ID_CNT, (uint8_t*)profile_vel);
    HAL_Delay(5);
    Dynamixel_SyncWrite(ADDR_GOAL_POSITION, 4, DXL_ID_LIST, DXL_ID_CNT, (uint8_t*)goal_pos);
}
