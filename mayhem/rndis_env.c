/* Host environment for components/usb/iot_usbh_rndis: the ESP-IDF USB host, CDC and FreeRTOS calls the driver
 * makes are replaced by an in-memory USB device (mayhem/stubs/ holds the headers). The driver itself is
 * compiled unchanged and is driven only through its public iot_eth_driver_t interface plus the callbacks it
 * registers with the CDC host layer, exactly as the CDC host layer and the FreeRTOS scheduler would. */
#include <string.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "iot_usbh_rndis.h"
#include "iot_usbh_cdc.h"
#include "iot_eth_interface.h"
#include "iot_eth_types.h"
#include "iot_usbh_rndis_descriptor.h"
#include "rndis_env.h"

#ifndef CONFIG_USBH_CDC_CONTROL_TRANSFER_BUFFER_SIZE
#error "build with -DCONFIG_USBH_CDC_CONTROL_TRANSFER_BUFFER_SIZE=<bytes>"
#endif

#define RNDIS_STOP_BIT (1u << 4) /* the driver's private RNDIS_STOP */

/* ---- the simulated device -------------------------------------------------------------------------- */
static const uint8_t *g_in;
static size_t g_in_len, g_in_pos;
static rndis_report_t *g_rep;

static usbh_cdc_device_event_callback_t g_dev_cb;
static void *g_dev_ctx;
static TaskFunction_t g_task_fn;
static void *g_task_arg;
static struct usbh_cdc_port_t { int unused; } g_port;

const char *esp_err_to_name(esp_err_t code)
{
    return code == ESP_OK ? "ESP_OK" : "ESP_ERR";
}

esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type)
{
    (void)type;
    static const uint8_t base[6] = {0x24, 0x0a, 0xc4, 0x00, 0x00, 0x10};
    memcpy(mac, base, 6);
    return ESP_OK;
}

esp_err_t usbh_rndis_interface_check(const usb_device_desc_t *device_desc, const usb_config_desc_t *config_desc, int *itf_num)
{
    (void)device_desc;
    (void)config_desc;
    *itf_num = 0;
    return ESP_OK;
}

/* ---- FreeRTOS: single-threaded, the driver task is run by the session ----------------------------- */
struct EventGroupDef_t { EventBits_t bits; };

EventGroupHandle_t xEventGroupCreate(void)
{
    return calloc(1, sizeof(struct EventGroupDef_t));
}

void vEventGroupDelete(EventGroupHandle_t g)
{
    free(g);
}

EventBits_t xEventGroupSetBits(EventGroupHandle_t g, EventBits_t bits)
{
    g->bits |= bits;
    return g->bits;
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t g, EventBits_t bits, BaseType_t clear, BaseType_t all, TickType_t ticks)
{
    (void)all;
    (void)ticks;
    EventBits_t got = g->bits & bits;
    if (got == 0 && (bits & RNDIS_STOP_BIT)) {
        got = RNDIS_STOP_BIT; /* nothing left to deliver: the scheduler's next event is the driver's stop request */
    }
    if (clear) {
        g->bits &= ~got;
    }
    return got;
}

BaseType_t xTaskCreate(TaskFunction_t fn, const char *name, uint32_t stack, void *arg, UBaseType_t prio, TaskHandle_t *out)
{
    (void)name; (void)stack; (void)prio;
    g_task_fn = fn;
    g_task_arg = arg;
    if (out) {
        *out = NULL;
    }
    return pdPASS;
}

void vTaskDelete(TaskHandle_t t) { (void)t; }
void vTaskDelay(TickType_t ticks) { (void)ticks; }

/* ---- CDC host layer ------------------------------------------------------------------------------ */
esp_err_t usbh_cdc_register_dev_event_cb(const usb_device_match_id_t *list, usbh_cdc_device_event_callback_t cb, void *user)
{
    (void)list;
    g_dev_cb = cb;
    g_dev_ctx = user;
    return ESP_OK;
}

esp_err_t usbh_cdc_unregister_dev_event_cb(usbh_cdc_device_event_callback_t cb)
{
    (void)cb;
    g_dev_cb = NULL;
    return ESP_OK;
}

esp_err_t usbh_cdc_port_open(const usbh_cdc_port_config_t *cfg, usbh_cdc_port_handle_t *out)
{
    (void)cfg;
    *out = &g_port;
    return ESP_OK;
}

