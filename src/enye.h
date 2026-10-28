// Copyright 2026 Jose Padron
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

/*
 * Fn + N types ñ (Ñ with Shift or Caps Lock) on a US host layout.
 *
 * The keyboard can only send key codes, so it types the Linux Unicode input
 * sequence (Ctrl+Shift+U, code point, Space), which IBus turns into the letter.
 * The sequence uses a short pause per key (ENYE_TAP_DELAY_MS) so the intermediate
 * "u00f1" is not drawn; the rest of the keyboard keeps its stock timing.
 * It is hooked in process_record_kb instead of the keymap because VIA keeps the
 * keymap in EEPROM and only reloads keymap.c when the build date changes.
 */

#ifdef ENYE_ENABLE
bool process_enye(uint16_t keycode, keyrecord_t *record);
#else
static inline bool process_enye(uint16_t keycode, keyrecord_t *record) {
    return true;
}
#endif
