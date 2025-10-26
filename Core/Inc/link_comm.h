/*
 * link_comm.h
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#ifndef INC_LINK_COMM_H_
#define INC_LINK_COMM_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define RX_BUFFER_SIZE 2048

extern uint8_t rx_buffer[RX_BUFFER_SIZE];
extern volatile bool data_ready;

extern volatile char link_mark;

void LinkComm_Init(UART_HandleTypeDef* huart);
void LinkComm_Task(void);
void print_to_link(const char *fmt, ...);
void HAL_UART_IDLE_Callback(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* INC_LINK_COMM_H_ */
