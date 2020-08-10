#ifndef __BLE_EXAMPLE_H__
#define __BLE_EXAMPLE_H__

#include "esp_err.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#define DEVICE_NAME "MY BLE DEVICE"
uint8_t ble_addr_type;
void ble_app_scan(void);

static int ble_gap_event(struct ble_gap_event *event, void *arg);
void ble_app_scan(void);
void ble_app_on_sync(void);
void host_task(void *param);

#endif /* __BLE_EXAMPLE_H__ */
