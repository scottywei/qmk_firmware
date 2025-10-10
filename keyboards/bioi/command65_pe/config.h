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

#pragma once

#define USB_POLLING_INTERVAL_MS 1

#define BLE_NAME "BIOI COMMAND65 BLE"
#define BIOI_CMD65
#define BIOI_CMD65_PE
#define NO_BAT_LEVEL
#define CH5X2_MODULE
#define BOOTMAGIC_LITE_ROW 0
#define BOOTMAGIC_LITE_COLUMN 0
#define LED_CAPS_LOCK_PIN F0
#define AUDIO_PIN B7
#define AUDIO_CLICKY
#define STARTUP_SONG SONG(MUSIC_ON_SOUND)
#define DIP_SWITCH_MATRIX_GRID { {4,7}, {4,8} }
#define KEYBOARD_LOCK_ENABLE
#define MAGIC_KEY_LOCK L
#define IS_COMMAND() (get_mods() == (MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT)))