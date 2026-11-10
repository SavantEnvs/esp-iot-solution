#pragma once
#include <stdint.h>
#include "usb/usb_types_ch9.h"
typedef void *usb_device_handle_t;
typedef struct { uint8_t bLength; uint8_t bDescriptorType; uint16_t wTotalLength; uint8_t bNumInterfaces; uint8_t bConfigurationValue; } usb_config_desc_t;
typedef struct { uint8_t bLength; uint8_t bDescriptorType; uint8_t bInterfaceNumber; uint8_t bAlternateSetting; uint8_t bNumEndpoints; uint8_t bInterfaceClass; uint8_t bInterfaceSubClass; uint8_t bInterfaceProtocol; } usb_intf_desc_t;
typedef struct { uint8_t bLength; uint8_t bDescriptorType; uint16_t idVendor; uint16_t idProduct; } usb_device_desc_t;
