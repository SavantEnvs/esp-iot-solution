/* In-memory elf_source_t shared by the harnesses and the oracle: every read is bounds-checked
 * against the image size, as a file- or flash-backed source would fail past the end. */
#pragma once
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_elf.h"

typedef struct {
    const uint8_t *buf;
    size_t size;
} mem_image_t;

static size_t mem_source_size(void *ctx)
{
    return ((mem_image_t *)ctx)->size;
}

static int mem_source_read(void *ctx, size_t off, void *dst, size_t len)
{
    const mem_image_t *img = ctx;
    if (off > img->size || len > img->size - off) {
        return -EINVAL;
    }
    memcpy(dst, img->buf + off, len);
    return 0;
}

static elf_source_t mem_source(mem_image_t *img)
{
    return (elf_source_t) {
        .size = mem_source_size,
        .read = mem_source_read,
        .close = NULL,
        .ctx = img,
    };
}
