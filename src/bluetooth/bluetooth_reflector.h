#ifndef BT_REFLECTOR_H
#define BT_REFLECTOR_H

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include "common.h"

int setup_reflector(void);
struct bt_gatt_discover_params* get_discover_params(void);
int act_as_reflector(struct bt_conn* connection);
struct k_sem* get_sem_discovered(void);
struct k_sem* get_sem_written(void);

#endif