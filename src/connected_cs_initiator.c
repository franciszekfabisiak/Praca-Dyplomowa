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
#include "bluetooth/bluetooth_initiator.h"

#define CS_CONFIG_ID     0
#define NUM_MODE_0_STEPS 1

static struct bt_conn *connection;

int main(void)
{
	int err;
	struct k_sem* sem_connected = get_sem_connected();

	printk("Starting Channel Sounding Demo init\n");

	/* Initialize the Bluetooth Subsystem */
	ble_init();

	err = register_step_data_gatt_service();
	if (err) {
		printk("bt_gatt_service_register() returned err %d\n", err);
		return 0;
	}

	err = start_bt_scan();
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return 0;
	}

	k_sem_take(sem_connected, K_FOREVER);
	printk("After sem connected \n");
	connection = get_bt_connection();

	err = get_bt_le_cs_default_settings(false, connection);
	if (err) {
		printk("Failed to configure default CS settings (err %d)\n", err);
	}

	err = setup_initiator(connection);
	if (err) {
		printk("Failed to setup initiator (err %d)\n", err);
	}

	err = act_as_initiator();

	return 0;
}
