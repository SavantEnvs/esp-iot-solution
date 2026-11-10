/* Known-answer oracle for the RNDIS driver (run by test.sh, built with NORMAL flags): enumerate the device
 * whose response stream is the file argument and print what the driver reports. */
#include <stdio.h>
#include <stdlib.h>
#include "rndis_env.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <responses>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror(argv[1]);
        return 2;
    }
    static uint8_t buf[1 << 16];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    rndis_report_t rep;
    rndis_session(buf, n, &rep);
    printf("link_events=%d link_up=%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n", rep.link_events, rep.link_up,
           rep.mac[0], rep.mac[1], rep.mac[2], rep.mac[3], rep.mac[4], rep.mac[5]);
    return 0;
}
