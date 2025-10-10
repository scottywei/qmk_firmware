/* Copyright 2022 Basic I/O Instruments (Scott Wei)
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

#include "action.h"
#include "host.h"
#include "host_driver.h"
#include "keyboard.h"
#include "led.h"
#include "report.h"
#include <avr/pgmspace.h>

#include "debug.h"
#include "sendchar.h"
#ifdef SLEEP_LED_ENABLE
#    include "sleep_led.h"
#endif
#include "suspend.h"

#include "command.h"
#include "lufa.h"
#include "quantum.h"
#include "usb_descriptor.h"
#include <util/atomic.h>

#include "print.h"

#include "ch5x2.h"
#ifdef QMK_UART
#   include "uart.h"
#else
#   include "usart.h"
#endif

#define BATT_FULL 912
#define BATT_EMPTY 796

#define BUFF_SIZE 25

#ifdef BLE_NAME
#    define AT_NAME_CMD "AT+GAPDEVNAME=" BLE_NAME "\r\n"
#else
#    define AT_NAME_CMD "AT+GAPDEVNAME=BIOI BLE DEVICE\r\n"
#endif

keyboard_config_t kb_config;
ble_led_stat_t ble_led_stat;

bool force_ble = false;
bool channel_paired = false;
bool module_sleep = false;

uint8_t uart_buffer[UART_RX1_BUFFER_SIZE];
uint8_t uart_checksum = 0;

uint8_t uart_led_report[] = {FRAME_HEADER, 0x20, 0x01};

uint8_t uart_channel_new_pairing[] = {FRAME_HEADER, 0x21, 0x01, 0x01, 0x78};
uint8_t uart_channel_connected[] = {FRAME_HEADER, 0x21, 0x01, 0x02, 0x79};
uint8_t uart_channel_reconnect_paired[] = {FRAME_HEADER, 0x21, 0x01, 0x05, 0x7C};
uint8_t uart_channel_never_paired[] = {FRAME_HEADER, 0x21, 0x01, 0x06, 0x7D};

uint8_t uart_sleep[] = {0x55, 0x21, 0x01, 0x08, 0x7F};

// BIOI KB CHn, n=1~5
uint8_t ble_device_name[] = {0x43, 0x6F, 0x6D, 0x6D, 0x61, 0x6E, 0x64, 0x20, 0x36, 0x35, 0x20, 0x43, 0x48};

static uint8_t bluefruit_keyboard_leds = 0;

static void bluefruit_serial_send(uint8_t);

void send_str(const char *str) {
    uint8_t c;
    while ((c = pgm_read_byte(str++)))
#ifdef QMK_UART
        uart_write(c);
        
#else
        uart1_putc(c);
#endif
}

#ifndef QMK_UART
void uart_transmit(const uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        uart1_putc(data[i]);
    }
}
#endif

void serial_send(uint8_t data) {
    dprintf("Sending: %u\n", data);
}

void send_bytes(uint8_t data) {
    char hexStr[3];
    sprintf(hexStr, "%02X", data);
    for (int j = 0; j < 2; j++) {
#ifdef QMK_UART
        uart_write(hexStr[j]);
#else
        uart1_putc(hexStr[j]);
#endif
    }
}

#ifdef BLUEFRUIT_TRACE_SERIAL
static void bluefruit_trace_header(void) {
    dprintf("+------------------------------------+\n");
    dprintf("| HID report to Bluefruit via serial |\n");
    dprintf("+------------------------------------+\n|");
}

static void bluefruit_trace_footer(void) {
    dprintf("|\n+------------------------------------+\n\n");
}
#endif

static void bluefruit_serial_send(uint8_t data) {
#ifdef BLUEFRUIT_TRACE_SERIAL
    dprintf(" ");
    debug_hex8(data);
    dprintf(" ");
#endif
    serial_send(data);
}

/*------------------------------------------------------------------*
 * Host driver
 *------------------------------------------------------------------*/

static uint8_t keyboard_leds(void);
static void    send_keyboard(report_keyboard_t *report);
static void    send_nkro(report_nkro_t *report);
static void    send_mouse(report_mouse_t *report);
static void    send_extra(report_extra_t *report);

