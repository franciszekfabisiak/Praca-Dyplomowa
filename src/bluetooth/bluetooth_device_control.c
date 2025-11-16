#include "bluetooth_global.h"
#include "bluetooth_device_control.h"
// declarations
static const struct bt_uuid_128 logic_svc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x23d1bcea, 0x5f78, 0x2315, 0xdeef, 0x121201000000));

static const struct bt_uuid_128 control_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x23d1bcea, 0x5f78, 0x2315, 0xdeef, 0x121201000001));

static const struct bt_uuid_128 logic_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x23d1bcea, 0x5f78, 0x2315, 0xdeef, 0x121201000002));

static K_SEM_DEFINE(sem_discovered, 0, 1);
static K_SEM_DEFINE(sem_written, 0, 1);

LOG_MODULE_REGISTER(control);

static uint16_t logic_attr_handle;

// private function definitions
static ssize_t choose_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static uint8_t discover_logic(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			struct bt_gatt_discover_params *params);

static void write_logic(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params);
//GATT structures
static struct bt_gatt_attr logic_gatt_attributes[] = {
	BT_GATT_PRIMARY_SERVICE(&logic_svc_uuid),
	BT_GATT_CHARACTERISTIC(&control_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, NULL,
			       choose_mode, NULL),
	BT_GATT_CHARACTERISTIC(&logic_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, NULL,
			       choose_mode, NULL),
};

static struct bt_gatt_discover_params logic_discover_params = {
    .uuid = &logic_char_uuid.uuid,
    .func = discover_logic,
	.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.type = BT_GATT_DISCOVER_CHARACTERISTIC
};

static struct bt_gatt_write_params logic_write_params = {
    .func = write_logic,
	.length = STEP_DATA_BUF_LEN
};

static struct bt_gatt_service logic_gatt_service = BT_GATT_SERVICE(logic_gatt_attributes);

// public functions
int register_logic_gatt_service(void)
{
    int err = bt_gatt_service_register(&logic_gatt_service);
	return err;
}

int discover_logic_gatt_service(struct bt_conn* conn)
{
    int err = bt_gatt_discover(conn, &logic_discover_params);
	if (err) {
		LOG_ERR("Discovery of logic gatt failed (err %d)", err);
		return err;
	}

    err = k_sem_take(&sem_discovered, K_SECONDS(10));
	if (err) {
		LOG_ERR("Timed out during GATT discovery");
		return err;
	}
	LOG_INF("After sem_discovered in logic");
    return 0;
}

int logic_gatt_write(struct bt_conn *conn, uint8_t data)
{
	uint8_t data2 = 111;
    logic_write_params.handle = logic_attr_handle;
    logic_write_params.data = &data2;
    logic_write_params.offset = 0;

    int err = bt_gatt_write(conn, &logic_write_params);
    if (err) {
        LOG_ERR("Write failed (err %d)", err);
        return err;
    }

    err = k_sem_take(&sem_written, K_SECONDS(10));
    if (err) {
        LOG_ERR("Timed out during GATT write for logic");
        return err;
    }
    return 0;
}

// private functions
static ssize_t choose_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    uint8_t data[250];
	LOG_INF("In write %d, offset %d, flags: %d", len, offset, flags);
    if (len >= sizeof(data))
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

    memcpy(data, buf, len);
    data[len] = '\0';

    LOG_INF("Received: %d", data[0]);

    return len;
}

static uint8_t discover_logic(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	char str[BT_UUID_STR_LEN];

	LOG_INF("Discovery: attr %p in logic", attr);

	if (!attr) {
		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	bt_uuid_to_str(chrc->uuid, str, sizeof(str));
	LOG_INF("UUID %s", str);

	if (!bt_uuid_cmp(chrc->uuid, &logic_char_uuid.uuid)) {
		logic_attr_handle = chrc->value_handle;

		LOG_INF("Found expected UUID");

		k_sem_give(&sem_discovered);
	}

	return BT_GATT_ITER_STOP;
}

static void write_logic(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	LOG_INF("sem_written before");
	if (err) {
		LOG_ERR("Write failed (err %d)", err);

		return;
	}

	k_sem_give(&sem_written);
}