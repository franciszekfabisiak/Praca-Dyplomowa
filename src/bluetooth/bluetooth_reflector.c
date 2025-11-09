#include "bluetooth_reflector.h"
#include "bluetooth_global.h"
#include "common.h"

static K_SEM_DEFINE(sem_written, 0, 1);
static K_SEM_DEFINE(sem_discovered, 0, 1);

static struct bt_uuid_128 step_data_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba8));

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_write_params write_params;
static uint16_t step_data_attr_handle;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	char str[BT_UUID_STR_LEN];

	printk("Discovery: attr %p\n", attr);

	if (!attr) {
		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	bt_uuid_to_str(chrc->uuid, str, sizeof(str));
	printk("UUID %s\n", str);

	if (!bt_uuid_cmp(chrc->uuid, &step_data_char_uuid.uuid)) {
		step_data_attr_handle = chrc->value_handle;

		printk("Found expected UUID\n");

		k_sem_give(&sem_discovered);
	}

	return BT_GATT_ITER_STOP;
}

static void write_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	printk("sem_written before\n");
	if (err) {
		printk("Write failed (err %d)\n", err);

		return;
	}

	k_sem_give(&sem_written);
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
		printk("sem_procedure_done before\n");
		k_sem_take(sem_procedure_done, K_FOREVER);
		printk("sem_procedure_done after\n");

		write_params.func = write_func;
		write_params.handle = step_data_attr_handle;
		write_params.length = STEP_DATA_BUF_LEN;
		write_params.data = &latest_local_steps[0];
		write_params.offset = 0;

		err = bt_gatt_write(connection, &write_params);
		if (err) {
			printk("Write failed (err %d)\n", err);
			return 0;
		}

		err = k_sem_take(&sem_written, K_SECONDS(10));
		if (err) {
			printk("Timed out during GATT write\n");
			return 0;
		}
	}
}