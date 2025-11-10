#include "bluetooth_reflector.h"
#include "bluetooth_global.h"
#include <zephyr/logging/log.h>
#include "common.h"

static K_SEM_DEFINE(sem_written, 0, 1);
static K_SEM_DEFINE(sem_discovered, 0, 1);

static struct bt_conn *connection;

static struct bt_uuid_128 step_data_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba8));

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_write_params write_params;
static uint16_t step_data_attr_handle;

LOG_MODULE_REGISTER(bt_reflector, CONFIG_LOG_DEFAULT_LEVEL);

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	char str[BT_UUID_STR_LEN];

	LOG_INF("Discovery: attr %p", attr);

	if (!attr) {
		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	bt_uuid_to_str(chrc->uuid, str, sizeof(str));
	LOG_INF("UUID %s", str);

	if (!bt_uuid_cmp(chrc->uuid, &step_data_char_uuid.uuid)) {
		step_data_attr_handle = chrc->value_handle;

		LOG_INF("Found expected UUID");

		k_sem_give(&sem_discovered);
	}

	return BT_GATT_ITER_STOP;
}

static void write_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	LOG_INF("sem_written before");
	if (err) {
		LOG_ERR("Write failed (err %d)", err);

		return;
	}

	k_sem_give(&sem_written);
}

// public functions
int setup_reflector(void)
{
	struct bt_gatt_discover_params* discover_params;
	struct k_sem* sem_connected = get_sem_connected();
	struct k_sem* sem_config_created = get_sem_config_created();
	
	k_sem_take(sem_connected, K_FOREVER);

	connection = get_bt_connection();
	LOG_INF("After sem connected ");

	int err = get_bt_le_cs_default_settings(true, connection);
	if (err) {
		LOG_ERR("Failed to configure default CS settings (err %d)", err);
		return err;
	}

	k_sem_take(sem_config_created, K_FOREVER);
	LOG_INF("After sem_config_created ");

	discover_params = get_discover_params();

	err = bt_gatt_discover(connection, discover_params);
	if (err) {
		LOG_ERR("Discovery failed (err %d)", err);
		return err;
	}

	err = k_sem_take(&sem_discovered, K_SECONDS(10));
	LOG_INF("After sem_discovered ");
	if (err) {
		LOG_ERR("Timed out during GATT discovery");
		return err;
	}
	return 0;
}

struct k_sem* get_sem_written(void)
{
    return &sem_written;
}

struct k_sem* get_sem_discovered(void)
{
    return &sem_discovered;
}

struct bt_gatt_discover_params* get_discover_params(void){
	discover_params.uuid = &step_data_char_uuid.uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return &discover_params;
}

int act_as_reflector(struct bt_conn* connection){
	int err;
	struct k_sem* sem_procedure_done = get_sem_procedure_done();
	while (true) {
		LOG_INF("sem_procedure_done before");
		k_sem_take(sem_procedure_done, K_FOREVER);
		LOG_INF("sem_procedure_done after");
		uint8_t* latest_local_steps = get_latest_local_steps();

		write_params.func = write_func;
		write_params.handle = step_data_attr_handle;
		write_params.length = STEP_DATA_BUF_LEN;
		write_params.data = latest_local_steps;
		write_params.offset = 0;

		err = bt_gatt_write(connection, &write_params);
		if (err) {
			LOG_ERR("Write failed (err %d)", err);
			return 0;
		}

		err = k_sem_take(&sem_written, K_SECONDS(10));
		if (err) {
			LOG_ERR("Timed out during GATT write");
			return 0;
		}
	}
}