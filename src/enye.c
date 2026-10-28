// Copyright 2026 Jose Padron
// SPDX-License-Identifier: GPL-2.0-or-later

#include "enye.h"

// Fn layers of the stock keymap: 1 on the Mac side of the switch, 4 on the Windows side.
#define FN_LAYER_MAC 1
#define FN_LAYER_WIN 4

// Pause between the simulated key events. IBus shows "u00f1" until the final Space,
// so the shorter the whole sequence, the less of it is ever drawn on screen. The rest
// of the keyboard keeps the stock TAP_CODE_DELAY.
#ifndef ENYE_TAP_DELAY_MS
#    define ENYE_TAP_DELAY_MS 2
#endif

static void tap_fast(uint16_t keycode) {
    register_code16(keycode);
    wait_ms(ENYE_TAP_DELAY_MS);
    unregister_code16(keycode);
    wait_ms(ENYE_TAP_DELAY_MS);
}

// Linux Unicode input: Ctrl+Shift+U, hex code point, Space. ñ = U+00F1, Ñ = U+00D1.
// The hex digits and the closing Space go out in a single report: Linux emits the keys
// of one report in usage order (F 0x09 / D 0x07, 1 0x1E, Space 0x2C), which is exactly
// the order IBus needs, so nothing is left on screen between the digits and the Space.
static void type_enye(bool upper) {
    const uint8_t tail[] = {upper ? KC_D : KC_F, KC_1, KC_SPACE};

    uint8_t mods      = get_mods();
    uint8_t weak_mods = get_weak_mods();
    clear_mods();
    clear_weak_mods();
    send_keyboard_report();

    tap_fast(LCTL(LSFT(KC_U)));

    for (uint8_t i = 0; i < sizeof(tail); i++) add_key(tail[i]);
    send_keyboard_report();
    wait_ms(ENYE_TAP_DELAY_MS);
    for (uint8_t i = 0; i < sizeof(tail); i++) del_key(tail[i]);
    send_keyboard_report();
    wait_ms(ENYE_TAP_DELAY_MS);

    set_mods(mods);
    set_weak_mods(weak_mods);
    send_keyboard_report();
}

bool process_enye(uint16_t keycode, keyrecord_t *record) {
    if (keycode != KC_N || !(IS_LAYER_ON(FN_LAYER_MAC) || IS_LAYER_ON(FN_LAYER_WIN))) {
        return true;
    }
    if (record->event.pressed) {
        type_enye((get_mods() & MOD_MASK_SHIFT) || host_keyboard_led_state().caps_lock);
    }
    return false;
}
