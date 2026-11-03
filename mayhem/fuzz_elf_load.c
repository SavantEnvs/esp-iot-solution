/* libFuzzer harness for the ELF loader: validate + load + relocate a mutated image through both
 * the whole-buffer and the streaming-source entry points, then tear it down. The entry point is
 * never run and esp_elf_request() is never called, so no image code executes. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_elf.h"
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

    esp_elf_t elf;
    if (esp_elf_init(&elf) == 0) {
        (void)esp_elf_relocate(&elf, image, size);
        esp_elf_deinit(&elf);
    }

    mem_image_t img = { .buf = image, .size = size };
    elf_source_t src = mem_source(&img);
    if (esp_elf_init(&elf) == 0) {
        (void)esp_elf_relocate_source(&elf, &src);
        esp_elf_deinit(&elf);
    }

    free(image);
    return 0;
}
