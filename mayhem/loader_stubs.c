/* Host stand-ins for the ESP-IDF platform hooks the ELF loader links against. */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "private/elf_platform.h"
#include "private/elf_symbol.h"

/* A device heap is small; refuse huge requests the way heap_caps_malloc() would
 * instead of letting a 4 GiB memsz from a mutated header exhaust the fuzzer host. */
#define STUB_HEAP_LIMIT (16u * 1024u * 1024u)

void *esp_elf_malloc(uint32_t bytes, bool executable)
{
    (void)executable;
    if (bytes > STUB_HEAP_LIMIT) {
        return NULL;
    }
    return malloc(bytes);
}

void esp_elf_free(void *ptr)
{
    free(ptr);
}

/* Resolve every imported name to a distinct fake address so relocation runs to completion. */
uintptr_t elf_find_sym_default(const char *name)
{
    uint32_t h = 2166136261u;
    for (; *name; ++name) {
        h = (h ^ (uint8_t)*name) * 16777619u;
    }
    return 0x40000000u | (h & 0x00fffffcu);
}
