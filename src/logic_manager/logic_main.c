#include "logic_main.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include "bluetooth_global.h"

static struct k_thread logic_thread_data;
static k_tid_t logic_thread_id;
static K_THREAD_STACK_DEFINE(logic_thread_stack, LOGIC_THREAD_STACK_SIZE);

static void logic_thread(void *, void *,void *);
static void start_adv_event(uint8_t value);

static logic_cb logic_map[MESSAGE_COUNT] =
    {
        [MESSAGE_NONE] = NULL,
        [MESSAGE_START_ADV] = start_adv_event
    };


K_MSGQ_DEFINE(logic_queue, sizeof(message_object), 10, 1);

LOG_MODULE_REGISTER(logic);

void logic_thread_init(void)
{

    logic_thread_id = k_thread_create(&logic_thread_data, logic_thread_stack, 
                                   K_THREAD_STACK_SIZEOF(logic_thread_stack), logic_thread, 
                                   NULL, NULL, NULL, LOGIC_THREAD_PRIORITY, K_ESSENTIAL, K_MSEC(500));
}

int send_message(enum message_type message, int value)
{
    message_object msg;
    msg.event = message;
    msg.value = value;

    int err = k_msgq_put(&logic_queue, &msg, K_NO_WAIT);
    if(err){
        LOG_ERR("msg sending error: %d", err);
        return err;
    }

    return 0;
}

static void logic_thread(void *, void *,void *)
{
    message_object msg;
    logic_cb cb;

    while (1)
    {
        LOG_INF("Got message, event %d, value %d", msg.event, msg.value);
        k_msgq_get(&logic_queue, &msg, K_FOREVER);
        cb = logic_map[msg.event];
        if (cb != NULL)
            cb(msg.value);
    }
}

static void start_adv_event(uint8_t value){
    int err = start_adv();
    if(err)
        LOG_INF("adv failed in msq queue %d", err);
}