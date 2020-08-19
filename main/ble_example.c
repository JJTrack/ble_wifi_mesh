#include "ble_example.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

/*******************************************************
 *                Function Definitions
 *******************************************************/


static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_hs_adv_fields fields;

    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
        ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        printf("discovered device %.*s\n", fields.svc_data_uuid16_len, fields.svc_data_uuid16);
        ESP_LOGI("RSSI", "This is an rssi value: %d", event->disc.rssi );
        
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