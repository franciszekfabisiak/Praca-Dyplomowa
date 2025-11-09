#include "bluetooth_global.h"
#include "common.h"

static struct bt_uuid_128 step_data_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba8));
static const struct bt_uuid_128 step_data_svc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba9));
	
static const char sample_str[] = "CS Sample";
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "CS Sample", sizeof(sample_str) - 1),
};

static const struct bt_le_cs_set_default_settings_param default_settings_reflector = {
	.enable_initiator_role = false,
	.enable_reflector_role = true,
	.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
	.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
};

static const struct bt_le_cs_set_default_settings_param default_settings_initiator = {
	.enable_initiator_role = true,
	.enable_reflector_role = false,
	.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
	.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
};

static K_SEM_DEFINE(sem_acl_encryption_enabled, 0, 1);
static K_SEM_DEFINE(sem_remote_capabilities_obtained, 0, 1);
static K_SEM_DEFINE(sem_config_created, 0, 1);
static K_SEM_DEFINE(sem_cs_security_enabled, 0, 1);
static K_SEM_DEFINE(sem_procedure_done, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_data_received, 0, 1);

static struct bt_conn *connection;

// static declarations
static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params);

static void connected_cb(struct bt_conn *conn, uint8_t err);
static void disconnected_cb(struct bt_conn *conn, uint8_t reason);
static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);
static void remote_capabilities_cb(struct bt_conn *conn,
				   uint8_t status,
				   struct bt_conn_le_cs_capabilities *params);

static void config_create_cb(struct bt_conn *conn,
			     uint8_t status,
			     struct bt_conn_le_cs_config *config);

static void security_enable_cb(struct bt_conn *conn, uint8_t status);
static void procedure_enable_cb(struct bt_conn *conn,
				 uint8_t status,
				 struct bt_conn_le_cs_procedure_enable_complete *params);

static void subevent_result_cb(struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result);

// other declarations
static struct bt_gatt_attr gatt_attributes[] = {
	BT_GATT_PRIMARY_SERVICE(&step_data_svc_uuid),
	BT_GATT_CHARACTERISTIC(&step_data_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, NULL,
			       on_attr_write_cb, NULL),
};
static struct bt_gatt_service step_data_gatt_service = BT_GATT_SERVICE(gatt_attributes);

// private variables
static uint8_t n_ap;
static uint8_t latest_num_steps_reported;
static uint16_t latest_step_data_len;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];
static uint8_t latest_peer_steps[STEP_DATA_BUF_LEN];

// public functions
int register_step_data_gatt_service(void){
    int err = bt_gatt_service_register(&step_data_gatt_service);
	return err;
}

struct bt_conn* get_bt_connection(void){
    if (connection) {
        return bt_conn_ref(connection);
    }
    return NULL;
}

struct k_sem* get_sem_acl_encryption_enabled(void)
{
    return &sem_acl_encryption_enabled;
}

struct k_sem* get_sem_remote_capabilities_obtained(void)
{
    return &sem_remote_capabilities_obtained;
}

struct k_sem* get_sem_config_created(void)
{
    return &sem_config_created;
}

struct k_sem* get_sem_cs_security_enabled(void)
{
    return &sem_cs_security_enabled;
}

struct k_sem* get_sem_procedure_done(void)
{
    return &sem_procedure_done;
}

struct k_sem* get_sem_connected(void)
{
    return &sem_connected;
}

struct k_sem* get_sem_data_received(void)
{
    return &sem_data_received;
}

int ble_init(void){
    
    /* Initialize the Bluetooth Subsystem */
	int err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
					      BT_GAP_ADV_FAST_INT_MAX_1, NULL),
			      ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}
    return 0;
}

void call_estimate_distance(void)
{
	printk("estimating distance\n");
    estimate_distance(
        latest_local_steps, latest_step_data_len, latest_peer_steps,
        latest_step_data_len -
            NUM_MODE_0_STEPS *
                (sizeof(struct bt_hci_le_cs_step_data_mode_0_initiator) -
                    sizeof(struct bt_hci_le_cs_step_data_mode_0_reflector)),
        n_ap, BT_CONN_LE_CS_ROLE_INITIATOR);
}

