#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    int link_events;      /* IOT_ETH_STAGE_LINK notifications the driver sent */
    int link_up;          /* link state of the last one (-1: none) */
    uint8_t mac[6];       /* iot_eth_driver_t.get_addr() after the session */
} rndis_report_t;

/* Run one USB device session against the real iot_usbh_rndis driver: bring the driver up, plug in an
 * RNDIS device whose control-endpoint responses are the records of `data` (each a u16le length followed by
 * that many bytes, taken in order by GET_ENCAPSULATED_RESPONSE requests), let the driver's task enumerate
 * it, and shut the driver down. */
void rndis_session(const uint8_t *data, size_t size, rndis_report_t *rep);
