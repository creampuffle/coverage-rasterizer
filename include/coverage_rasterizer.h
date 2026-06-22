/*
 * Copyright (C) 2026 creampuffle
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef COVERAGE_RASTERIZER_H
#define COVERAGE_RASTERIZER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct cr_edge {
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
} cr_edge;

typedef struct cr_workspace {
    int64_t *intersections;
    size_t intersection_capacity;
    uint16_t *row_coverage;
    size_t row_capacity;
} cr_workspace;

typedef enum cr_status {
    CR_OK = 0,
    CR_INVALID_ARGUMENT = 1,
    CR_WORKSPACE_TOO_SMALL = 2,
    CR_INVALID_GEOMETRY = 3
} cr_status;

/*
 * Renders closed contours with the even-odd fill rule into an 8-bit coverage
 * mask. Edges may be supplied in any order. Horizontal edges are ignored.
 * Coordinates must lie inside [0, source_width] x [0, source_height].
 * samples_y may be 1 through 8. The destination stride is measured in bytes.
 */
cr_status cr_rasterize_mask(
    uint8_t *destination,
    uint32_t width,
    uint32_t height,
    size_t destination_stride,
    const cr_edge *edges,
    size_t edge_count,
    uint32_t source_width,
    uint32_t source_height,
    unsigned samples_y,
    cr_workspace *workspace);

/* Converts an 8-bit coverage mask to packed 0x00RRGGBB pixels. */
cr_status cr_blit_xrgb8888(
    uint32_t *destination,
    size_t destination_stride_pixels,
    const uint8_t *mask,
    size_t mask_stride,
    uint32_t width,
    uint32_t height,
    uint32_t foreground,
    uint32_t background);

#ifdef __cplusplus
}
#endif

#endif