// host_driver_t struct changed since the VIA_PROTOCOL_VERSION 0x000B

#if VIA_PROTOCOL_VERSION < 0x000B
static void   send_system(uint16_t data);
static void   send_consumer(uint16_t data);
host_driver_t bluefruit_driver = {keyboard_leds, send_keyboard, send_mouse, send_system, send_consumer};
#elif VIA_PROTOCOL_VERSION >= 0x000B
static void   send_extra(report_extra_t *report);
host_driver_t bluefruit_driver = {keyboard_leds, send_keyboard, send_nkro, send_mouse, send_extra};
#endif

host_driver_t null_driver = {};

static uint8_t keyboard_leds(void) {
    return bluefruit_keyboard_leds;
}
static void send_nkro(report_nkro_t *report) {
}

static void send_keyboard(report_keyboard_t *report) {
    if (module_sleep) {
        wakeup_module();
        wait_ms(5);
    }
    uart_checksum = 0;
#ifdef BLUEFRUIT_TRACE_SERIAL
    bluefruit_trace_header();
#endif

    uart_checksum += FRAME_HEADER;
    uart_checksum += CMD_KEYPRESS;
    uart_checksum += 0x08;

#ifdef QMK_UART
    uart_write(FRAME_HEADER);
    uart_write(CMD_KEYPRESS);
    uart_write(0x08);
#else
    uart1_putc(FRAME_HEADER);
    uart1_putc(CMD_KEYPRESS);
    uart1_putc(0x08);
#endif

    uart_checksum += report->mods;

#ifdef QMK_UART
    uart_write(report->mods);
    uart_write(0);
#else
    uart1_putc(report->mods);
    uart1_putc(0);
#endif

    for (uint8_t i = 0; i < KEYBOARD_REPORT_KEYS; i++) {
            uart_checksum += report->keys[i];
#ifdef QMK_UART
            uart_write(report->keys[i]);
#else
            uart1_putc(report->keys[i]);
#endif
    }

#ifdef QMK_UART
    uart_write(uart_checksum & 0xFF); // send uart checksum byte;
#else
    uart1_putc(uart_checksum & 0xFF); // send uart checksum byte;
#endif

#ifdef BLUEFRUIT_TRACE_SERIAL
    bluefruit_trace_footer();
#endif

}

static void send_mouse(report_mouse_t *report) {
#ifdef BLUEFRUIT_TRACE_SERIAL
    bluefruit_trace_header();
#endif
    bluefruit_serial_send(0xFD);
    bluefruit_serial_send(0x00);
    bluefruit_serial_send(0x03);
    bluefruit_serial_send(report->buttons);
    bluefruit_serial_send(report->x);
    bluefruit_serial_send(report->y);
    bluefruit_serial_send(report->v); // should try sending the wheel v here
    bluefruit_serial_send(report->h); // should try sending the wheel h here
    bluefruit_serial_send(0x00);
#ifdef BLUEFRUIT_TRACE_SERIAL
    bluefruit_trace_footer();
#endif
}

#if VIA_PROTOCOL_VERSION < 0x000B
static void send_extra(uint8_t report_id, uint16_t data) {
#ifdef EXTRAKEY_ENABLE
#ifdef CONSOLE_ENABLE
    uprint_hex8(data);
    uprintf("\r\n");
#endif
    // Construct data frame
    uint8_t frame[6];
    frame[0] = 0x55;                    // Frame header
    frame[1] = 0x83;                    // Command code
    frame[2] = 0x02;                    // Data length
    
    // Extract lower 8 bits and upper 8 bits of usage
    frame[3] = (uint8_t)(data & 0xFF);        // Data low byte
    frame[4] = (uint8_t)((data >> 8) & 0xFF); // Data high byte
    
    // Calculate checksum: accumulate from frame header to last data byte then & 0xFF
    uint8_t checksum = 0;
    for (int i = 0; i < 5; i++) {
        checksum += frame[i];
    }
    frame[5] = checksum & 0xFF;         // Checksum
    
    // Send data frame
    uart_transmit(frame, 6);
    
    // Debug output retained
    char hexStr[9];
    sprintf(hexStr, "%04X", report->usage);
    uprintf("%s \r\n", hexStr);
#endif
}

