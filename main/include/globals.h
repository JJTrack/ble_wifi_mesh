#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef struct
{
    int rssi;
    uint8_t uuid[6];
} ble_data_for_queue_t;


xQueueHandle ble_data_queue;


#endif /* __GLOBALS_H__ */