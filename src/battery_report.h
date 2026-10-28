// Copyright 2026 Jose Padron
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/*
 * Battery report over the 2.4G dongle.
 *
 * The stock RF link never carries the battery level to the host. This module
 * sends it as a consumer report whose usage is outside the descriptor logical
 * range (1..0x2A0), so the host OS ignores it as a key while hidraw readers
 * still receive it:
 *
 *   usage = BATTERY_REPORT_DISCHARGING | pct   (0x0300..0x0364)
 *   usage = BATTERY_REPORT_CHARGING    | pct   (0x0400..0x0464)
 *
 * followed by a 0x0000 release.
 */

#define BATTERY_REPORT_DISCHARGING 0x0300
#define BATTERY_REPORT_CHARGING    0x0400

#ifdef BATTERY_REPORT_ENABLE
void battery_report_task(void);
void battery_report_request(void);
#else
static inline void battery_report_task(void) {}
static inline void battery_report_request(void) {}
#endif
