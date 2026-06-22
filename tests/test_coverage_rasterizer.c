#include "coverage_rasterizer.h"

#include <assert.h>
#include <stdio.h>

static void test_full_rectangle(void)
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
    assert(cr_rasterize_mask(mask, 4, 4, 4, rectangle, 4, 4, 4, 2, &workspace)
        == CR_OK);
    for (size_t index = 0; index < sizeof(mask); ++index)
        assert(mask[index] == 255);
}

static void test_antialiased_triangle(void)
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
    assert(cr_rasterize_mask(mask, 4, 4, 4, triangle, 3, 4, 4, 4, &workspace)
        == CR_OK);
    assert(mask[0] == 0 && mask[3] == 0);
    assert(mask[14] == 255);
    assert(mask[1] > 0 && mask[1] < 255);
    assert(cr_blit_xrgb8888(pixels, 4, mask, 4, 4, 4, 0x00ffffffu, 0)
        == CR_OK);
    assert(pixels[0] == 0);
    assert(pixels[14] == 0x00ffffffu);
}

static void test_validation(void)
{
    static const cr_edge open_shape[] = {{0, 0, 4, 4}};
    uint8_t mask[16];
    int64_t intersections[1];
    uint16_t coverage[4];
    cr_workspace workspace = {intersections, 1, coverage, 4};
    assert(cr_rasterize_mask(mask, 4, 4, 4, open_shape, 1, 4, 4, 2, &workspace)
        == CR_INVALID_GEOMETRY);
    workspace.row_capacity = 3;
    assert(cr_rasterize_mask(mask, 4, 4, 4, open_shape, 1, 4, 4, 2, &workspace)
        == CR_WORKSPACE_TOO_SMALL);
}

int main(void)
{
    test_full_rectangle();
    test_antialiased_triangle();
    test_validation();
    puts("coverage-rasterizer tests passed");
    return 0;
}
