/* Mesh Internal Communication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "nvs_flash.h"




#include "wifi_mesh_example.h"
#include "ble_example.h"


/*******************************************************
 *                Macros
 *******************************************************/

 /*******************************************************
  *                Constants
  *******************************************************/




  /*******************************************************
   *                Variable Definitions
   *******************************************************/


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
        printf("discovered device %.*s\n", fields.name_len, fields.name);
        if (fields.name_len == strlen(DEVICE_NAME) && memcmp(fields.name, DEVICE_NAME, strlen(DEVICE_NAME)) == 0)
        {
            printf("device found\n");
            ble_gap_disc_cancel();
            ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 1000, NULL, ble_gap_event, NULL);
        }
        break;
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_CONNECT %s", event->connect.status == 0 ? "OK" : "Failed");
        if (event->connect.status == 0)
        {

        }
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

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    /*  tcpip initialization */
    tcpip_adapter_init();
    /* for mesh
     * stop DHCP server on softAP interface by default
     * stop DHCP client on station interface by default
     * */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));
    /*  event initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /*  wifi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* mesh ID */
    memcpy((uint8_t *)&cfg.mesh_id, MESH_ID, 6);
    /* router */
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *)&cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *)&cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
        strlen(CONFIG_MESH_ROUTER_PASSWD));
    /* mesh softAP */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *)&cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
        strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));


    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
    ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n", esp_get_free_heap_size(),
        esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");



    while (1) {
        vTaskDelay(10 * 1000 / portTICK_RATE_MS);
        esp_wifi_stop();
        vTaskDelay(10 * 1000 / portTICK_RATE_MS);
        esp_nimble_hci_and_controller_init();
        nimble_port_init();

        ble_svc_gap_device_name_set(DEVICE_NAME);
        ble_svc_gap_init();

        ble_hs_cfg.sync_cb = ble_app_on_sync;
        nimble_port_freertos_init(host_task);

        vTaskDelay(10 * 1000 / portTICK_RATE_MS);

        esp_wifi_restore();
    }

}
