/*
 * Copyright (C) 2026 creampuffle
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "coverage_rasterizer.h"

#define CR_COORDINATE_LIMIT 65535u

static void sort_intersections(int64_t *values, size_t count)
{
    for (size_t index = 1; index < count; ++index) {
        int64_t value = values[index];
        size_t position = index;
        while (position != 0 && values[position - 1u] > value) {
            values[position] = values[position - 1u];
            --position;
        }
        values[position] = value;
    }
}

static void add_span(
    uint16_t *coverage,
    uint32_t width,
    int64_t left,
    int64_t right)
{
    int64_t limit = (int64_t)width * 256;
    if (left < 0)
        left = 0;
    if (right > limit)
        right = limit;
    if (right <= left)
        return;

    uint32_t first = (uint32_t)left >> 8;
    uint32_t last = (uint32_t)(right - 1) >> 8;
    if (first == last) {
        coverage[first] = (uint16_t)(coverage[first] + (uint16_t)(right - left));
        return;
    }
    coverage[first] = (uint16_t)(coverage[first]
        + (uint16_t)(256 - ((uint32_t)left & 255u)));
    for (uint32_t pixel = first + 1u; pixel < last; ++pixel)
        coverage[pixel] = (uint16_t)(coverage[pixel] + 256u);
    coverage[last] = (uint16_t)(coverage[last]
        + (uint16_t)(right - ((int64_t)last << 8)));
}

static int valid_edge(const cr_edge *edge, uint32_t width, uint32_t height)
{
    return edge->x0 >= 0 && edge->x1 >= 0 && edge->y0 >= 0 && edge->y1 >= 0
        && (uint32_t)edge->x0 <= width && (uint32_t)edge->x1 <= width
        && (uint32_t)edge->y0 <= height && (uint32_t)edge->y1 <= height;
}

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
    cr_workspace *workspace)
{
    if (destination == NULL || width == 0 || height == 0
        || destination_stride < width || edges == NULL || edge_count == 0
        || source_width == 0 || source_height == 0
        || width > CR_COORDINATE_LIMIT || height > CR_COORDINATE_LIMIT
        || source_width > CR_COORDINATE_LIMIT || source_height > CR_COORDINATE_LIMIT
        || samples_y == 0 || samples_y > 8 || workspace == NULL
        || workspace->intersections == NULL || workspace->row_coverage == NULL
        || (size_t)(height - 1u) > ((size_t)-1) / destination_stride)
        return CR_INVALID_ARGUMENT;
    if (workspace->intersection_capacity < edge_count
        || workspace->row_capacity < width)
        return CR_WORKSPACE_TOO_SMALL;
    for (size_t index = 0; index < edge_count; ++index) {
        if (!valid_edge(edges + index, source_width, source_height))
            return CR_INVALID_GEOMETRY;
    }

    int64_t scan_denominator = (int64_t)2 * samples_y * height;
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x)
            workspace->row_coverage[x] = 0;

        for (unsigned sample = 0; sample < samples_y; ++sample) {
            int64_t sample_y = ((int64_t)2 * y * samples_y + 2 * sample + 1)
                * source_height;
            size_t count = 0;
            for (size_t edge_index = 0; edge_index < edge_count; ++edge_index) {
                cr_edge edge = edges[edge_index];
                if (edge.y0 == edge.y1)
                    continue;
                if (edge.y0 > edge.y1) {
                    int32_t swap_x = edge.x0;
                    int32_t swap_y = edge.y0;
                    edge.x0 = edge.x1;
                    edge.y0 = edge.y1;
                    edge.x1 = swap_x;
                    edge.y1 = swap_y;
                }
                int64_t lower = (int64_t)edge.y0 * scan_denominator;
                int64_t upper = (int64_t)edge.y1 * scan_denominator;
                if (sample_y < lower || sample_y >= upper)
                    continue;
                if (count >= workspace->intersection_capacity)
                    return CR_WORKSPACE_TOO_SMALL;

                int64_t x0 = (int64_t)edge.x0 * width * 256 / source_width;
                int64_t dx = (int64_t)(edge.x1 - edge.x0) * width * 256
                    / source_width;
                workspace->intersections[count++] = x0
                    + (sample_y - lower) * dx / (upper - lower);
            }
            sort_intersections(workspace->intersections, count);
            if ((count & 1u) != 0)
                return CR_INVALID_GEOMETRY;
            for (size_t index = 0; index < count; index += 2u)
                add_span(
                    workspace->row_coverage,
                    width,
                    workspace->intersections[index],
                    workspace->intersections[index + 1u]);
        }

        uint8_t *row = destination + (size_t)y * destination_stride;
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t value = ((uint32_t)workspace->row_coverage[x]
                + samples_y / 2u) / samples_y;
            row[x] = (uint8_t)(value > 255u ? 255u : value);
        }
    }
    return CR_OK;
}

static uint32_t blend_channel(uint32_t foreground, uint32_t background, uint32_t alpha)
{
    return (foreground * alpha + background * (255u - alpha) + 127u) / 255u;
}

cr_status cr_blit_xrgb8888(
    uint32_t *destination,
    size_t destination_stride_pixels,
    const uint8_t *mask,
    size_t mask_stride,
    uint32_t width,
    uint32_t height,
    uint32_t foreground,
    uint32_t background)
{
    if (destination == NULL || mask == NULL || width == 0 || height == 0
        || destination_stride_pixels < width || mask_stride < width
        || (size_t)(height - 1u) > ((size_t)-1) / sizeof(*destination)
            / destination_stride_pixels
        || (size_t)(height - 1u) > ((size_t)-1) / mask_stride)
        return CR_INVALID_ARGUMENT;
    foreground &= 0x00ffffffu;
    background &= 0x00ffffffu;
    for (uint32_t y = 0; y < height; ++y) {
        uint32_t *output = destination + (size_t)y * destination_stride_pixels;
        const uint8_t *input = mask + (size_t)y * mask_stride;
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t alpha = input[x];
            uint32_t red = blend_channel(
                (foreground >> 16) & 255u, (background >> 16) & 255u, alpha);
            uint32_t green = blend_channel(
                (foreground >> 8) & 255u, (background >> 8) & 255u, alpha);
            uint32_t blue = blend_channel(
                foreground & 255u, background & 255u, alpha);
            output[x] = (red << 16) | (green << 8) | blue;
        }
    }
    return CR_OK;
}
