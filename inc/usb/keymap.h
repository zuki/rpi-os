//
// keymap.h
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef INC_USB_KEYMAP_H
#define INC_USB_KEYMAP_H

#include "types.h"

#define PHY_MAX_CODE    127

#define K_NORMTAB        0
#define K_SHIFTTAB        1
#define K_ALTTAB        2
#define K_ALTSHIFTTAB    3

typedef enum
{
    KeyNone  = 0x00,
    KeySpace = 0x100,
    KeyEscape,
    KeyBackspace,
    KeyTabulator,
    KeyReturn,
    KeyInsert,
    KeyHome,
    KeyPageUp,
    KeyDelete,
    KeyEnd,
    KeyPageDown,
    KeyUp,
    KeyDown,
    KeyLeft,
    KeyRight,
    KeyF1,
    KeyF2,
    KeyF3,
    KeyF4,
    KeyF5,
    KeyF6,
    KeyF7,
    KeyF8,
    KeyF9,
    KeyF10,
    KeyF11,
    KeyF12,
    KeyApplication,
    KeyCapsLock,
    KeyPrintScreen,
    KeyScrollLock,
    KeyPause,
    KeyNumLock,
    KeyKP_Divide,
    KeyKP_Multiply,
    KeyKP_Subtract,
    KeyKP_Add,
    KeyKP_Enter,
    KeyKP_1,
    KeyKP_2,
    KeyKP_3,
    KeyKP_4,
    KeyKP_5,
    KeyKP_6,
    KeyKP_7,
    KeyKP_8,
    KeyKP_9,
    KeyKP_0,
    KeyKP_Center,
    KeyKP_Comma,
    KeyKP_Period,
    KeyMaxCode
} special_key_t;

typedef enum
{
    ActionSwitchCapsLock = KeyMaxCode,
    ActionSwitchNumLock,
    ActionSwitchScrollLock,
    ActionSelectConsole1,
    ActionSelectConsole2,
    ActionSelectConsole3,
    ActionSelectConsole4,
    ActionSelectConsole5,
    ActionSelectConsole6,
    ActionSelectConsole7,
    ActionSelectConsole8,
    ActionSelectConsole9,
    ActionSelectConsole10,
    ActionSelectConsole11,
    ActionSelectConsole12,
    ActionShutdown,
    ActionNone
} special_action_t;


typedef struct keymap
{
    uint16_t keymap[PHY_MAX_CODE+1][K_ALTSHIFTTAB+1];

    boolean capslock;
    boolean numlock;
    boolean scrolllock;
} keymap_t;


void keymap(keymap_t *self);
void _keymap(keymap_t *self);

boolean km_clear_table(keymap_t *self, uint8_t table);
boolean km_set_entry(keymap_t *self, uint8_t table, uint8_t pcode, uint16_t value);

uint16_t km_translate(keymap_t *self, uint8_t pcode, uint8_t modifiers);
const char *km_get_string(keymap_t *self, uint16_t kcode, uint8_t modifiers, char buffer[2]);

uint8_t km_get_led_status(keymap_t *self);

#endif
