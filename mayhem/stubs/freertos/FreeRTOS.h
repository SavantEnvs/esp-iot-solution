#pragma once
#include <stdint.h>
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
#define pdFALSE 0
#define pdTRUE  1
#define pdPASS  1
#define portMAX_DELAY 0xffffffffU
#define configTICK_RATE_HZ 100
#define pdMS_TO_TICKS(ms) ((TickType_t)(((uint64_t)(ms) * configTICK_RATE_HZ) / 1000))
