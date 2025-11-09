#include "bluetooth_initiator.h"
#include "bluetooth_global.h"
#include "common.h"

int setup_initiator(struct bt_conn* connection){

	struct k_sem* sem_acl_encryption_enabled = get_sem_acl_encryption_enabled();
	struct k_sem* sem_remote_capabilities_obtained = get_sem_remote_capabilities_obtained();
	struct k_sem* sem_config_created = get_sem_config_created();
	struct k_sem* sem_cs_security_enabled = get_sem_cs_security_enabled();

	int err = bt_conn_set_security(connection, BT_SECURITY_L2);
	if (err) {
		printk("Failed to encrypt connection (err %d)\n", err);
		return err;
	}

	k_sem_take(sem_acl_encryption_enabled, K_FOREVER);

	err = bt_le_cs_read_remote_supported_capabilities(connection);
	if (err) {
		printk("Failed to exchange CS capabilities (err %d)\n", err);
		return err;
	}

	k_sem_take(sem_remote_capabilities_obtained, K_FOREVER);

	struct bt_le_cs_create_config_params config_params = {
		.id = CS_CONFIG_ID,
		.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_2,
		.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_1,
		.min_main_mode_steps = 2,
		.max_main_mode_steps = 10,
		.main_mode_repetition = 0,
		.mode_0_steps = NUM_MODE_0_STEPS,
		.role = BT_CONN_LE_CS_ROLE_INITIATOR,
		.rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY,
		.cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY,
		.channel_map_repetition = 1,
		.channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B,
		.ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT,
		.ch3c_jump = 2,
	};

	bt_le_cs_set_valid_chmap_bits(config_params.channel_map);

	err = bt_le_cs_create_config(connection, &config_params,
				     BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE);
	if (err) {
		printk("Failed to create CS config (err %d)\n", err);
		return err;
	}

	k_sem_take(sem_config_created, K_FOREVER);

	err = bt_le_cs_security_enable(connection);
	if (err) {
		printk("Failed to start CS Security (err %d)\n", err);
		return err;
	}

	k_sem_take(sem_cs_security_enabled, K_FOREVER);

	const struct bt_le_cs_set_procedure_parameters_param procedure_params = {
		.config_id = CS_CONFIG_ID,
		.max_procedure_len = 12,
		.min_procedure_interval = 100,
		.max_procedure_interval = 100,
		.max_procedure_count = 0,
		.min_subevent_len = 1250,//6750
		.max_subevent_len = 1250,//6750
		.tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_A1_B1,
		.phy = BT_LE_CS_PROCEDURE_PHY_1M,
		.tx_power_delta = 0x80,
		.preferred_peer_antenna = BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_1,
		.snr_control_initiator = BT_LE_CS_SNR_CONTROL_NOT_USED,
		.snr_control_reflector = BT_LE_CS_SNR_CONTROL_NOT_USED,
	};

	err = bt_le_cs_set_procedure_parameters(connection, &procedure_params);
	if (err) {
		printk("Failed to set procedure parameters (err %d)\n", err);
		return err;
	}

	struct bt_le_cs_procedure_enable_param params = {
		.config_id = CS_CONFIG_ID,
		.enable = 1,
	};

	err = bt_le_cs_procedure_enable(connection, &params);
	if (err) {
		printk("Failed to enable CS procedures (err %d)\n", err);
		return err;
	}

	return 0;
}

int act_as_initiator(void){

	struct k_sem* sem_procedure_done = get_sem_procedure_done();
	struct k_sem* sem_data_received = get_sem_data_received();

	while (true) {
		printk("before sem_done\n");
		k_sem_take(sem_procedure_done, K_FOREVER);
		printk("after sem_done\n");
		k_sem_take(sem_data_received, K_FOREVER);
		call_estimate_distance();
	}
	return 0;
}