static void send_system(uint16_t data) {
#ifdef EXTRAKEY_ENABLE
    send_extra(REPORT_ID_SYSTEM, data);
#endif
}

static void send_consumer(uint16_t data) {
#ifdef EXTRAKEY_ENABLE
    send_extra(REPORT_ID_SYSTEM, data);
#endif
}

#elif VIA_PROTOCOL_VERSION >= 0x000B
static void send_extra(report_extra_t *report) {
#ifdef EXTRAKEY_ENABLE
#ifdef CONSOLE_ENABLE
    uprint_hex8(report->usage);
    uprintf("\r\n");
#endif
    
    // Construct data frame
    uint8_t frame[6];
    frame[0] = 0x55;                    // Frame header
    frame[1] = 0x83;                    // Command code
    frame[2] = 0x02;                    // Data length
    
    // Extract lower 8 bits and upper 8 bits of usage
    frame[3] = (uint8_t)(report->usage & 0xFF);        // Data low byte
    frame[4] = (uint8_t)((report->usage >> 8) & 0xFF); // Data high byte
    
    // Calculate checksum: accumulate from frame header to last data byte then & 0xFF
    uint8_t checksum = 0;
    for (int i = 0; i < 5; i++) {
        checksum += frame[i];
    }
    frame[5] = checksum & 0xFF;         // Checksum
    
    // Send data frame
    uart_transmit(frame, 6);
    
#ifdef CONSOLE_ENABLE
    char hexStr[9];
    sprintf(hexStr, "%04X", report->usage);
    uprintf("%s \r\n", hexStr);
#endif
#endif
}
#endif

void ble_clear_keyboard(void) {
    clear_keyboard();

    uint8_t CLEAR_CMD[] = {FRAME_HEADER, CMD_KEYPRESS, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDE};
    uart_transmit(CLEAR_CMD, 12);
    uart_transmit(CLEAR_CMD, 12);
}

void enable_ble_batt(void) {
    send_str(PSTR("AT+BLEBATTEN=1\r\n"));
    wait_ms(100);
    send_str(PSTR("ATZ\r\n"));
    wait_ms(100);
}

void reset_ble_batt(void) {
#ifdef CONSOLE_ENABLE
    print("Resetting BLE Battery Level\r\n");
#endif
    send_str(PSTR("AT+BLEBATTVAL=100\r\n"));
    wait_ms(100);
}

void update_ble_batt(void) {
    uint8_t update_batt_cmd[] = {FRAME_HEADER, 0x87, 0x01, 0x64, 0x41};
    uart_transmit(update_batt_cmd, 5);
}

void module_disconnect(void) {
    send_str(PSTR("AT+GAPDISCONNECT\r\n"));
    wait_ms(100);
    send_str(PSTR("AT+GAPDISCONNECT\r\n"));
    wait_ms(100);
}

void module_delbonds(void) {
    send_str(PSTR("AT+GAPDELBONDS\r\n"));
    wait_ms(100);
    send_str(PSTR("AT+GAPDELBONDS\r\n"));
    wait_ms(100);
    send_str(PSTR("ATZ\r\n"));
    wait_ms(500);
}

void rgb_power_off(void) {
#ifdef RGB_BL_CONTROL_PIN
    setPinOutput(RGB_UG_CONTROL_PIN);
    writePinLow(RGB_BL_CONTROL_PIN);
#endif
#ifdef RGB_UG_CONTROL_PIN
    setPinOutput(RGB_UG_CONTROL_PIN);
    writePinHigh(RGB_UG_CONTROL_PIN);
#endif
#ifdef BIOI_HARPE65
    send_str(PSTR("AT+HWGPIOMODE=19,1\r\n"));
    wait_ms(50);
    send_str(PSTR("AT+HWGPIOMODE=19,1\r\n"));
    wait_ms(50);
    send_str(PSTR("AT+HWGPIO=19,0\r\n"));
    wait_ms(50);
    send_str(PSTR("AT+HWGPIO=19,0\r\n"));
    wait_ms(50);
#endif
}

void rgb_logical_off(void) {
#if defined(RGBLIGHT_ENABLE)
    rgblight_disable();
#endif
#ifdef RGB_MATRIX_ENABLE
    rgb_matrix_disable();
#endif
}

