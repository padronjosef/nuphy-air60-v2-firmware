// Copyright 2026 Jose Padron
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ansi.h"
#include "host.h"
#include "timer.h"
#include "battery_report.h"

// Minimum time between sends triggered by a battery level change.
#ifndef BATTERY_REPORT_CHANGE_INTERVAL_MS
#    define BATTERY_REPORT_CHANGE_INTERVAL_MS 60000
#endif

// Minimum time between sends triggered by a request (key press, reconnect).
#ifndef BATTERY_REPORT_REQUEST_INTERVAL_MS
#    define BATTERY_REPORT_REQUEST_INTERVAL_MS 2000
#endif

// Charger plug state must be stable this long before it is reported (connector bounce).
#ifndef BATTERY_REPORT_CHARGE_DEBOUNCE_MS
#    define BATTERY_REPORT_CHARGE_DEBOUNCE_MS 1000
#endif

// Delay after the link connects so the RF status sync can refresh the level.
#ifndef BATTERY_REPORT_SETTLE_MS
#    define BATTERY_REPORT_SETTLE_MS 3000
#endif

extern DEV_INFO_STRUCT dev_info;
extern bool            f_goto_sleep;
extern bool            f_wakeup_prepare;

void uart_send_report(uint8_t report_type, uint8_t *report_buf, uint8_t report_size);

static bool     pending       = true;
static bool     was_connected = false;
static uint16_t last_sent     = 0;
static uint32_t last_send_time;
static uint32_t connect_time;
static bool     reported_charging = false;
static bool     charge_changing   = false;
static uint32_t charge_change_time;

void battery_report_request(void) {
    pending = true;
}

static bool charger_connected(void) {
    return dev_info.rf_charge & 0x01;
}

// Plugging or unplugging the charger is reported right away, not after the change interval.
static void track_charger(void) {
    if (charger_connected() == reported_charging) {
        charge_changing = false;
        return;
    }
    if (!charge_changing) {
        charge_changing    = true;
        charge_change_time = timer_read32();
    } else if (timer_elapsed32(charge_change_time) >= BATTERY_REPORT_CHARGE_DEBOUNCE_MS) {
        charge_changing   = false;
        reported_charging = charger_connected();
        pending           = true;
    }
}

static uint16_t battery_report_value(void) {
    uint8_t pct = dev_info.rf_baterry;
    if (pct > 100) pct = 100;
    return (reported_charging ? BATTERY_REPORT_CHARGING : BATTERY_REPORT_DISCHARGING) | pct;
}

static void send_consumer_usage(uint16_t usage) {
    uint8_t buf[2] = {usage & 0xFF, usage >> 8};
    // Call uart_send_report directly: the uart_send_*_report wrappers reset
    // no_act_time, which would keep the keyboard from going to sleep.
    uart_send_report(CMD_RPT_CONSUME, buf, sizeof(buf));
}

void battery_report_task(void) {
    bool connected = (dev_info.link_mode == LINK_RF_24) && (dev_info.rf_state == RF_CONNECT);

    if (!connected) {
        was_connected = false;
        return;
    }
    if (!was_connected) {
        was_connected = true;
        connect_time      = timer_read32();
        pending           = true;
        reported_charging = charger_connected();
        charge_changing   = false;
    }

    track_charger();

    // Never send while asleep or waking up, and never cut a held media key.
    if (f_goto_sleep || f_wakeup_prepare) return;
    if (timer_elapsed32(connect_time) < BATTERY_REPORT_SETTLE_MS) return;
    if (host_last_consumer_usage() != 0) return;

    uint16_t value = battery_report_value();
    uint32_t since = timer_elapsed32(last_send_time);

    if (pending) {
        if (last_sent != 0 && since < BATTERY_REPORT_REQUEST_INTERVAL_MS) return;
    } else {
        if (value == last_sent || since < BATTERY_REPORT_CHANGE_INTERVAL_MS) return;
    }

    send_consumer_usage(value);
    send_consumer_usage(0);

    pending        = false;
    last_sent      = value;
    last_send_time = timer_read32();
}
