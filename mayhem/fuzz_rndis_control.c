/* Harness for the RNDIS USB host driver (components/usb/iot_usbh_rndis): the input is the stream of control
 * responses an attached USB device returns while the driver enumerates it (see rndis_env.h). */
#include <stdint.h>
#include <stddef.h>
#include "rndis_env.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    rndis_report_t rep;
    rndis_session(data, size, &rep);
    return 0;
}
