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

#include QMK_KEYBOARD_H

#include "muse.h"


enum mode {
  INSERT_MODE,
  INSERT_SAVE_MODE,
  COMMAND_MODE,
  REPLACE_MODE_INIT,
  REPLACE_MODE,
  REPEAT_MODE,
  XL_MODE,
};



#define HOLD_SHIFT register_mods(MOD_BIT(KC_LSFT))
#define UNHOLD_SHIFT unregister_mods(MOD_BIT(KC_LSFT))
#define HOLD_ALT register_mods(MOD_BIT(KC_LALT))
#define UNHOLD_ALT unregister_mods(MOD_BIT(KC_LALT))
#define HOLD_GUI register_mods(MOD_BIT(KC_LGUI))
#define UNHOLD_GUI unregister_mods(MOD_BIT(KC_LGUI))
#define HOLD_CTRL register_mods(MOD_BIT(KC_LCTRL))
#define UNHOLD_CTRL unregister_mods(MOD_BIT(KC_LCTRL))
#define ALT KC_LALT
#define GUI KC_LGUI
#define SHIFT KC_LSFT
#define CTRL KC_LCTRL

#define CMDBUFFSIZE 16
#define SAVEBUFFSIZE 16
#define NO_CHAR '\0'


bool visual_mode = false;
int       currmode = INSERT_MODE;
uint8_t   vim_layer;
int       currcmdsize = 0;
char      currcmd[CMDBUFFSIZE];
int       savedcmdsize = 0;
char      savedcmd[CMDBUFFSIZE];
int       currsavesize = 0;
uint16_t  saved_keycodes[SAVEBUFFSIZE];
bool      saved_shiftstate[SAVEBUFFSIZE];

// If REPEAT_MODE (user used .)
// just insert everything from buffer, then go back to command mode
// else go insert mode
void go_insert_mode(void) {
  if(currmode == REPEAT_MODE) { 
    for(int i = 0; i < currsavesize; i++) {
      if(saved_shiftstate[i]) HOLD_SHIFT;
      tap_code(saved_keycodes[i]);
      if(saved_shiftstate[i]) UNHOLD_SHIFT;
    }
    currmode = COMMAND_MODE;
    return;
  }
  savedcmdsize = currcmdsize;
  memcpy(savedcmd, currcmd, CMDBUFFSIZE);
  currcmdsize = 0;
  currsavesize = 0;
  currmode = INSERT_SAVE_MODE;
  visual_mode = false;
  layer_move(0);
}

void add_to_vim_cmd(char c) {
  currcmd[currcmdsize++] = c;
  currcmd[currcmdsize]   = NO_CHAR;
  if(currcmdsize == CMDBUFFSIZE - 1)
    go_insert_mode();
}

void tap_code_num(uint16_t keycode, int num) {
  for (int i = 0; i < num; i++)
    tap_code(keycode);
}

void mod_type_num(uint16_t modcode, uint16_t keycode, int num) {
  register_mods(MOD_BIT(modcode));
  tap_code_num(keycode, num);
  unregister_mods(MOD_BIT(modcode));
}

void mod_type(uint16_t modcode, uint16_t keycode) {
  mod_type_num(modcode, keycode, 1);
}

int get_prev_num(void) {
  int val = 0;
  int dec = 1;
  for (int i = currcmdsize - 2; i >= 0; i--) {
    char c = currcmd[i];
    if (c >= '0' && c <= '9') {
      val += (c - '0') * dec;
      dec *= 10;
    }
  }
  return val > 500 ? 500 : val;
}

