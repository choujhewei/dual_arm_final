/*
 * dynamixel.h
 *
 *  Created on: Oct 16, 2025
 *      Author: jeffr
 */

#ifndef INC_DYNAMIXEL_H_
#define INC_DYNAMIXEL_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    UART_HandleTypeDef* huart;
    GPIO_TypeDef*       dir_port;
    uint16_t            dir_pin;
    volatile bool       dma_rx_complete_flag;
} DynamixelBus_t;

extern DynamixelBus_t Dynamixel_Handle_R;
extern DynamixelBus_t Dynamixel_Handle_L;

void Dynamixel_begin_right(UART_HandleTypeDef* uart, GPIO_TypeDef* dir_port, uint16_t pin);
void Dynamixel_begin_left (UART_HandleTypeDef* uart, GPIO_TypeDef* dir_port, uint16_t pin);
void Dynamixel_onDmaRxComplete(UART_HandleTypeDef *huart);

void Dynamixel_setDirPin(bool is_tx);

void Dynamixel_transmitPacket(uint8_t id, uint8_t instruction, const uint8_t* params, uint16_t param_len);
bool Dynamixel_receiveStatusPacket(uint8_t* buffer, uint16_t buffer_size, uint32_t timeout_ms);
bool Dynamixel_readByte(uint8_t id, uint16_t address, uint8_t* value, uint32_t timeout_ms);

void Dynamixel_write(uint8_t id, uint16_t address, const uint8_t* data, uint16_t data_len);
void Dynamixel_torqueOn(uint8_t id);
void Dynamixel_setOperatingMode(uint8_t id, uint8_t mode);
int32_t Dynamixel_getPresentPosition(uint8_t id);

void Dynamixel_SyncWrite(uint16_t address, uint16_t data_len, const uint8_t* ids, uint8_t id_count, const uint8_t* all_data);
void Dynamixel_SyncRead(uint16_t address, uint16_t data_len, const uint8_t* ids, uint8_t id_count);

unsigned short update_crc(unsigned short crc_accum, unsigned char *data_blk_ptr, unsigned short data_blk_size);

#endif /* INC_DYNAMIXEL_H_ */
