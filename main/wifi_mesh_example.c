#include <string.h>
#include "wifi_mesh_example.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "globals.h"

static void tcp_client_task(void *pvParameters)
{

    ESP_LOGE("TCP", "STARTING TCP");
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        ESP_LOGE("IP Version", "Connected using IPV6");
        struct sockaddr_in6 dest_addr = { 0 };
        inet6_aton(HOST_IP_ADDR, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        // Setting scope_id to the connecting interface for correct routing if IPv6 Local Link supplied
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TCP, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TCP, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TCP, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TCP, "Successfully connected");

        while (1) {
            if (err < 0) {
                ESP_LOGE(TCP, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TCP, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TCP, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}


void esp_mesh_p2p_tx_main(void *arg)
{
    extern xQueueHandle ble_data_queue;
    uint8_t NodeUUID[6] = {123, 231, 213, 90, 88, 33}; 
    uint8_t PacketID[8] = {5, 2, 12, 4, 5, 6, 42, 88};
    uint8_t dataPacket[21];
    esp_err_t err;
    int send_count = 0;
    mesh_data_t data;
    data.size = TCP_PACKET_SIZE;
    data.proto = MESH_PROTO_BIN;
    data.tos = MESH_TOS_P2P;
    is_running = true;

    while (is_running) {
        /* non-root do nothing but print */
        if (!esp_mesh_is_root()) {
            ESP_LOGI("ROOT", "This is not the root node");
            vTaskDelay(10 * 1000 / portTICK_RATE_MS);
        }

        if (esp_mesh_is_root()) {
            ESP_LOGI("ROOT", "THIS IS THE ROOT NODE");
            vTaskDelay(10 * 1000 / portTICK_RATE_MS);
            continue;
        }

        ble_data_for_queue_t tx_uuid;

        if(xQueueReceive(ble_data_queue, &tx_uuid, 1000 / portTICK_PERIOD_MS)) {
            ESP_LOGE("QUEUE", "This is the tx_uuid: %02x:%02x:%02x:%02x:%02x:%02x",
            tx_uuid.uuid[5] & 0xff, tx_uuid.uuid[4] & 0xff, tx_uuid.uuid[3] & 0xff,
            tx_uuid.uuid[2] & 0xff, tx_uuid.uuid[1] & 0xff, tx_uuid.uuid[0] & 0xff);

            ESP_LOGE("QUEUE", "This is the tx_rssi: %d", tx_uuid.rssi);

            for (int i = 0; i<21; i++) 
            {
                if (i < 1) {
                    dataPacket[i] = (uint8_t) (tx_uuid.rssi*-1);
                }
                else if (i < 7) {
                    dataPacket[i] = tx_uuid.uuid[i-1];
                }
                else if (i < 13) {
                    dataPacket[i] = NodeUUID[i-7];
                }
                else if (i < 21) {
                    dataPacket[i] = PacketID[i-13];
                }
            }

            data.data = dataPacket;

            send_count++;
            tx_buf[25] = (send_count >> 24) & 0xff;
            tx_buf[24] = (send_count >> 16) & 0xff;
            tx_buf[23] = (send_count >> 8) & 0xff;
            tx_buf[22] = (send_count >> 0) & 0xff;

            err = esp_mesh_send(NULL, &data, 0, NULL, 0);

            if(err) {
                ESP_LOGE("SENDING ERROR", "CANNOT SEND DATA PACKET");
            } else {
                ESP_LOGE("SENDING SUCCESS", "DATA PACKET SENT");
                ESP_LOGE("SENDING", "This is the rx_uuid: %02x:%02x:%02x:%02x:%02x:%02x",
                data.data[6] & 0xff, data.data[5] & 0xff, data.data[4] & 0xff,
                data.data[3] & 0xff, data.data[2] & 0xff, data.data[1] & 0xff);
            }
        }


    }
    vTaskDelete(NULL);
}

void esp_mesh_p2p_rx_main(void *arg)
{
    int recv_count = 0;
    esp_err_t err;
    mesh_addr_t from;
    int send_count = 0;
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;
    is_running = true;

    while (is_running) {
        data.size = RX_SIZE;
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || !data.size) {
            ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }

        int error = send(sock, data.data, data.size, 0);

        ESP_LOGE("RECEIVE", "This is the rx_nodeuuid: %02x:%02x:%02x:%02x:%02x:%02x",
        data.data[7] & 0xff, data.data[8] & 0xff, data.data[9] & 0xff,
        data.data[10] & 0xff, data.data[11] & 0xff, data.data[12] & 0xff);

        ESP_LOGE("RECEIVE", "This is the rx_uuid: %02x:%02x:%02x:%02x:%02x:%02x",
        data.data[6] & 0xff, data.data[5] & 0xff, data.data[4] & 0xff,
        data.data[3] & 0xff, data.data[2] & 0xff, data.data[1] & 0xff);

        ESP_LOGE("RECEIVE", "This is the rx_rssi: %d", data.data[0]);
    }
    
    vTaskDelete(NULL);
}

esp_err_t esp_mesh_comm_p2p_start(void)
{
    static bool is_comm_p2p_started = false;
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;

        xTaskCreate(esp_mesh_p2p_tx_main, "MPTX", 3072, NULL, 5, NULL);
        xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
        
    }
    return ESP_OK;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data)
{
    mesh_addr_t id ={ 0, };
    static uint8_t last_layer = 0;

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    }
                           break;
    case MESH_EVENT_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    }
                           break;
    case MESH_EVENT_CHILD_CONNECTED: {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
            child_connected->aid,
            MAC2STR(child_connected->mac));
    }
                                   break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
            child_disconnected->aid,
            MAC2STR(child_disconnected->mac));
    }
                                      break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
            routing_table->rt_size_change,
            routing_table->rt_size_new);
    }
                                     break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
            routing_table->rt_size_change,
            routing_table->rt_size_new);
    }
                                        break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
            no_parent->scan_times);
    }
                                   /* TODO handler for the failure */
                                   break;
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
            last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
            esp_mesh_is_root() ? "<ROOT>" :
            (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        is_mesh_connected = true;
        if (esp_mesh_is_root()) {
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        }
        esp_mesh_comm_p2p_start();
    }
                                    break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
            disconnected->reason);
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    }
                                       break;
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        mesh_layer = layer_change->new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
            last_layer, mesh_layer,
            esp_mesh_is_root() ? "<ROOT>" :
            (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
    }
                                break;
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
            MAC2STR(root_addr->addr));
    }
                                break;
    case MESH_EVENT_VOTE_STARTED: {
        mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
            vote_started->attempts,
            vote_started->reason,
            MAC2STR(vote_started->rc_addr.addr));
    }
                                break;
    case MESH_EVENT_VOTE_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ: {
        mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
            switch_req->reason,
            MAC2STR(switch_req->rc_addr.addr));
    }
                                   break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    }
                                   break;
    case MESH_EVENT_TODS_STATE: {
        mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
                              break;
    case MESH_EVENT_ROOT_FIXED: {
        mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
            root_fixed->is_fixed ? "fixed" : "not fixed");
    }
                              break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
        mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
            MAC2STR(root_conflict->addr),
            root_conflict->rssi,
            root_conflict->capacity);
    }
                                    break;
    case MESH_EVENT_CHANNEL_SWITCH: {
        mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    }
                                  break;
    case MESH_EVENT_SCAN_DONE: {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
            scan_done->number);
    }
                             break;
    case MESH_EVENT_NETWORK_STATE: {
        mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
            network_state->is_rootless);
    }
                                 break;
    case MESH_EVENT_STOP_RECONNECTION: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
                                     break;
    case MESH_EVENT_FIND_NETWORK: {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
            find_network->channel, MAC2STR(find_network->router_bssid));
    }
                                break;
    case MESH_EVENT_ROUTER_SWITCH: {
        mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
            router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    }
                                 break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%d", event_id);
        break;
    }
}

void ip_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:%s", ip4addr_ntoa(&event->ip_info.ip));

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}