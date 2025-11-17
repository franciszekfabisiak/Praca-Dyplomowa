/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include "distance_estimation.h"
#include "bluetooth_global.h"
#include "bluetooth_reflector.h"
#include "bluetooth_initiator.h"
#include <zephyr/logging/log.h>
#include "logic_main.h"
#include "bluetooth_device_control.h"
#include "bluetooth_anchor.h"

static bool anchor = true; 
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	int err;

    logic_thread_init();
    mark_as_anchor(anchor);
	LOG_INF("Starting Channel Sounding Demo");

	err = ble_init();
    if (err) 
        LOG_ERR("Failed in ble init (err %d)", err);

    is_initiator(anchor);

    if(anchor)
        err = act_as_anchor();

    if(!anchor){
        while(1){
            k_sleep(K_SECONDS(10));
        }
    }
}
// bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);