void rgb_power_on(void) {
#ifdef BIOI_MANTA
#if defined(RGBLIGHT_ENABLE)
    rgblight_enable();
#endif
#endif
#ifdef RGB_BL_CONTROL_PIN
    setPinOutput(RGB_UG_CONTROL_PIN);
    writePinHigh(RGB_BL_CONTROL_PIN);
#endif
#ifdef RGB_UG_CONTROL_PIN
    setPinOutput(RGB_UG_CONTROL_PIN);
    writePinLow(RGB_UG_CONTROL_PIN);
#endif
#ifdef BIOI_HARPE65
    send_str(PSTR("AT+HWGPIOMODE=19,1\r\n"));
    wait_ms(50);
    send_str(PSTR("AT+HWGPIOMODE=19,1\r\n"));
    wait_ms(50);
    send_str(PSTR("AT+HWGPIO=19,1\r\n"));
    wait_ms(50);
    send_str(PSTR("AT+HWGPIO=19,1\r\n"));
    wait_ms(50);
#endif
}

void rgb_logical_on(void) {
#if defined(RGBLIGHT_ENABLE)
    rgblight_enable();
#endif
#ifdef RGB_MATRIX_ENABLE
    rgb_matrix_enable();
#endif
}

void force_rgb_off(void) {
    rgb_logical_off();
    rgb_power_off();
    kb_config.raw       = eeconfig_read_kb();
    kb_config.rgb_power = 0;
    eeconfig_update_kb(kb_config.raw);
}

void force_rgb_on(void) {
    rgb_logical_on();
    rgb_power_on();
    kb_config.raw       = eeconfig_read_kb();
    kb_config.rgb_power = 1;
    eeconfig_update_kb(kb_config.raw);
}

void skog_reboot_rgb_strip_only(void) {
    // Reserved function for SKOG Reboot
#if defined(RGBLIGHT_ENABLE)
    rgblight_set_clipping_range(0, 5);
    rgblight_setrgb_range(0, 0, 0, 5, 9);
#endif
}

void skog_reboot_rgb_logo_only(void) {
    // Reserved function for SKOG Reboot
#if defined(RGBLIGHT_ENABLE)
    rgblight_set_clipping_range(4, 5);
    rgblight_setrgb_range(0, 0, 0, 0, 4);
#endif
}

void module_reset(void) {

}

void clear_paired(void) {
    uprintf("Cleaning paired hosts...\r\n");

    clear_keyboard();
    ble_clear_keyboard();

    uint8_t clear_paried_cmd[] = {FRAME_HEADER, 0x49, 0x00, 0x9E};
    uart_transmit(clear_paried_cmd, 4);
}

// The CH5X2 module will go to sleep after being idle for 5 seconds,
// wake it up by sending some null data.
void wakeup_module(void) {
    uint8_t wakeup_cmd[] = {FRAME_HEADER, 0xFF, 0x00, 0x54};
    for (uint8_t i = 0; i < 2; i++) {
        uart_transmit(wakeup_cmd, 4);
    }
}

void interface_rename(uint8_t interface) {
    uart_checksum = 0;
    uart_checksum += FRAME_HEADER;
    uart_checksum += 0xC0;
    // Device name : "BIOI KB CHn" length is 11;
    uart_checksum += 0x0E; // "Command 65 CHn" length is 14;
    // checksum of data "BIOI KB CH";
    uart_checksum += 0x03F5; // checksum of "Command 65 CH";
    uart_checksum += (0x30 + interface - 2);

    // Ranem Device if present interface is BLE;
    if ((interface > 2) && (interface <8)) {
        // Rename Device
        uint8_t interface_rename_cmd[] = {FRAME_HEADER, 0xC0, 0x0E};
        uart_transmit(interface_rename_cmd, 3);

        uart_transmit(ble_device_name, 13);

        uint8_t interface_number[] = {(0x30 + interface -2), (uart_checksum & 0xFF)};
        uart_transmit(interface_number, 2);
    }
    uprintf("Renaming module...\r\n");
}

void pair_module(void) {
    uprintf("Paring module...\r\n");
    wait_ms(50);
    uint8_t pair_cmd[] = {FRAME_HEADER, 0x48, 0x00, 0x9D};
    uart_transmit(pair_cmd, 4);

    wait_ms(50);
}

