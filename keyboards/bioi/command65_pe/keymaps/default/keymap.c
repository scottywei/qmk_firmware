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

#include QMK_KEYBOARD_H

#include "print.h"
#include "../../../ch5x2.h"
#include "quantum.h"

ble_led_stat_t ble_led;

enum my_keycodes {
  QK_CH0 = QK_KB,
  QK_CH1,
  QK_CH2,
  QK_CH3,
  QK_CH4,
  QK_CH5,
  QK_CLR,
  QK_PAR,
  QK_TOG
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // clang-format off
    // 0: Base Layer
    [0] = LAYOUT(
                                                                                                                                KC_NO, KC_NO, \
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,   KC_5,   KC_6,   KC_7,   KC_8,   KC_9,    KC_0,    KC_MINS,  KC_EQL,  KC_BSPC,  KC_DEL, KC_HOME, \
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,   KC_T,   KC_Y,   KC_U,   KC_I,   KC_O,    KC_P,    KC_LBRC,  KC_RBRC,           KC_BSLS,  KC_PGUP, \
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,   KC_G,   KC_H,   KC_J,   KC_K,   KC_L,    KC_SCLN, KC_QUOT,  KC_NONUS_HASH, KC_ENT,   KC_PGDN, \
        KC_LSFT, KC_NUBS,   KC_Z,    KC_X,    KC_C,   KC_V,   KC_B,   KC_N,   KC_M,   KC_COMM, KC_DOT,  KC_SLSH,  KC_RSFT, KC_UP,    KC_END, \
        KC_LCTL, KC_LGUI, KC_LALT,                          KC_SPC,                          KC_RALT, MO(1),   KC_LEFT,  KC_DOWN,  KC_RIGHT),

    // 1: Function Layer
    [1] = LAYOUT(
                                                                                                                                      _______, _______, \
        _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  _______, _______, QK_BOOT, \
        QK_CH0,  QK_CH1,  QK_CH2,  QK_CH3,  _______,  _______,  _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, QK_TOG, QK_PAR, QK_CLR, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, AU_TOGG, CK_TOGG, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______,                            _______,                                     _______, _______, _______, _______, _______),

    [2] = LAYOUT(
                                                                                                                                      _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______,                            _______,                                     _______, _______, _______, _______, _______),

    [3] = LAYOUT(
                                                                                                                                      _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
        _______, _______, _______,                            _______,                                     _______, _______, _______, _______, _______),
    // clang-format on
};

bool led_update_user(led_t led_state) {
    if(!ble_on) {
        if (LED_PIN_ON_STATE) {
            gpio_write_pin(LED_CAPS_LOCK_PIN, led_state.caps_lock);
        }
    } else {
        // ble_led.caplck = led_state.caps_lock;
        // ble_led.numlck = led_state.num_lock;
        // ble_led.srclck = led_state.scroll_lock;
    }
    return false;
}

void keyboard_pre_init_user(void) {
    setPinOutput(LED_CAPS_LOCK_PIN);
    writePinLow(LED_CAPS_LOCK_PIN);

    setPinOutput(F5);
    writePinLow(F5);

    setPinOutput(E2);
    writePinLow(E2);
}

void keyboard_post_init_user(void) {
  // Customise these values to desired behaviour
  // debug_enable=true;
  // debug_matrix=false;
  // debug_keyboard=true;
  // debug_mouse=true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case QK_OUTPUT_USB:
            if (record->event.pressed) {
                force_ble = 0;
            }
            break;
        case QK_OUTPUT_BLUETOOTH :
            if (record->event.pressed) {
                force_ble = 1;
            }
            break;
        case QK_TOG:
            if (record->event.pressed) {
                force_ble = !force_ble;
            }
            break;
        case QK_CLR:
            if (record->event.pressed) {
                clear_paired();
            }
            break;
        case QK_PAR:
            if (record->event.pressed) {
                pair_module();
            }
            break;
        case QK_CH0:
            if (record->event.pressed) {
                interface_switch(2);
            }
            break;
        case QK_CH1:
            if (record->event.pressed) {
                interface_switch(3);
            }
            break;
        case QK_CH2:
            if (record->event.pressed) {
                interface_switch(4);
            }
            break;
        case QK_CH3:
            if (record->event.pressed) {
                interface_switch(5);
            }
            break;
        case QK_CH4:
            if (record->event.pressed) {
                interface_switch(6);
            }
            break;
        case QK_CH5:
            if (record->event.pressed) {
                interface_switch(7);
            }
            break;
        default:
            return true;
    }
    return true;
}

