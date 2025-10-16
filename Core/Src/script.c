/*
 * script.c
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include "main.h"
#include "script.h"
#include "motion.h"

static void build_cmd_from_step(const DualStep* st, char* out, size_t cap) {
    size_t pos = 0;
    #define APPEND(fmt, ...) do {                                     \
        if (pos < cap) {                                              \
            int n = snprintf(out + pos, cap - pos, fmt, ##__VA_ARGS__); \
            if (n > 0) pos += (size_t)n;                              \
        }                                                              \
    } while (0)

    APPEND("[R");
    for (int i = 0; i < 7; ++i) {
        APPEND(",");
        if (isnan(st->R[i])) APPEND("x");
        else                 APPEND("%.1f", st->R[i]);
    }
    APPEND("][L");
    for (int i = 0; i < 7; ++i) {
        APPEND(",");
        if (isnan(st->L[i])) APPEND("x");
        else                 APPEND("%.1f", st->L[i]);
    }
    APPEND("]!");
    #undef APPEND
}

void execute_dual_script(const DualStep* steps, uint8_t step_count, uint32_t timeout_ms) {
    (void)timeout_ms;
    for (uint8_t i = 0; i < step_count; ++i) {
        char cmd[256];
        build_cmd_from_step(&steps[i], cmd, sizeof(cmd));
//        printf(".STEP %u/%u: %s\r\n", (unsigned)i + 1, (unsigned)step_count, cmd);
        parse_and_control(cmd);
    }
}