esp_err_t usbh_cdc_send_custom_request(usbh_cdc_port_handle_t port, uint8_t bmRequestType, uint8_t bRequest,
                                       uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t *data)
{
    (void)port; (void)bRequest; (void)wValue; (void)wIndex;
    if (wLength > 0 && data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    /* the CDC layer's control transfer buffer bounds wLength */
    if (wLength > CONFIG_USBH_CDC_CONTROL_TRANSFER_BUFFER_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!(bmRequestType & USB_BM_REQUEST_TYPE_DIR_IN)) {
        return ESP_OK; /* the device accepts every command */
    }
    if (g_in_pos + 2 > g_in_len) {
        return ESP_ERR_INVALID_RESPONSE; /* the device has nothing more to say */
    }
    size_t n = g_in[g_in_pos] | (g_in[g_in_pos + 1] << 8);
    g_in_pos += 2;
    if (n > g_in_len - g_in_pos) {
        n = g_in_len - g_in_pos;
    }
    size_t copy = n < wLength ? n : wLength; /* the host copies at most wLength bytes */
    memset(data, 0, wLength);
    memcpy(data, g_in + g_in_pos, copy);
    g_in_pos += n;
    return ESP_OK;
}

esp_err_t usbh_cdc_write_bytes(usbh_cdc_port_handle_t p, const uint8_t *buf, size_t len, TickType_t t) { (void)p; (void)buf; (void)len; (void)t; return ESP_OK; }
esp_err_t usbh_cdc_read_bytes(usbh_cdc_port_handle_t p, uint8_t *buf, size_t *len, TickType_t t) { (void)p; (void)buf; (void)t; *len = 0; return ESP_OK; }
esp_err_t usbh_cdc_flush_rx_buffer(usbh_cdc_port_handle_t p) { (void)p; return ESP_OK; }
esp_err_t usbh_cdc_get_rx_buffer_size(usbh_cdc_port_handle_t p, size_t *size) { (void)p; *size = 0; return ESP_OK; }

/* ---- the session ----------------------------------------------------------------------------------- */
static esp_err_t med_stack_input(iot_eth_mediator_t *m, uint8_t *data, size_t len)
{
    (void)m; (void)data; (void)len;
    return ESP_OK;
}

static esp_err_t med_on_stage_changed(iot_eth_mediator_t *m, iot_eth_stage_t stage, void *arg)
{
    (void)m;
    if (stage == IOT_ETH_STAGE_LINK && arg != NULL) {
        g_rep->link_events++;
        g_rep->link_up = (*(iot_eth_link_t *)arg == IOT_ETH_LINK_UP);
    }
    return ESP_OK;
}

void rndis_session(const uint8_t *data, size_t size, rndis_report_t *rep)
{
    memset(rep, 0, sizeof(*rep));
    rep->link_up = -1;
    g_rep = rep;
    g_in = data;
    g_in_len = size;
    g_in_pos = 0;
    g_dev_cb = NULL;
    g_task_fn = NULL;

    iot_usbh_rndis_config_t cfg = {.match_id_list = NULL};
    iot_eth_driver_t *drv = NULL;
    iot_eth_mediator_t mediator = {.stack_input = med_stack_input, .on_stage_changed = med_on_stage_changed};
    if (iot_eth_new_usb_rndis(&cfg, &drv) != ESP_OK) {
        return;
    }
    drv->set_mediator(drv, &mediator);
    if (drv->init(drv) != ESP_OK || g_dev_cb == NULL || g_task_fn == NULL) {
        return;
    }

    usb_device_desc_t dev_desc = {.bLength = 18, .bDescriptorType = 1, .idVendor = 0x12d1, .idProduct = 0x1506};
    usb_config_desc_t cfg_desc = {.bLength = 9, .bDescriptorType = 2, .wTotalLength = 9};
    usbh_cdc_device_event_data_t ev = {.new_dev = {.dev_addr = 1, .matched_intf_num = 0,
                                                   .device_desc = &dev_desc, .active_config_desc = &cfg_desc}};
    g_dev_cb(CDC_HOST_DEVICE_EVENT_CONNECTED, &ev, g_dev_ctx);

    g_task_fn(g_task_arg);

    drv->get_addr(drv, rep->mac);
    drv->deinit(drv);
}
