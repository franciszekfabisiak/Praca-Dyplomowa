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
#include <zephyr/logging/log.h>

static bool initiator = true; 
static struct bt_conn *connection;
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	int err;

	LOG_INF("Starting Channel Sounding Demo");

	err = ble_init();
    is_initiator(initiator);

    if (initiator)
    {
        LOG_INF("Initiator");
        err = setup_initiator();
        if (err) {
            LOG_ERR("Failed to setup initiator (err %d)", err);
            return err;
        }

	    err = act_as_initiator();
    }
    else
    {
        LOG_INF("Reflector");
        err = setup_reflector();
        if (err) {
            LOG_ERR("Failed in setup reflector (err %d)", err);
        }

        connection = get_bt_connection();

        err = act_as_reflector(connection);
    }
    is_initiator(false);
    connection = get_bt_connection();
    bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);

    LOG_INF("Reflector");
    err = setup_reflector();
    if (err) {
        LOG_ERR("Failed in setup reflector (err %d)", err);
    }

    connection = get_bt_connection();

    err = act_as_reflector(connection);

	return 0;
}
