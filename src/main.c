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
#include "common.h"
#include "bluetooth/bluetooth_global.h"
#include "bluetooth/bluetooth_reflector.h"
#include "bluetooth/bluetooth_initiator.h"

static bool is_initiator = false; 
static struct bt_conn *connection;

int main(void)
{
	int err;

	printk("Starting Channel Sounding Demo\n");

	err = ble_init();

    if (is_initiator)
    {
        printk("Initiator\n");
        err = setup_initiator();
        if (err) {
            printk("Failed to setup initiator (err %d)\n", err);
            return err;
        }

	    err = act_as_initiator();
    }
    else
    {
        printk("Reflector\n");
        err = setup_reflector();
        if (err) {
            printk("Failed in setup reflector (err %d)\n", err);
        }

        connection = get_bt_connection();

        err = act_as_reflector(connection);
    }
	return 0;
}
