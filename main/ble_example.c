#include "ble_example.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "globals.h"

/*******************************************************
 *                Function Definitions
 *******************************************************/

void add_val_to_queue(void * params) 
{
    long ok;

    ble_data_for_queue_t stuff = *(ble_data_for_queue_t *) params;
    ok = xQueueSend(ble_data_queue, &stuff, 1000 / portTICK_PERIOD_MS);

    if(ok) {
        ESP_LOGI("QUEUE", "ADDED UUID");
    } else {
        ESP_LOGI("QUEUE", "FAILED TO ADD UUID");
    }

    vTaskDelete(NULL);
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_hs_adv_fields fields;

    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
        ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (fields.name_len == strlen(DEVICE_NAME) && memcmp(fields.name, DEVICE_NAME, strlen(DEVICE_NAME)) == 0){
            ble_data_for_queue_t stuff;
            stuff.rssi = event->disc.rssi;
            memcpy(stuff.uuid, event->disc.addr.val, 6);
            xTaskCreate(&add_val_to_queue, "add_to_queue", 2048, &stuff, 2, NULL);

            ESP_LOGI("NAME", "This is the name: %.*s", fields.name_len, fields.name);
            ESP_LOGI("UUID", "This is the uuid: %02x:%02x:%02x:%02x:%02x:%02x",
            event->disc.addr.val[5] & 0xff, event->disc.addr.val[4] & 0xff, event->disc.addr.val[3] & 0xff,
            event->disc.addr.val[2] & 0xff, event->disc.addr.val[1] & 0xff, event->disc.addr.val[0] & 0xff);
            ESP_LOGI("RSSI", "This is an rssi value: %d\n", event->disc.rssi );
        }
        
        // ble_gap_disc_cancel();
        break;
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_CONNECT %s", event->connect.status == 0 ? "OK" : "Failed");
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_DISCONNECT");
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_ADV_COMPLETE");
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_SUBSCRIBE");
        break;
    default:
        break;
    }
    return 0;
}


void ble_app_scan(void)
{
    struct ble_gap_disc_params ble_gap_disc_params;
    ble_gap_disc_params.filter_duplicates = 1;
    ble_gap_disc_params.passive = 1;
    ble_gap_disc_params.itvl = 0;
    ble_gap_disc_params.window = 0;
    ble_gap_disc_params.filter_policy = 0;
    ble_gap_disc_params.limited = 0;
    ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &ble_gap_disc_params, ble_gap_event, NULL);
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_scan();
}

void host_task(void *param)
{
    nimble_port_run();
}