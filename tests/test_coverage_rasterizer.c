/*
 * Copyright (C) 2026 creampuffle
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "coverage_rasterizer.h"

#include <stdio.h>

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            fprintf(stderr, "check failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
            return 0; \
        } \
    } while (0)

static int test_full_rectangle(void)
{
    static const cr_edge rectangle[] = {
        {0, 0, 4, 0},
        {4, 0, 4, 4},
        {4, 4, 0, 4},
        {0, 4, 0, 0},
    };
    uint8_t mask[4 * 4];
    int64_t intersections[4];
    uint16_t coverage[4];
    cr_workspace workspace = {intersections, 4, coverage, 4};
    CHECK(cr_rasterize_mask(mask, 4, 4, 4, rectangle, 4, 4, 4, 2, &workspace)
        == CR_OK);
    for (size_t index = 0; index < sizeof(mask); ++index)
        CHECK(mask[index] == 255);
    return 1;
}

static int test_antialiased_triangle(void)
{
    static const cr_edge triangle[] = {
        {2, 0, 4, 4},
        {4, 4, 0, 4},
        {0, 4, 2, 0},
    };
    uint8_t mask[4 * 4];
    uint32_t pixels[4 * 4];
    int64_t intersections[3];
    uint16_t coverage[4];
    cr_workspace workspace = {intersections, 3, coverage, 4};
    CHECK(cr_rasterize_mask(mask, 4, 4, 4, triangle, 3, 4, 4, 4, &workspace)
        == CR_OK);
    CHECK(mask[0] == 0 && mask[3] == 0);
    CHECK(mask[14] == 255);
    CHECK(mask[1] > 0 && mask[1] < 255);
    CHECK(cr_blit_xrgb8888(pixels, 4, mask, 4, 4, 4, 0x00ffffffu, 0)
        == CR_OK);
    CHECK(pixels[0] == 0);
    CHECK(pixels[14] == 0x00ffffffu);
    return 1;
}

static int test_validation(void)
{
    static const cr_edge open_shape[] = {{0, 0, 4, 4}};
    uint8_t mask[16];
    int64_t intersections[1];
    uint16_t coverage[4];
    cr_workspace workspace = {intersections, 1, coverage, 4};
    CHECK(cr_rasterize_mask(mask, 4, 4, 4, open_shape, 1, 4, 4, 2, &workspace)
        == CR_INVALID_GEOMETRY);
    workspace.row_capacity = 3;
    CHECK(cr_rasterize_mask(mask, 4, 4, 4, open_shape, 1, 4, 4, 2, &workspace)
        == CR_WORKSPACE_TOO_SMALL);
    return 1;
}

int main(void)
{
    if (!test_full_rectangle() || !test_antialiased_triangle() || !test_validation())
        return 1;
    puts("coverage-rasterizer tests passed");
    return 0;
}