// Update the keyboard output interface according to the configuration from the EEPROM.
void read_eeprom_interface(void) {
    kb_config.raw = eeconfig_read_kb();
#ifdef CONSOLE_ENABLE
    uprintf("Reading kb_config RAW...\r\n");
    uprintf("Present Interface: %d  \r\n", kb_config.interface);
#endif
    if ((kb_config.interface > 1) && (kb_config.interface < 8)) {
        interface_switch(kb_config.interface);
        ble_on = 1;
        force_ble = 1;
    } else {
        uprintf("kb_config.interface is out of valid values.\r\n");
        // Reset the interface configuration to 0x01 if an invalid value is given.
        kb_config.interface = 1;
        eeconfig_update_kb(kb_config.raw);
        force_ble = 0;
    }
#ifdef CONSOLE_ENABLE
    uprintf("RAW: 0x%06X\r\n", kb_config.raw);
    uprintf("interface: %d\r\n", kb_config.interface);
    uprintf("rgb_power: %d\r\n", kb_config.rgb_power);
#endif
}

int find_sequence(uint8_t pattern[], uint8_t pattern_length) {
    uint8_t i;
    // Find command pattern in the UART buffer
    for (i = 0; i <= UART_RX1_BUFFER_SIZE - pattern_length; i++) {
        uint8_t j;
        for (j = 0; j < pattern_length; j++) {
            if (uart_buffer[i + j] != pattern[j]) {
                break;  // Break the loop if it's out of range
            }
        }
        if (j == pattern_length) {
            return i;  // Return pattern position
        }
    }
    // No match, return -1
    return -1;
}

void process_uart_led_report(void) {
    int find_led_stat = find_sequence(uart_led_report, 3);
    if (find_led_stat > -1) {
        uint8_t led_bits = uart_buffer[(find_led_stat+3)];

        // LED status in UART report: bit2: ScrollLock, bit1: CapsLock, bit1: NumLock
        ble_led_stat.srclck = (led_bits >> 2) & 1;
        ble_led_stat.caplck = (led_bits >> 1) & 1;
        ble_led_stat.numlck = (led_bits >> 0) & 1;

        gpio_write_pin(LED_CAPS_LOCK_PIN, ble_led_stat.caplck);
        
        uprintf("SRCLK: %d, CAPLCK: %d, NUMLCK: %d\r\n", ble_led_stat.srclck, ble_led_stat.caplck, ble_led_stat.numlck);
    }

    uprintf("let_stat: %d\r\n", find_led_stat);
}

void process_uart_channel_report(void) {
    int find_paired_stat = find_sequence(uart_channel_never_paired, 5);
    if (find_paired_stat > -1) {
        channel_paired = false;
        pair_module();
    } else {
        channel_paired = true;
    }
    uprintf("channel_paired: %d\r\n", channel_paired);
}

void process_uart_sleep_report(void) {
    int find_sleep_stat = find_sequence(uart_sleep, 5);
    if (find_sleep_stat > -1) {
        module_sleep = true;
    } else {
        module_sleep = false;
    }
    uprintf("module_sleep: %d\r\n", module_sleep);
}

void interface_switch(uint8_t interface) {

    uprintf("Switching Module Interface to: %d \r\n", interface);

    force_ble = 1;
    wakeup_module();

    wait_ms(50);

    clear_keyboard();
    ble_clear_keyboard();

    uart_checksum = 0;
    uart_checksum += FRAME_HEADER;
    uart_checksum += (0x40 + interface);
    uart_checksum += 0x00;

    uint8_t interface_switch_cmd[] = {FRAME_HEADER, (0x40 + interface), 0x00, (uart_checksum & 0xFF)};
    uart_transmit(interface_switch_cmd, 4);

    wait_ms(50);

    interface_rename(interface);
    update_ble_batt();

    uprintf("Saving Interface to: %d \r\n", interface);
    kb_config.interface = interface;
    eeconfig_update_kb(kb_config.raw);
}

