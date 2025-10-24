#pragma once

#include "libraries/pico_graphics/pico_graphics.hpp"

using namespace pimoroni;

class PixelFill : public PicoGraphics_PenRGB888
{
private:
    int w;
    int h;

    int i = 0;
    int j = 0;

    static const int limit = 1;
    int count = 0;
    int index = 0;
    int l = 0;

    void drawPixel(int x, int y, uint32_t color)
    {
        set_pen(color);
        set_pixel(Point(x, y));
    }

public:
    explicit PixelFill(uint width = 32, uint height = 16) : PicoGraphics_PenRGB888(width, height, nullptr), w(width), h(height)
    {
        set_pen(0);
        clear();
    }

    void fill(int start, int end)
    {
        static const uint32_t col[] = {0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0x00ffff, 0xffffff, 0x8b4513, 0xffa500};

        count++;

        if (count > limit)
        {
            count = 0;
            j++;
            if (j > 31)
            {
                j = 0;
                l++;
                if ( l > 15) l = 0;
            }
        }

         drawPixel(j, l, col[l%8]);
    }
};
