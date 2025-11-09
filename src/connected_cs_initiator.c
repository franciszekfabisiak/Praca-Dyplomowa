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

static const char sample_str[] = "CS Sample";

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN] = {};
	int err;

	if (connection) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_data_parse(ad, data_cb, name);

	if (strcmp(name, sample_str)) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	printk("Found device with name %s, connecting...\n", name);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&connection);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
	}
}

int main(void)
{
	int err;
	connection = get_bt_connection();
	struct k_sem* sem_connected = get_sem_connected();

	printk("Starting Channel Sounding Demo init\n");

	/* Initialize the Bluetooth Subsystem */
	ble_init();

	err = register_step_data_gatt_service();
	if (err) {
		printk("bt_gatt_service_register() returned err %d\n", err);
		return 0;
	}

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return 0;
	}

	k_sem_take(sem_connected, K_FOREVER);

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
