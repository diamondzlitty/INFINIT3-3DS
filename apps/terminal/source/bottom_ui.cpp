#include <3ds.h>
#include "bottom_ui.hpp"

#include <stddef.h>

namespace {

constexpr int LOGICAL_WIDTH  = 320;
constexpr int LOGICAL_HEIGHT = 240;
constexpr int FB_STRIDE      = 240;

static inline u16 gray(u8 value)
{
    return RGB8_to_565(
        value,
        value,
        value
    );
}

static inline u16 *framebuffer()
{
    return reinterpret_cast<u16 *>(
        gfxGetFramebuffer(
            GFX_BOTTOM,
            GFX_LEFT,
            nullptr,
            nullptr
        )
    );
}

static inline size_t pixelIndex(
    int x,
    int y
)
{
    // 3DS framebuffers are stored sideways.
    return
        static_cast<size_t>(x) *
        FB_STRIDE +
        static_cast<size_t>(
            FB_STRIDE - 1 - y
        );
}

static inline void putPixel(
    int x,
    int y,
    u16 color
)
{
    if (
        x < 0 ||
        x >= LOGICAL_WIDTH ||
        y < 0 ||
        y >= LOGICAL_HEIGHT
    ) {
        return;
    }

    framebuffer()[
        pixelIndex(x, y)
    ] = color;
}

static void fillScreen(
    u16 color
)
{
    u16 *fb = framebuffer();

    for (
        int x = 0;
        x < LOGICAL_WIDTH;
        ++x
    ) {
        for (
            int y = 0;
            y < LOGICAL_HEIGHT;
            ++y
        ) {
            fb[
                pixelIndex(x, y)
            ] = color;
        }
    }
}

static void horizontalLine(
    int x,
    int y,
    int width,
    u16 color
)
{
    for (
        int px = x;
        px < x + width;
        ++px
    ) {
        putPixel(
            px,
            y,
            color
        );
    }
}

static void verticalLine(
    int x,
    int y,
    int height,
    u16 color
)
{
    for (
        int py = y;
        py < y + height;
        ++py
    ) {
        putPixel(
            x,
            py,
            color
        );
    }
}

static void rectangle(
    int x,
    int y,
    int width,
    int height,
    u16 color
)
{
    if (
        width <= 0 ||
        height <= 0
    ) {
        return;
    }

    horizontalLine(
        x,
        y,
        width,
        color
    );

    horizontalLine(
        x,
        y + height - 1,
        width,
        color
    );

    verticalLine(
        x,
        y,
        height,
        color
    );

    verticalLine(
        x + width - 1,
        y,
        height,
        color
    );
}

}

bool t6aBottomInit()
{
    // consoleInit(GFX_BOTTOM) establishes RGB565.
    // Keep this screen single-buffered and persistent.
    gfxSetDoubleBuffering(
        GFX_BOTTOM,
        false
    );

    return
        gfxGetScreenFormat(
            GFX_BOTTOM
        ) == GSP_RGB565_OES;
}

void t6aBottomDrawFoundation()
{
    const u16 black =
        gray(0);

    const u16 dim =
        gray(75);

    const u16 medium =
        gray(125);

    const u16 bright =
        gray(210);

    fillScreen(
        black
    );

    // Outer terminal frame.
    rectangle(
        3,
        3,
        314,
        234,
        medium
    );

    // Header.
    rectangle(
        7,
        7,
        306,
        22,
        bright
    );

    // Selection region.
    rectangle(
        7,
        31,
        306,
        30,
        medium
    );

    // Candle inspection region.
    rectangle(
        7,
        63,
        306,
        54,
        dim
    );

    // Big-3 / macro region.
    rectangle(
        7,
        119,
        306,
        58,
        dim
    );

    // Status region.
    rectangle(
        7,
        179,
        306,
        32,
        dim
    );

    // Controls region.
    rectangle(
        7,
        213,
        306,
        20,
        medium
    );
}
