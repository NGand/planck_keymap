/* Copyright 2015-2021 Jack Humbert
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

#ifdef AUDIO_ENABLE
#    define STARTUP_SONG SONG(PLANCK_SOUND)
// #define STARTUP_SONG SONG(NO_SOUND)

#    define DEFAULT_LAYER_SONGS \
        { SONG(QWERTY_SOUND), SONG(COLEMAK_SOUND), SONG(DVORAK_SOUND) }
#endif

/*
 * MIDI options
 */

/* enable basic MIDI features:
   - MIDI notes can be sent when in Music mode is on
*/

#define MIDI_BASIC

/* enable advanced MIDI features:
   - MIDI notes can be added to the keymap
   - Octave shift and transpose
   - Virtual sustain, portamento, and modulation wheel
   - etc.
*/
//#define MIDI_ADVANCED

/* override number of MIDI tone keycodes (each octave adds 12 keycodes and allocates 12 bytes) */
//#define MIDI_TONE_KEYCODE_OCTAVES 2

// Most tactile encoders have detents every 4 stages
#define ENCODER_RESOLUTION 4
#define PERMISSIVE_HOLD

#define TAPPING_TOGGLE 1
#define TAPPING_TERM 125     
#define MOUSEKEY_TIME_TO_MAX 5
#define MOUSEKEY_DELTA 10
#define MOUSEKEY_INTERVAL 16
#define MOUSEKEY_DELAY 150
#define MOUSEKEY_MAX_SPEED 3

#define GUITAB LGUI(KC_TAB)
//#define ALT_AA RALT_T(SWE_AA)

#define L_SHIFT_HELD (get_mods() & (MOD_BIT(KC_LSFT)))
#define R_SHIFT_HELD (get_mods() & (MOD_BIT(KC_RSFT)))
#define SHIFT_HELD (L_SHIFT_HELD || R_SHIFT_HELD)
#define L_CTRL_HELD (get_mods() & (MOD_BIT(KC_LCTRL)))
#define R_CTRL_HELD (get_mods() & (MOD_BIT(KC_RCTRL)))
#define CTRL_HELD (L_CTRL_HELD || R_CTRL_HELD)
#define L_GUI_HELD (get_mods() & (MOD_BIT(KC_LGUI)))
#define R_GUI_HELD (get_mods() & (MOD_BIT(KC_RGUI)))
#define GUI_HELD (L_GUI_HELD || R_GUI_HELD)
#define COMBO_COUNT 14
#define  COMBO_TERM 20
