#pragma once
#include "FreeRTOS.h"
typedef struct EventGroupDef_t *EventGroupHandle_t;
typedef uint32_t EventBits_t;
EventGroupHandle_t xEventGroupCreate(void);
void vEventGroupDelete(EventGroupHandle_t group);
EventBits_t xEventGroupSetBits(EventGroupHandle_t group, EventBits_t bits);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t group, EventBits_t bits, BaseType_t clear, BaseType_t all, TickType_t ticks);