void read_uart_buffer(void) {
    bool buffer_loaded = false;
    uint16_t c_buffer;
    uint8_t i = 0;
    memset(uart_buffer, 0, sizeof(uart_buffer));

#ifdef CH5X2_MODULE
#ifdef QMK_UART
        while (uart_available()) {
            c_buffer = uart_read();
            uprintf("%02X\r\n", (c_buffer));
        }
#else
        while (uart1_available()) {
            /*
             * Get received character from ringbuffer
             * uart_getc() returns in the lower byte the received character and
             * in the higher byte (bitmask) the last receive error
             * UART_NO_DATA is returned when no data is available.
             *
             */
            c_buffer = uart1_getc();
            if ((c_buffer) & UART_NO_DATA) {
                /*
                 * No data available from UART
                 */
            } else {
                /*
                 * New data available from UART
                 * Check for Frame or Overrun error
                 */
                buffer_loaded = true;
                if ((c_buffer) & UART_FRAME_ERROR) {
                    /* Framing Error detected, i.e no stop bit detected */
                    //uart_puts_P("UART Frame Error: ");
                    uprintf("UART Frame Error: \r\n");
                }
                if ((c_buffer) & UART_OVERRUN_ERROR) {
                    /*
                     * Overrun, a character already present in the UART UDR register was
                     * not read by the interrupt handler before the next character arrived,
                     * one or more received characters have been dropped
                     */
                    //uart_puts_P("UART Overrun Error: ");
                    uprintf("UART Overrun Error: \r\n");
                }
                if ((c_buffer) & UART_BUFFER_OVERFLOW) {
                    /*
                     * We are not reading the receive buffer fast enough,
                     * one or more received character have been dropped
                     */
                    //uart_puts_P("Buffer overflow error: ");
                    uprintf("Buffer overflow error: \r\n");
                }
                /*
                 * Send received character back
                 */
                
                uprintf("%02X ", c_buffer);
                uart_buffer[i] = c_buffer;
                i++;
            }
        }
        if (buffer_loaded) {
            uprintf("\r\n");
            process_uart_led_report();
            process_uart_channel_report();
            process_uart_sleep_report();
        }
#endif
#endif
}

void usart_init(void) {
#ifdef QMK_UART
    uart_init(115200);
#else
    uart1_init(UART_BAUD_SELECT_DOUBLE_SPEED(115200, 8000000L));
#endif
    wait_ms(100);

    uint8_t set_baudrate_cmd[] = {FRAME_HEADER, 0xC5, 0x04, 0x00, 0x2C, 0x01, 0x00, 0x4B};
    uart_transmit(set_baudrate_cmd, 8);

    wait_ms(100);
#ifdef QMK_UART
    uart_init(76800);
#else
    uart1_init(UART_BAUD_SELECT_DOUBLE_SPEED(76800, 8000000L));
#endif
    wait_ms(100);

#ifdef CH5X2_MODULE
    // Turn DC-DC mode ON, improve power performance
    uint8_t set_dcdc_cmd[] = {FRAME_HEADER, 0xD5, 0x01, 0x01, 0x2C};
    uart_transmit(set_dcdc_cmd, 5);

    update_ble_batt();

    wakeup_module();
    read_eeprom_interface();
#endif
}

bool command_extra(uint8_t code) {
    switch (code) {
        case KC_Q:
            read_uart_buffer();
            return true;
        case KC_1:
            interface_switch(3);
            return true;
        case KC_2:
            interface_switch(4);
            return true;
        case KC_3:
            interface_switch(5);
            return true;
        case KC_Z:
            interface_switch(2);
            return true;    
        case KC_R:
            read_eeprom_interface();
            //module_reset();
            return true;
        case KC_C:
            clear_paired();
            return true;
        case KC_X:
            pair_module();
            return true;
#ifndef NO_BAT_LEVEL
        case KC_V:
            update_ble_batt();
            return true;
#endif
        case KC_B:
            force_ble = !force_ble;
            return true;
        case KC_G:
            force_rgb_off();
            return true;
        case KC_H:
            force_rgb_on();
            return true;
        case KC_LBRC:
            skog_reboot_rgb_strip_only();
            return true;
        case KC_RBRC:
            skog_reboot_rgb_logo_only();
            return true;
        default:
            clear_keyboard();
            ble_clear_keyboard();
            return false;
    }
}
