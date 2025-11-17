#ifndef BT_DEVICE_CONTROL_H
#define BT_DEVICE_CONTROL_H

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

enum device_setting
{
    SETTING_REFLECTOR = 0,
    SETTING_REFLECTOR_DELAY,
    SETTING_INITIATOR,
    SETTING_COUNT

};

int register_logic_gatt_service(void);
int discover_logic_gatt_service(struct bt_conn* conn);
int logic_gatt_write(struct bt_conn *conn, uint8_t data);

#endif