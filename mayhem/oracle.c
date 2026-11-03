/* Known-answer oracle for the ELF validator and loader (run by test.sh, built with NORMAL flags).
 * For each fixture it prints the validator/loader verdicts on the pristine image and on a fixed
 * set of structural corruptions; test.sh diffs that output against golden files. Built per
 * architecture; see build.sh. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_elf.h"
#include "private/elf_validate.h"
#include "mem_source.h"

static void put16(uint8_t *p, size_t off, uint16_t v) { memcpy(p + off, &v, 2); }
static void put32(uint8_t *p, size_t off, uint32_t v) { memcpy(p + off, &v, 4); }
static uint16_t get16(const uint8_t *p, size_t off) { uint16_t v; memcpy(&v, p + off, 2); return v; }
static uint32_t get32(const uint8_t *p, size_t off) { uint32_t v; memcpy(&v, p + off, 4); return v; }

static const char *verdict(int rc) { return rc == 0 ? "accept" : "reject"; }

static void check(const char *label, const uint8_t *img, size_t size)
{
    uint8_t *copy = malloc(size);
    memcpy(copy, img, size);
    int rb = esp_elf_validate(copy, size);
    mem_image_t m = { .buf = copy, .size = size };
    elf_source_t src = mem_source(&m);
    int rs = esp_elf_validate_source(&src);
    printf("%-22s buffer=%s source=%s\n", label, verdict(rb), verdict(rs));
    free(copy);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <fixture.elf>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror(argv[1]);
        return 2;
    }
    uint8_t img[4096];
    size_t size = fread(img, 1, sizeof(img), f);
    fclose(f);

    printf("size=%zu type=%u machine=%u entry=0x%x shnum=%u phnum=%u\n", size, get16(img, 16),
           get16(img, 18), get32(img, 24), get16(img, 48), get16(img, 44));

    check("pristine", img, size);

    uint8_t bad[4096];
#define MUTATE(label, stmt) do { memcpy(bad, img, size); stmt; check(label, bad, size); } while (0)
    MUTATE("bad-magic", bad[1] = 'X');
    MUTATE("wrong-machine", put16(bad, 18, get16(img, 18) ^ 0x7f));
    MUTATE("wrong-class", bad[4] = 2);
    MUTATE("shoff-past-end", put32(bad, 32, (uint32_t)size));
    MUTATE("shnum-huge", put16(bad, 48, 0xffff));
    MUTATE("phoff-past-end", put32(bad, 28, (uint32_t)size + 1));
    MUTATE("shstrndx-oob", put16(bad, 50, get16(img, 48)));
    check("truncated-half", img, size / 2);
    check("truncated-header", img, 40);

    esp_elf_t elf;
    if (esp_elf_init(&elf) != 0) {
        return 1;
    }
    int rc = esp_elf_relocate(&elf, img, size);
    printf("relocate rc=%d entry=%s\n", rc, elf.entry ? "set" : "null");
    esp_elf_deinit(&elf);

    memcpy(bad, img, size);
    put16(bad, 18, get16(img, 18) ^ 0x7f);
    if (esp_elf_init(&elf) != 0) {
        return 1;
    }
    rc = esp_elf_relocate(&elf, bad, size);
    printf("relocate-bad rc=%s entry=%s\n", rc ? "error" : "ok", elf.entry ? "set" : "null");
    esp_elf_deinit(&elf);

    mem_image_t m = { .buf = img, .size = size };
    elf_source_t src = mem_source(&m);
    if (esp_elf_init(&elf) != 0) {
        return 1;
    }
    rc = esp_elf_relocate_source(&elf, &src);
    printf("relocate-source rc=%d entry=%s\n", rc, elf.entry ? "set" : "null");
    esp_elf_deinit(&elf);
    return 0;
}
