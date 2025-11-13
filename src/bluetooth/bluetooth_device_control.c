#include "bluetooth_global.h"

// declarations
static const struct bt_uuid_128 logic_svc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x23d1bcea, 0x5f78, 0x2315, 0xdeef, 0x121201000000));

static const struct bt_uuid_128 control_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x23d1bcea, 0x5f78, 0x2315, 0xdeef, 0x121201000001));

static const struct bt_uuid_128 logic_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x23d1bcea, 0x5f78, 0x2315, 0xdeef, 0x121201000002));

static struct bt_gatt_discover_params control_discover_params = {
    .uuid = &control_char_uuid.uuid,
    .func = NULL,
	.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.type = BT_GATT_DISCOVER_CHARACTERISTIC
};

// private function definitions
static ssize_t choose_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

//GATT structures
static struct bt_gatt_attr logic_gatt_attributes[] = {
	BT_GATT_PRIMARY_SERVICE(&logic_svc_uuid),
	BT_GATT_CHARACTERISTIC(&control_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, NULL,
			       choose_mode, NULL),
	BT_GATT_CHARACTERISTIC(&logic_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, NULL,
			       NULL, NULL),
};
static struct bt_gatt_service logic_gatt_service = BT_GATT_SERVICE(logic_gatt_attributes);


// public functions
struct bt_gatt_discover_params* get_control_discover_params(void)
{
	return &control_discover_params;
}

int register_logic_gatt_service(void)
{
    int err = bt_gatt_service_register(&logic_gatt_service);
	return err;
}

// private functions
static ssize_t choose_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    char data[32];

    if (len >= sizeof(data))
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

    memcpy(data, buf, len);
    data[len] = '\0';

    printk("Received: %s\n", data);

    return len;
}