int get_bt_le_cs_default_settings(bool is_reflector, struct bt_conn *connection)
{
	if (is_reflector){
		return bt_le_cs_set_default_settings(connection, &default_settings_reflector);
	}
	else{
		return bt_le_cs_set_default_settings(connection, &default_settings_initiator);
	}
}

// private functions
static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	printk("flag %d\n", flags);
	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(latest_local_steps)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (flags & BT_GATT_WRITE_FLAG_EXECUTE) {
		uint8_t *data = (uint8_t *)buf;

		memcpy(latest_peer_steps, &data[offset], len);
		k_sem_give(&sem_data_received);
	}

	return len;
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	printk("MTU exchange %s (%u)\n", err == 0U ? "success" : "failed", bt_gatt_get_mtu(conn));
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected to %s (err 0x%02X)\n", addr, err);

	__ASSERT(connection == conn, "Unexpected connected callback");

	if (err) {
		bt_conn_unref(conn);
		connection = NULL;
	}

    // add a check for this
	connection = bt_conn_ref(conn);

	static struct bt_gatt_exchange_params mtu_exchange_params = {.func = mtu_exchange_cb};

	err = bt_gatt_exchange_mtu(connection, &mtu_exchange_params);
	if (err) {
		printk("%s: MTU exchange failed (err %d)\n", __func__, err);
	}

	k_sem_give(&sem_connected);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02X)\n", reason);

	bt_conn_unref(conn);
	connection = NULL;
}

static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err) {
		printk("Encryption failed. (err %d)\n", err);
	} else {
		printk("Security changed to level %d.\n", level);
	}

	k_sem_give(&sem_acl_encryption_enabled);
}

static void remote_capabilities_cb(struct bt_conn *conn,
				   uint8_t status,
				   struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(params);

	if (status == BT_HCI_ERR_SUCCESS) {
		printk("CS capability exchange completed.\n");
		k_sem_give(&sem_remote_capabilities_obtained);
	} else {
		printk("CS capability exchange failed. (HCI status 0x%02x)\n", status);
	}
}

static void config_create_cb(struct bt_conn *conn,
			     uint8_t status,
			     struct bt_conn_le_cs_config *config)
{
	if (status == BT_HCI_ERR_SUCCESS) {
		printk("CS config creation complete. ID: %d\n", config->id);
		k_sem_give(&sem_config_created);
	} else {
		printk("CS config creation failed. (HCI status 0x%02x)\n", status);
	}
}

static void security_enable_cb(struct bt_conn *conn, uint8_t status)
{
	if (status == BT_HCI_ERR_SUCCESS) {
		printk("CS security enabled.\n");
		k_sem_give(&sem_cs_security_enabled);
	} else {
		printk("CS security enable failed. (HCI status 0x%02x)\n", status);
	}
}

static void procedure_enable_cb(struct bt_conn *conn,
				 uint8_t status,
				 struct bt_conn_le_cs_procedure_enable_complete *params)
{
	if (status == BT_HCI_ERR_SUCCESS) {
		if (params->state == 1) {
			printk("CS procedures enabled.\n");
		} else {
			printk("CS procedures disabled.\n");
		}
	} else {
		printk("CS procedures enable failed. (HCI status 0x%02x)\n", status);
	}
}

static void subevent_result_cb(struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result)
{
	printk("abort? %d\n", result->header.abort_step);
	printk("steps %d\n", result->header.num_steps_reported);
	printk("paths %d\n", result->header.num_antenna_paths);
	
	latest_num_steps_reported = result->header.num_steps_reported;
	n_ap = result->header.num_antenna_paths;
	if (result->step_data_buf) {
		if (result->step_data_buf->len <= STEP_DATA_BUF_LEN) {
			memcpy(latest_local_steps, result->step_data_buf->data,
			       result->step_data_buf->len);
			latest_step_data_len = result->step_data_buf->len;
		} else {
			printk("Not enough memory to store step data. (%d > %d)\n",
			       result->step_data_buf->len, STEP_DATA_BUF_LEN);
			latest_num_steps_reported = 0;
		}
	}
	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE) {
		k_sem_give(&sem_procedure_done);
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,
	.le_cs_read_remote_capabilities_complete = remote_capabilities_cb,
	.le_cs_config_complete = config_create_cb,
	.le_cs_security_enable_complete = security_enable_cb,
	.le_cs_procedure_enable_complete = procedure_enable_cb,
	.le_cs_subevent_data_available = subevent_result_cb,
};