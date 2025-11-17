#include "bluetooth_anchor.h"
#include "bluetooth_global.h"
#include <zephyr/logging/log.h>
#include "bluetooth_device_control.h"
#include "bluetooth_initiator.h"

LOG_MODULE_REGISTER(bt_anchor);

int act_as_anchor(void)
{
    is_initiator(true);

    LOG_INF("Initiator");

	int err = start_bt_scan();

	LOG_INF("After sem connected");
	struct bt_conn* connection = get_bt_connection();

    err = discover_logic_gatt_service(connection);

    if (err)
        LOG_ERR("Could not discover logic (err %d)", err);
    uint8_t data = SETTING_REFLECTOR;

    err = logic_gatt_write(connection, data);
    if (err) 
        LOG_ERR("Could not write to logic (err %d)", err);

    k_sleep(K_SECONDS(2));

    err = setup_initiator(false);
    if (err) 
        LOG_ERR("Could not setup initiator (err %d)", err);

    err = act_as_initiator();
    if (err) 
        LOG_ERR("Could not act as initiator (err %d)", err);

    return 0;
}