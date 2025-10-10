/* Copyright 2022 Basic I/O Instruments (@scottywei)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BLE_H
#define BLE_H

#include "host.h"
#include "host_driver.h"
#include <stdbool.h>


#define FRAME_HEADER 0x55
#define CMD_KEYPRESS 0x81
#define CMD_PAIR 0x48

typedef union {
  uint32_t raw;
  struct {
    // Keybaord output interface: USB: 1; 2.4G: 2; BLE1~5: 3~7;
    uint8_t interface : 3;
    bool rgb_power : 1;
  };
} keyboard_config_t;

extern keyboard_config_t kb_config;

extern bool ble_on;
extern bool channel_paired;
extern bool force_ble;
extern bool module_sleep;

typedef struct {
  bool caplck;
  bool srclck;
  bool numlck;
} ble_led_stat_t;

extern ble_led_stat_t ble_led_stat;

extern host_driver_t bluefruit_driver;
extern host_driver_t null_driver;

void send_str(const char *str);
#ifndef QMK_UART
void uart_transmit(const uint8_t *data, uint16_t length);
#endif
// void uart_getln(char *buffer, uint8_t bufferlimit);
void ble_clear_keyboard(void);
void wakeup_module(void);
void interface_switch(uint8_t interface);
void interface_update(void);
void usart_init(void);
void module_reset(void);
void clear_paired(void);
void pair_module(void);
void enable_ble_batt(void);
void reset_ble_batt(void);
void update_ble_batt(void);
void rgb_power_off(void);
void rgb_power_on(void);
void rgb_logical_off(void);
void rgb_logical_on(void);
void force_rgb_on(void);
void force_rgb_off(void);
void read_uart_buffer(void);

#endif