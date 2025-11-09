#ifndef BT_GLOBAL_H
#define BT_GLOBAL_H

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include "distance_estimation.h"

#define CS_CONFIG_ID     0
#define NUM_MODE_0_STEPS 1
#define NAME_LEN          30
#define STEP_DATA_BUF_LEN 512 /* Maximum GATT characteristic length */
    
int ble_init(void);

int register_step_data_gatt_service(void);
struct bt_conn* get_bt_connection(void);

struct k_sem* get_sem_acl_encryption_enabled(void);
struct k_sem* get_sem_remote_capabilities_obtained(void);
struct k_sem* get_sem_config_created(void);
struct k_sem* get_sem_cs_security_enabled(void);
struct k_sem* get_sem_procedure_done(void);
struct k_sem* get_sem_connected(void);
struct k_sem* get_sem_data_received(void);
uint8_t* get_latest_local_steps(void);
void call_estimate_distance(void);
int get_bt_le_cs_default_settings(bool is_reflector, struct bt_conn *connection);
int start_bt_scan(void);

#endif