/* libFuzzer harness for the ELF image validator, over both entry points (whole buffer and
 * on-demand source). The image is copied into an exact-size heap block so ASan sees any read
 * the validator makes past the end. Built once per target architecture (see build.sh). */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "private/elf_validate.h"
#include "mem_source.h"

#define ELF32_EHDR_SIZE 52

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < ELF32_EHDR_SIZE) {
        return -1;
    }

    uint8_t *image = malloc(size);
    if (!image) {
        return -1;
    }
    memcpy(image, data, size);

    int rc_buf = esp_elf_validate(image, size);

    mem_image_t img = { .buf = image, .size = size };
    elf_source_t src = mem_source(&img);
    int rc_src = esp_elf_validate_source(&src);

    free(image);
    (void)rc_buf;
    (void)rc_src;
    return 0;
}
