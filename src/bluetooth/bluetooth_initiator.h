#ifndef BT_INITIATOR_H
#define BT_INITIATOR_H

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include "common.h"

int setup_initiator(void);
int act_as_initiator(void);

#endif