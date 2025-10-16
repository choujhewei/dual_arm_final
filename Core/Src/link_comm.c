/*
 * link_comm.c
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#include "link_comm.h"
#include "motion.h"
#include "dynamixel.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static UART_HandleTypeDef* link_uart;
uint8_t rx_char;
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint16_t last_rx_index = 0;
volatile bool data_ready = false;

void LinkComm_Init(UART_HandleTypeDef* huart) {
    link_uart = huart;
    HAL_UART_Receive_DMA(link_uart, rx_buffer, RX_BUFFER_SIZE);
    __HAL_UART_ENABLE_IT(link_uart, UART_IT_IDLE);
    HAL_UART_Receive_IT(link_uart, &rx_char, 1);
}

void LinkComm_Task(void) {
    if (data_ready) {
        data_ready = false;
        parse_and_control((char*)rx_buffer);
        memset(rx_buffer, 0, sizeof(rx_buffer));
    }
}

void print_to_link(const char *fmt, ...) {
    char buff[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, sizeof(buff), fmt, args);
    HAL_UART_Transmit(link_uart, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);
    va_end(args);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == link_uart->Instance) {
        uint8_t rx_data = rx_char;
        if (rx_data == '!' || last_rx_index >= RX_BUFFER_SIZE - 1) {
            rx_buffer[last_rx_index] = '\0';
            last_rx_index = 0;
            data_ready = true;
        } else {
            rx_buffer[last_rx_index++] = rx_data;
        }
        HAL_UART_Receive_IT(link_uart, &rx_char, 1);
    } else if (huart->Instance == UART4) {
        Dynamixel_Handle.dma_rx_complete_flag = true;
    }
}

void HAL_UART_IDLE_Callback(UART_HandleTypeDef *huart) {
    if (huart->Instance == link_uart->Instance) {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        uint16_t current_pos = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);
        uint16_t data_len = (current_pos >= last_rx_index) ?
                             (current_pos - last_rx_index) :
                             (RX_BUFFER_SIZE - last_rx_index + current_pos);
        if (data_len > 0) {
            memcpy(rx_buffer, &rx_buffer[last_rx_index], data_len);
            rx_buffer[data_len] = '\0';
            data_ready = true;
        }
        last_rx_index = current_pos;
    }
}
