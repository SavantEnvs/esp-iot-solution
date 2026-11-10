/* Host stand-in for the ESP-IDF headers the RNDIS driver includes. Only what iot_usbh_rndis.c uses. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

typedef int esp_err_t;
#define ESP_OK                  0
#define ESP_FAIL                (-1)
#define ESP_ERR_NO_MEM          0x101
#define ESP_ERR_INVALID_ARG     0x102
#define ESP_ERR_INVALID_STATE   0x103
#define ESP_ERR_INVALID_SIZE    0x104
#define ESP_ERR_NOT_FOUND       0x105
#define ESP_ERR_NOT_SUPPORTED   0x106
#define ESP_ERR_TIMEOUT         0x107
#define ESP_ERR_INVALID_RESPONSE 0x108

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif
#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef __containerof
#define __containerof(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

const char *esp_err_to_name(esp_err_t code);
