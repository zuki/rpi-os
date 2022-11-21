//
// keymap.c
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
#include "usb/keymap.h"
#include "usb/hid.h"
#include "types.h"
#include "console.h"
#include "string.h"

#define KEYPAD_FIRST    0x53
#define KEYPAD_LAST        0x63

// order must match TSpecialKey beginning at KeySpace
static const char *key_str[KeyMaxCode-KeySpace] =
{
    " ",            // KeySpace
    "\x1b",         // KeyEscape
    "\x7f",         // KeyBackspace
    "\t",           // KeyTabulator
    "\n",           // KeyReturn
    "\x1b[2~",      // KeyInsert
    "\x1b[1~",      // KeyHome
    "\x1b[5~",      // KeyPageUp
    "\x1b[3~",      // KeyDelete
    "\x1b[4~",      // KeyEnd
    "\x1b[6~",      // KeyPageDown
    "\x1b[A",       // KeyUp
    "\x1b[B",       // KeyDown
    "\x1b[D",       // KeyLeft
    "\x1b[C",       // KeyRight
    "\x1b[[A",      // KeyF1
    "\x1b[[B",      // KeyF2
    "\x1b[[C",      // KeyF3
    "\x1b[[D",      // KeyF4
    "\x1b[[E",      // KeyF5
    "\x1b[17~",     // KeyF6
    "\x1b[18~",     // KeyF7
    "\x1b[19~",     // KeyF8
    "\x1b[20~",     // KeyF9
    0,              // KeyF10
    0,              // KeyF11
    0,              // KeyF12
    0,              // KeyApplication
    0,              // KeyCapsLock
    0,              // KeyPrintScreen
    0,              // KeyScrollLock
    0,              // KeyPause
    0,              // KeyNumLock
    "/",            // KeyKP_Divide
    "*",            // KeyKP_Multiply
    "-",            // KeyKP_Subtract
    "+",            // KeyKP_Add
    "\n",           // KeyKP_Enter
    "1",            // KeyKP_1
    "2",            // KeyKP_2
    "3",            // KeyKP_3
    "4",            // KeyKP_4
    "5",            // KeyKP_5
    "6",            // KeyKP_6
    "7",            // KeyKP_7
    "8",            // KeyKP_8
    "9",            // KeyKP_9
    "0",            // KeyKP_0
    "\x1b[G",       // KeyKP_Center
    ",",            // KeyKP_Comma
    "."             // KeyKP_Period
};

#define C(chr)        ((uint16_t) (uint8_t) (chr))

static const uint16_t default_map[PHY_MAX_CODE+1][K_ALTSHIFTTAB+1] =
{
    #include "keymap_us.h"
};

void keymap(keymap_t *self)
{
    assert (self != 0);
    self->capslock = false;
    self->numlock = true;
    self->scrolllock = false;

    assert (sizeof self->keymap == sizeof default_map);
    memcpy (self->keymap, default_map, sizeof self->keymap);
}

void _keymap (keymap_t *self)
{
}

boolean km_clear_table(keymap_t *self, uint8_t table)
{
    assert (self != 0);

    if (table > K_ALTSHIFTTAB) {
        return false;
    }

    for (unsigned pcode = 0; pcode <= PHY_MAX_CODE; pcode++)
    {
        self->keymap[pcode][table] = KeyNone;
    }

    return true;
}

boolean km_set_entry(keymap_t *self, uint8_t table, uint8_t pcode, uint16_t value)
{
    assert (self != 0);

    if (table > K_ALTSHIFTTAB || pcode == 0
     || pcode > PHY_MAX_CODE  || value >= KeyMaxCode) {
        return false;
    }

    self->keymap[pcode][table] = value;

    return true;
}

uint16_t km_translate(keymap_t *self, uint8_t pcode, uint8_t modifiers)
{
    assert (self != 0);

    if (pcode == 0 || pcode > PHY_MAX_CODE) {
        return KeyNone;
    }

    uint16_t norm = self->keymap[pcode][K_NORMTAB];

    if (norm == KeyDelete && (modifiers & (LCTRL | RCTRL))
                          && (modifiers & ALT)) {
        return ActionShutdown;
    }

    if ((KeyF1 <= norm && norm <= KeyF12) && (modifiers & ALT)) {
        return ActionSelectConsole1 + (norm - KeyF1);
    }

    if (modifiers & (ALT | LWIN | RWIN)) {
        return KeyNone;
    }

    unsigned table = K_NORMTAB;

    // TODO: hard-wired to keypad
    if (KEYPAD_FIRST <= pcode && pcode <= KEYPAD_LAST) {
        if (self->numlock) {
            table = K_SHIFTTAB;
        }
    } else if (modifiers & ALTGR) {
        if (modifiers & (LSHIFT | RSHIFT)) {
            table = K_ALTSHIFTTAB;
        } else {
            table = K_ALTTAB;
        }
    } else if (modifiers & (LSHIFT | RSHIFT)) {
        table = K_SHIFTTAB;
    }

    uint16_t lcode = self->keymap[pcode][table];

    switch (lcode)
    {
    case KeyCapsLock:
        self->capslock = !self->capslock;
        return ActionSwitchCapsLock;

    case KeyNumLock:
        self->numlock = !self->numlock;
        return ActionSwitchNumLock;

    case KeyScrollLock:
        self->scrolllock = !self->scrolllock;
        return ActionSwitchScrollLock;
    }

    return lcode;
}

const char *km_get_string(keymap_t *self, uint16_t kcode, uint8_t modifiers, char buffer[2])
{
    assert (self != 0);

    if (kcode <= ' ' || kcode >= KeyMaxCode) {
        return 0;
    }

    if (KeySpace <= kcode && kcode < KeyMaxCode) {
        return key_str[kcode-KeySpace];
    }

    char ch = (char) kcode;

    if (modifiers & (LCTRL | RCTRL)) {
        ch -= 'a';
        if ('\0' <= ch && ch <= 'z'-'a') {
            buffer[0] = ch + 1;
            buffer[1] = '\0';

            return buffer;
        }

        return 0;
    }

    if (self->capslock) {
        if ('A' <= ch && ch <= 'Z') {
            ch += 'a'-'A';
        } else if ('a' <= ch && ch <= 'z') {
            ch -= 'a'-'A';
        }
    }

    buffer[0] = ch;
    buffer[1] = '\0';

    return buffer;
}

uint8_t km_get_led_status(keymap_t *self)
{
    assert (self != 0);

    uint8_t result = 0;

    if (self->capslock) {
        result |= LED_CAPS_LOCK;
    }

    if (self->numlock) {
        result |= LED_NUM_LOCK;
    }

    if (self->scrolllock) {
        result |= LED_SCROLL_LOCK;
    }

    return result;
}
