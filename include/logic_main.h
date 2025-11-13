#ifndef LOGIC_MAIN_H
#define LOGIC_MAIN_H

#include <zephyr/sys/byteorder.h>

#define LOGIC_THREAD_PRIORITY 13
#define LOGIC_THREAD_STACK_SIZE 8192

enum message_type
{
    MESSAGE_NONE = 0,
    MESSAGE_START_ADV,
    MESSAGE_COUNT

};

typedef struct
{
    uint8_t event;
    uint8_t value;
} __attribute__((aligned(2))) message_object;

typedef void (*logic_cb)(uint8_t value);

void logic_thread_init(void);
int send_message(enum message_type message, int value);

#endif