char get_prev_char(void) {
  for (int i = currcmdsize - 2; i >= 0; i--) {
    char c = currcmd[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
      return c;
  }
  return NO_CHAR;
}

char get_2nd_prev_char(void) {
  bool first_found = false;
  for (int i = currcmdsize - 2; i >= 0; i--) {
    char c = currcmd[i];
    if (!first_found && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
      first_found = true;
    else if (first_found && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
      return c;
  }
  return NO_CHAR;
}

void handle_vim_cmd(void);

bool handle_cmd(char last_char, char prev_char, int num) {
  uint16_t direction = KC_RIGHT;
  if (num == 0) num = 1;

  switch (last_char) {
    case 'h':
    case 'j':
    case 'k':
    case 'l':
      if (last_char == 'h')
        direction = KC_LEFT;
      else if (last_char == 'j')
        direction = KC_DOWN;
      else if (last_char == 'k')
        direction = KC_UP;

      switch (prev_char) {
        case 'c':
        case 'd':
        case 'y':
          mod_type_num(SHIFT, direction, num);
          if (prev_char == 'y') {
            mod_type(CTRL, KC_C);
            tap_code(KC_LEFT);
            tap_code(KC_RIGHT);
          }
          tap_code(KC_DEL);
          if (prev_char == 'c')
            go_insert_mode();
          break;
        default:
          if(visual_mode) HOLD_SHIFT;
          tap_code_num(direction, num);
          if(visual_mode) UNHOLD_SHIFT;
          break;
      }
      return true;

    case 'w':
    case 'e':
      switch (prev_char) {
        case 'd':
        case 'c':
        case 'y':
          HOLD_CTRL;
          HOLD_SHIFT;
          tap_code_num(KC_RIGHT, num);
          // e vs w, going one further and then back again works in both word and
          // mac apps. Will not work well if this is the last word in the document however

          UNHOLD_CTRL;
          UNHOLD_SHIFT;

          // yw, ye, copy
          if (prev_char == 'y') {
            mod_type(CTRL, KC_C);
            tap_code(KC_LEFT);
            tap_code(KC_RIGHT);
          } else {
            tap_code(KC_DEL);
          }
          // cw, ce leave go insert mode
          if (prev_char == 'c')
            go_insert_mode();

          // :w is save
          if (prev_char == ';' && last_char == 'w' && num == 1) {
            mod_type(CTRL, KC_S);
          }
          return true;

        // Just w or e
        default:
          if(visual_mode) HOLD_SHIFT;
          mod_type_num(CTRL, KC_RIGHT, num);
          
          if(visual_mode) UNHOLD_SHIFT;
          return true;
      }

    case 'v':
      if(visual_mode) {
        visual_mode = false;
        tap_code(KC_RIGHT);
        tap_code(KC_LEFT);
      } else {
        visual_mode = true;
      }
      return true;

    case 'a':
      tap_code(KC_RIGHT);
    case 'i':
      go_insert_mode();
      return true;

    case '/':
      mod_type(CTRL, KC_F);
      go_insert_mode();
      return true;

    case 'u':
      mod_type(CTRL, KC_Z);
      return true;

    case 'o':
      tap_code(KC_END);
      tap_code(KC_ENT);
      go_insert_mode();
      return true;

    case 'D':
    case 'C':
      HOLD_GUI;
      mod_type(SHIFT, KC_RIGHT);
      UNHOLD_GUI;
      tap_code(KC_DEL);
      if (last_char == 'c')
        go_insert_mode();
      return true;

    case 'I':
      mod_type(CTRL, KC_LEFT);
      go_insert_mode();
      return true;

    case 'A':
      mod_type(CTRL, KC_RIGHT);
      go_insert_mode();
      break;

    case 'O':
      tap_code(KC_UP);
      tap_code(KC_END);
      tap_code(KC_ENT);
      go_insert_mode();
      return true;

    case 'p':
      mod_type(CTRL, KC_V);
      return true;

    case 'n':
      mod_type(CTRL, KC_G);
      return true;

    // This is a special case. In vi(m) p either pastes a new line or at the cursor depending
    // on what's in the paste buffer. I can't do that. So p and \ (button right of p) acts
    // as these two cases
    case '\\':
      mod_type(CTRL, KC_END);
      tap_code(KC_ENT);
      mod_type(CTRL, KC_V);
      return true;

    case 'q':
      // :q is save
      if (prev_char == ';' && num == 1) {
        mod_type(CTRL, KC_Q);
      }
      return true;

    case 'b':
      switch (prev_char) {
        case 'd':
          HOLD_SHIFT;
          mod_type_num(CTRL, KC_LEFT, num);
          UNHOLD_SHIFT;
          tap_code(KC_DEL);
          break;

        default:
          if(visual_mode) HOLD_SHIFT;
          mod_type_num(CTRL, KC_LEFT, num);
          if(visual_mode) UNHOLD_SHIFT;
          break;
      }
      return true;

    case 'd':
      switch (prev_char) {
        case 'd':
          tap_code(KC_HOME);
          mod_type_num(SHIFT, KC_END, num);
          tap_code(KC_DEL);
          return true;
        case NO_CHAR:
          if(visual_mode) {
            visual_mode = false;
            num = 1;
            tap_code_num(KC_DEL, num);
            return true;
          }
      }
      return false;

    case 'y':
      switch (prev_char) {
        case 'y':
          tap_code(KC_HOME);
          mod_type_num(SHIFT, KC_END, num);
          mod_type(CTRL, KC_C);
          tap_code(KC_RIGHT);
          tap_code(KC_LEFT);
          return true;
        default:
          if(visual_mode) {
            visual_mode = false;
            mod_type(CTRL, KC_C);
            tap_code(KC_LEFT);
            return true;
          }
      }
      return false;

    case 'x':
      if(visual_mode) {
        visual_mode = false;
        num = 1;
      }
      tap_code_num(KC_DEL, num);
      return true;

    case 'X':
      if(visual_mode) {
        visual_mode = false;
        num = 1;
      }
      tap_code_num(KC_BSPACE, num);
      return true;

    case '0':
    case '^':
      if (currcmdsize != 1)
        return false;
      if(visual_mode) HOLD_SHIFT;
      tap_code(KC_HOME);
      if(visual_mode) UNHOLD_SHIFT;
      return true;

    case '$':
      switch (prev_char) {
        case 'd':
        case 'c':
        case 'y':
          mod_type(SHIFT, KC_HOME);
          if (prev_char == 'y') {
            mod_type(CTRL, KC_C);
            tap_code(KC_LEFT);
            tap_code(KC_RIGHT);
            tap_code(KC_LEFT);
            visual_mode = false;
          } else {
            tap_code(KC_DEL);
            if (prev_char == 'c')
              go_insert_mode();
          }

          return true;
          

        default:
          if(visual_mode) HOLD_SHIFT;
          tap_code(KC_END);
          if(visual_mode) UNHOLD_SHIFT;
          return true;
      }
      return true;

    case 'G':
      if(prev_char == 'd' || prev_char == 'x' || prev_char == 'X' || prev_char == 'c' || prev_char == 'y') {
        HOLD_SHIFT;
        mod_type(CTRL, KC_END);
        UNHOLD_SHIFT;
        if (prev_char == 'c' || prev_char == 'd')
          tap_code(KC_DEL);
        if (prev_char == 'c')
          go_insert_mode();
        if (prev_char == 'y') {
          mod_type(CTRL, KC_C);
          tap_code(KC_LEFT);
          tap_code(KC_RIGHT);
        }
      } else {
        if(visual_mode) HOLD_SHIFT;
        mod_type(CTRL, KC_END);
        if(visual_mode) UNHOLD_SHIFT;
      }
      return true;

    case 'g':
      if (prev_char == 'g') {
        char ch2 = get_2nd_prev_char();
        if(ch2 == 'd' || ch2 == 'x' || ch2 == 'X' || ch2 == 'c' || ch2 == 'y') {
          HOLD_SHIFT;
          mod_type(CTRL, KC_HOME);
          UNHOLD_SHIFT;
          if (ch2 == 'c' || ch2 == 'd')
            tap_code(KC_DEL);
          if (ch2 == 'c')
            go_insert_mode();
          if (ch2 == 'y') {
            mod_type(CTRL, KC_C);
            tap_code(KC_RIGHT);
            tap_code(KC_LEFT);
          }
        } else {
          if(visual_mode) HOLD_SHIFT;
          mod_type(CTRL, KC_HOME);
          if(visual_mode) UNHOLD_SHIFT;
        }
      }
      return false;

    case 'r':
      tap_code(KC_DEL);
      currcmdsize = 0;
      visual_mode = false;
      currmode = REPLACE_MODE_INIT;
      return true;

    case '.':
      if(savedcmdsize != 0) {
        currmode = REPEAT_MODE;
        currcmdsize = savedcmdsize;
        memcpy(currcmd, savedcmd, CMDBUFFSIZE);
        handle_vim_cmd();
        currcmdsize = 0;
      }
      return true;
  }
  return false;
}

const char keycode_to_char_map[58] = {
  NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR,
  'a','b','c','d','e',NO_CHAR,'g','h','i','j',
  'k','l',NO_CHAR, NO_CHAR,'o','p','q','r', NO_CHAR, NO_CHAR,
  'u','v','w','x','y',NO_CHAR,
  '1','2','3','4','5','6','7','8','9','0',
  '$', '^', NO_CHAR, NO_CHAR, ' ', NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, '\\',
  NO_CHAR, ';', NO_CHAR, NO_CHAR, NO_CHAR, '.', '/'
};

const char keycode_to_char_map_shifted[58] = {
  NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR,
  'A',NO_CHAR,'C','D',NO_CHAR,NO_CHAR,'G',NO_CHAR,'I',NO_CHAR,
  NO_CHAR,NO_CHAR,NO_CHAR, NO_CHAR,'O','P',NO_CHAR,NO_CHAR,NO_CHAR, NO_CHAR,
  'U','V',NO_CHAR,'X',NO_CHAR,NO_CHAR,
  NO_CHAR,NO_CHAR,NO_CHAR,'$',NO_CHAR,'^',NO_CHAR,NO_CHAR,NO_CHAR,NO_CHAR,
  NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, '|',
  NO_CHAR, ':', NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR, NO_CHAR
};

char keycode_to_char(uint16_t keycode, keyrecord_t *record) {
  if (keycode == KC_DLR)
    return '$';
  if (keycode == KC_CIRC)
    return '^';
  if (SHIFT_HELD) {
    return keycode_to_char_map_shifted[keycode];
  }
  if(keycode < KC_A || keycode > KC_SLASH)
    return NO_CHAR;
  return keycode_to_char_map[keycode];
}




/* THIS FILE WAS GENERATED!

 *

 * This file was generated by qmk json2c. You may or may not want to

 * edit it directly.

 */
enum vim_keycodes{
  VIM_Q = SAFE_RANGE,
  VIM_NUM,
  VIM_START,
  VIM_A,
  VIM_B,
  VIM_C,
  VIM_CI,
  VIM_D,
  VIM_DI,
  VIM_E,
  VIM_H,
  VIM_G,
  VIM_I,
  VIM_J,
  VIM_K,
  VIM_L,
  VIM_O,
  VIM_P,
  VIM_S,
  VIM_U,
  VIM_V,
  VIM_VS, // visual-line
  VIM_VI,
  VIM_W,
  VIM_X,
  VIM_Y,
  VIM_PERIOD, // to support indent/outdent
  VIM_COMMA,  // and toggle comments
  VIM_SHIFT, // avoid side-effect of supporting real shift.
  VIM_ESC, // bookend
  VIM_SAFE_RANGE // start other keycodes here.
};
 

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

        [0] = LAYOUT_ortho_4x12(KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC, 

                                MO(2), KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_QUOT, KC_ENT,

                                 KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_LBRC, KC_RSFT,

                                  KC_LCTL, KC_LGUI, KC_LALT, TT(1), KC_SPC, KC_SPC, KC_SPC, KC_ESC, TT(3), TT(3), TT(4), KC_SLSH),

        [1] = LAYOUT_ortho_4x12(TO(0), KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_BSPC, 

                                      KC_TRNS, RESET, KC_R, KC_S, KC_T, KC_D, KC_H, KC_4, KC_5, KC_6, KC_MINS, KC_ENT, 

                                      KC_LSFT, KC_Z, KC_EXLM, KC_C, KC_V, KC_B, KC_K, KC_1, KC_2,KC_3, KC_PLUS, KC_EQL, 

                                       KC_LCTL, KC_LGUI, KC_LALT, TT(1), KC_SPC, KC_SPC, KC_SPC, KC_ESC, KC_0, KC_PAST, KC_PSLS, KC_SLSH),

        [2] = LAYOUT_ortho_4x12(TO(0), KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_BSPC, 

        TT(2), KC_F11, KC_F12, KC_PGUP, KC_PGDN, KC_WH_U, KC_LEFT, KC_DOWN, KC_UP, KC_RGHT, KC_SCLN, KC_ENT, 

        KC_LSFT, LCTL(KC_DEL), KC_DEL, KC_MINS, KC_EQL, KC_WH_D, KC_HOME, KC_END, KC_GRV, KC_QUES, KC_RBRC, KC_RSFT, 

        KC_LCTL, KC_LGUI, KC_LALT, TT(1), KC_SPC, KC_SPC, KC_SPC, KC_ESC, KC_RCTL, TT(3), TT(4), KC_BSLS),

        [3] = LAYOUT_ortho_4x12(TO(0), KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC, 

                                MO(2), KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_QUOT, KC_ENT,

                                 KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_LBRC, KC_RSFT,

                                  KC_LCTL, KC_LGUI, KC_LALT, VIM_NUM, KC_SPC, KC_SPC, KC_SPC, KC_ESC, VIM_ESC, VIM_ESC, TT(4), KC_SLSH),

        [4] = LAYOUT_ortho_4x12(TO(0), KC_EXLM,KC_MS_U, KC_HASH,KC_TRNS, KC_PGUP, KC_PGUP, KC_AMPR, KC_ASTR, KC_LPRN, KC_HOME, KC_END, 

                                 KC_TRNS, KC_MS_L, KC_MS_D, KC_MS_R, KC_WH_L, KC_WH_U,KC_WH_R, KC_DOWN, KC_UP, KC_RGHT, KC_RCBR, KC_PIPE,

                                  KC_BTN1, KC_ACL0  , KC_ACL1, KC_ACL2, KC_F10, KC_WH_D, KC_VOLU, KC_VOLD, KC_MUTE,KC_MPRV , KC_MNXT, KC_MPLY,

                                  KC_LCTL, KC_LGUI, KC_LALT, TT(1), KC_BTN2, KC_SPC, KC_SPC, KC_ESC, TT(3), TT(3), TT(4), KC_SLSH),

        [5] = LAYOUT_ortho_4x12(TO(0), KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_BSPC, 
         KC_NO, KC_EXLM, KC_AT, KC_HASH, KC_DLR, KC_PERC, KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_LBRC, 
         KC_NO, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, 
         TO(0), KC_NO, KC_NO, VIM_NUM, KC_V, KC_NO, KC_NO, KC_N, KC_M, KC_NO, KC_NO, KC_NO),

        [6] = LAYOUT_ortho_4x12(KC_ESC,KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_BSPC, 

                                KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC, 

                                 KC_LSFT, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_QUOT, KC_ENT,

                                  KC_LCTL, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_SPC,KC_ESC, TT(3), TT(3), TT(4), TO(0)),
        [7] = LAYOUT_ortho_4x12(TO(0), KC_EXLM,KC_MS_U, KC_HASH,KC_TRNS, KC_PGUP, KC_PGUP, KC_AMPR, KC_ASTR, KC_LPRN, KC_HOME, KC_END, 

                                 KC_TRNS, KC_MS_L, KC_MS_D, KC_MS_R, KC_WH_L, KC_WH_U,KC_WH_R, KC_DOWN, KC_UP, KC_RGHT, KC_RCBR, KC_PIPE,

                                  KC_BTN1, KC_ACL0  , KC_ACL1, KC_ACL2, KC_F10, KC_WH_D, KC_VOLU, KC_VOLD, KC_MUTE,KC_MPRV , KC_MNXT, KC_MPLY,

                                  KC_LCTL, KC_LGUI, KC_LALT, TT(1), KC_BTN2, KC_SPC, KC_SPC, KC_ESC, TT(3), TT(3), TT(4), KC_SLSH),



};




const uint16_t PROGMEM qw_combo[] = {KC_Q, KC_W, COMBO_END};
const uint16_t PROGMEM we_combo[] = {KC_W, KC_E, COMBO_END};
const uint16_t PROGMEM er_combo[] = {KC_E, KC_R, COMBO_END};
const uint16_t PROGMEM rt_combo[] = {KC_R, KC_T, COMBO_END};
const uint16_t PROGMEM ty_combo[] = {KC_T, KC_Y, COMBO_END};
const uint16_t PROGMEM yu_combo[] = {KC_Y, KC_U, COMBO_END};
const uint16_t PROGMEM ui_combo[] = {KC_U, KC_I, COMBO_END};
const uint16_t PROGMEM io_combo[] = {KC_I, KC_O, COMBO_END};
const uint16_t PROGMEM op_combo[] = {KC_O, KC_P, COMBO_END};
const uint16_t PROGMEM caps_combo[] = {KC_LSFT, KC_RSFT, COMBO_END};
const uint16_t PROGMEM cv_combo[] = {KC_C, KC_V, COMBO_END};
const uint16_t PROGMEM semi_combo[] = {KC_L, KC_QUOT, COMBO_END};
const uint16_t PROGMEM vb_combo[] = {KC_V, KC_B, COMBO_END};
const uint16_t PROGMEM cs_combo[] = {KC_TAB, KC_BSPC, COMBO_END};

combo_t key_combos[COMBO_COUNT] = {
    COMBO(qw_combo, KC_EXLM),
    COMBO(cv_combo, KC_UNDS),
    COMBO(we_combo, KC_AT ),
    COMBO(er_combo, KC_HASH  ),
    COMBO(rt_combo,  KC_DLR),
    COMBO(ty_combo,  KC_PERC),
    COMBO(yu_combo,  KC_CIRC),
    COMBO(ui_combo,  KC_AMPR),
    COMBO(io_combo,  KC_ASTR),
    COMBO(op_combo,  KC_LPRN),
    COMBO(caps_combo, KC_CAPS ),
    COMBO(semi_combo, KC_COLN ),
    COMBO(vb_combo, KC_PLUS),
    COMBO(cs_combo, TO(6)),
};







#ifdef AUDIO_ENABLE

  float plover_song[][2]     = SONG(PLOVER_SOUND);

  float plover_gb_song[][2]  = SONG(PLOVER_GOODBYE_SOUND);

  float numpad_song[][2] = SONG(NUM_LOCK_ON_SOUND);

#endif




 

bool muse_mode = false;

uint8_t last_muse_note = 0;

uint16_t muse_counter = 0;

uint8_t muse_offset = 70;

uint16_t muse_tempo = 50;
static uint8_t last_layer_on = 0;

 

layer_state_t layer_state_set_user(layer_state_t state) {


    if (layer_state_cmp(state,0) && last_layer_on !=0){
          rgblight_setrgb(250, 255,255) ;
          last_layer_on=0;
          currmode = INSERT_MODE;
          combo_enable();
    }
    
    if (layer_state_cmp(state,1) && last_layer_on !=1){
          rgblight_setrgb(0,0,255) ;
          last_layer_on=1;    
    }
   if (layer_state_cmp(state,3) && last_layer_on !=3){
          rgblight_setrgb(0,255,0) ;
          last_layer_on=  3;    
          currmode = COMMAND_MODE;
    }
  if (layer_state_cmp(state,4) && last_layer_on !=4){
          rgblight_setrgb( 59,255,0) ;
          last_layer_on= 4;    
    }
  if (layer_state_cmp(state,6) && last_layer_on !=6){
          rgblight_setrgb( 0,255,255) ;
          last_layer_on= 6;
          combo_disable();    
    }
  

    return state;

}


void handle_vim_cmd(void) {
  char last_char = currcmd[currcmdsize - 1];
  char prev_char = get_prev_char();
  int  prev_num  = get_prev_num();
  bool lshift    = L_SHIFT_HELD;
  bool rshift    = R_SHIFT_HELD;
  if (lshift) UNHOLD_SHIFT;
  if (rshift) unregister_mods(MOD_BIT(KC_RSFT));

  if (handle_cmd(last_char, prev_char, prev_num))
    currcmdsize = 0;

  if (lshift) HOLD_SHIFT;
  if (rshift) register_mods(MOD_BIT(KC_RSFT));
}

bool handle_vim_mode(uint16_t keycode, keyrecord_t *record, uint8_t vim_layer_no) {
  if (currmode == COMMAND_MODE) {
    switch (keycode) {
      case KC_LALT:
      case KC_LSHIFT:
      case KC_LCTRL:
      case KC_LGUI:
      case KC_RALT:
      case KC_RSHIFT:
      case KC_RCTRL:
      case KC_RGUI:
      case KC_ESC:
        return false;
      case VIM_NUM:
        if (record->event.pressed){
          layer_move(5);
          last_layer_on=5;
        } else {
          layer_move(3);
          last_layer_on=3;
        }
        return false;
      case TO(0):
        go_insert_mode();
        return true;
    }

    if (currcmdsize == 0) {
      // This takes care of holding down one of hjklwbe
      uint16_t newKey = 0;
      switch(keycode) {
        case KC_H:
          newKey = KC_LEFT;
          break;
        case KC_J:
          newKey = KC_DOWN;
          break;
        case KC_K:
          newKey = KC_UP;
          break;
        case KC_L:
          newKey = KC_RIGHT;
          break;
        case KC_W:
          if(record->event.pressed)
            HOLD_CTRL;
          else
            UNHOLD_CTRL;

          newKey = KC_RIGHT;
          break;
        case KC_E:
          if(record->event.pressed)
            HOLD_CTRL;
          else
            UNHOLD_CTRL;
          newKey = KC_RIGHT;
          break;
        case KC_B:
          if(record->event.pressed)
            HOLD_CTRL;
          else
            UNHOLD_CTRL;
          newKey = KC_LEFT;
          break;
      }
      if(newKey != 0) {
        if(record->event.pressed) {
          if(visual_mode)
            HOLD_SHIFT; 
          register_code(newKey);
        } else {
          unregister_code(newKey);
          if(keycode == KC_W) {
            mod_type(CTRL, KC_RIGHT);
          }
          if(visual_mode)
            UNHOLD_SHIFT;
        }
        return true;
      }
    }
  }

  if(currmode == REPLACE_MODE_INIT) {
    currmode = REPLACE_MODE;
    return true;
  } else
  if(currmode == REPLACE_MODE) {
    if (!record->event.pressed) {
      currmode = COMMAND_MODE;
    }
    return false;
  }

  if (!record->event.pressed)
    return currmode != INSERT_MODE && currmode != INSERT_SAVE_MODE;

  vim_layer = vim_layer_no;

  if (keycode == VIM_ESC) {
    currmode = COMMAND_MODE;
    layer_on(vim_layer);
    visual_mode = false;
    currcmdsize = 0;
  }

  if (currmode == INSERT_SAVE_MODE) {
    if(currsavesize == SAVEBUFFSIZE || keycode < KC_A || keycode > KC_SLASH) {
        currmode = INSERT_MODE;
        savedcmdsize = 0;
        currsavesize = 0;
        return false;
    }

    saved_keycodes[currsavesize] = keycode;
    saved_shiftstate[currsavesize++] = SHIFT_HELD;
    return false;
  } else if (currmode == INSERT_MODE) {
    return false;
  }

  if(GUI_HELD) {
    go_insert_mode();
    return false;
  }

  if (CTRL_HELD) {
    if (keycode == KC_R) {
      HOLD_SHIFT;
      UNHOLD_CTRL;
      mod_type(CTRL, KC_Z);
      HOLD_CTRL;
      UNHOLD_SHIFT;
      return true;
    } else if (keycode == KC_D) {
      UNHOLD_CTRL;
      tap_code(KC_PGDN);
      HOLD_CTRL;
      return true;
    } else if (keycode == KC_U) {
      UNHOLD_CTRL;
      tap_code(KC_PGUP);
      HOLD_CTRL;
      return true;
    }
    go_insert_mode();
    return false;
  }

  char ch = keycode_to_char(keycode, record);
  if(ch == NO_CHAR)
    return true;

  add_to_vim_cmd(ch);
  handle_vim_cmd();
  return true;
}



bool process_record_user(uint16_t keycode, keyrecord_t *record) {

  bool vim_handled = handle_vim_mode(keycode, record, last_layer_on);
  if (vim_handled){
        return false;
  }
  
  return true;

  }

