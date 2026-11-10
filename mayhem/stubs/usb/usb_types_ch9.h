#pragma once
#include <stdint.h>
#define USB_DESC_ATTR __attribute__((packed))
#ifndef ESP_STATIC_ASSERT
#define ESP_STATIC_ASSERT _Static_assert
#endif
#define USB_BM_REQUEST_TYPE_DIR_OUT        (0X00u << 7)
#define USB_BM_REQUEST_TYPE_DIR_IN         (0X01u << 7)
#define USB_BM_REQUEST_TYPE_TYPE_STANDARD  (0X00u << 5)
#define USB_BM_REQUEST_TYPE_TYPE_CLASS     (0X01u << 5)
#define USB_BM_REQUEST_TYPE_TYPE_VENDOR    (0X02u << 5)
#define USB_BM_REQUEST_TYPE_RECIP_DEVICE    (0X00u << 0)
#define USB_BM_REQUEST_TYPE_RECIP_INTERFACE (0X01u << 0)
#define USB_BM_REQUEST_TYPE_RECIP_ENDPOINT  (0X02u << 0)
