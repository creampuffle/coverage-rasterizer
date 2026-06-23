# Coverage Rasterizer

Small freestanding C rasterizer I extracted from my graphics work for FLAP.

It takes closed vector edges, fills them using the even-odd rule, and produces an 8-bit antialiased coverage mask. It can also blend that mask into an XRGB8888 pixel buffer.

Current features include

- closed contour filling
- 1 to 8 vertical coverage samples
- even-odd fill rules
- XRGB8888 output
- caller-owned work memory
- no heap allocation
- no operating system dependencies

## Build and test

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The public API is in `include/coverage_rasterizer.h`.

Released under the BSD 3-Clause License. Please keep the copyright and license notices.
