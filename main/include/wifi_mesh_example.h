#ifndef __WIFI_MESH_EXAMPLE_H__
#define __WIFI_MESH_EXAMPLE_H__

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"


/*******************************************************
 *                Constants
 *******************************************************/

#define RX_SIZE          (1500)
#define TX_SIZE          (1460)

 /*******************************************************
  *                Variable Definitions
  *******************************************************/
static const char *MESH_TAG = "mesh_main";
static const uint8_t NODE_ID[6] ={ 1, 0, 0, 0, 0, 0 };
static const uint8_t MESH_ID[6] ={ 0x77, 0x77, 0x77, 0x77, 0x77, 0x77 };
static uint8_t tx_buf[TX_SIZE] ={ 0, };
static uint8_t rx_buf[RX_SIZE] ={ 0, };
static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;

void esp_mesh_p2p_tx_main(void *arg);
void esp_mesh_p2p_rx_main(void *arg);
esp_err_t esp_mesh_comm_p2p_start(void);
void mesh_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data);
void ip_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data);

#endif /* __WIFI_MESH_EXAMPLE_H__ */