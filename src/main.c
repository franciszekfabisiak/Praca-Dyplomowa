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

static bool initiator = false; 
static struct bt_conn *connection;
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

// #if (IS_ENABLED(CONFIG_ANCHOR))
// #endif

int main(void)
{
	int err;
    logic_thread_init();
	LOG_INF("Starting Channel Sounding Demo");

	err = ble_init();
    is_initiator(initiator);

    while(1){
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
            if (err) 
                LOG_ERR("Failed in setup reflector (err %d)", err);

            connection = get_bt_connection();

            err = discover_logic_gatt_service(connection);
            if (err) 
                LOG_ERR("Could not discover logic (err %d)", err);
            uint8_t data = 48;
            err = logic_gatt_write(connection, data);
            if (err) 
                LOG_ERR("Could not write to logic (err %d)", err);

            k_sleep(K_SECONDS(10));

            err = act_as_reflector(connection);
            bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        }
        initiator = !initiator;
        is_initiator(initiator);
        k_sleep(K_SECONDS(5));
    }
	return 0;
}
// bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);