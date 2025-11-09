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

static struct bt_conn *connection;

int main(void)
{
	int err;
	struct bt_gatt_discover_params* discover_params;

	struct k_sem* sem_discovered = get_sem_discovered();
	struct k_sem* sem_connected = get_sem_connected();
	struct k_sem* sem_config_created = get_sem_config_created();

	printk("Starting Channel Sounding Demo reflect\n");

	ble_init();

	k_sem_take(sem_connected, K_FOREVER);

	connection = get_bt_connection();
	
	err = get_bt_le_cs_default_settings(true, connection);
	if (err) {
		printk("Failed to configure default CS settings (err %d)\n", err);
	}

	k_sem_take(sem_config_created, K_FOREVER);

	discover_params = get_discover_params();

	err = bt_gatt_discover(connection, discover_params);
	if (err) {
		printk("Discovery failed (err %d)\n", err);
		return 0;
	}

	err = k_sem_take(sem_discovered, K_SECONDS(10));
	if (err) {
		printk("Timed out during GATT discovery\n");
		return 0;
	}

	err = act_as_reflector(connection);
	
	return 0;
}
