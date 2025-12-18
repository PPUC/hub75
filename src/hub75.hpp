#include "pico.h"

// Wiring of the HUB75 matrix
#ifndef DATA_BASE_PIN
#define DATA_BASE_PIN 0
#endif
#define DATA_N_PINS 6
#ifndef ROWSEL_BASE_PIN
#define ROWSEL_BASE_PIN 6
#endif
#define ROWSEL_N_PINS 2
#ifndef CLK_PIN
#define CLK_PIN 11
#endif
#ifndef STROBE_PIN
#define STROBE_PIN 12
#endif
#ifndef OEN_PIN
#define OEN_PIN 13
#endif

#define EXIT_FAILURE 1

// #define TEMPORAL_DITHERING // use temporal dithering - remove define to use no dithering

// Scan rate 1 : 32 for a 64x64 matrix panel means 64 pixel height divided by 32 pixel results in 2 rows lit simultaneously.
// Scan rate 1 : 16 for a 64x64 matrix panel means 64 pixel height divided by 16 pixel results in 4 rows lit simultaneously.
// Scan rate 1 : 16 for a 64x32 matrix panel means 32 pixel height divided by 16 pixel results in 2 rows lit simultaneously.
// Scan rate 1 : 8 for a 64x32 matrix panel means 32 pixel height divided by 8 pixel results in 4 rows lit simultaneously.
// Scan rate 1 : 4 for a 32x16 matrix panel means 16 pixel height divided by 4 pixel results in 4 rows lit simultaneously.
// ...
// Define either HUB75_MULTIPLEX_2_ROWS or HUB75_MULTIPLEX_2_ROWS to fit your matrix panel.

#if !(defined HUB75_MULTIPLEX_2_ROWS) && !(defined HUB75_MULTIPLEX_4_ROWS)
// #define HUB75_MULTIPLEX_2_ROWS // two rows lit simultaneously
#define HUB75_MULTIPLEX_4_ROWS // four rows lit simultaneously
#endif

#ifndef BIT_DEPTH
#define BIT_DEPTH 10 ///< Number of bit planes
#endif

// Accumulator precision has to fit the lut precision.
#ifndef ACC_BITS
#define ACC_BITS 12
#endif

// #define TEMPORAL_DITHERING

enum PanelType
{
    PANEL_GENERIC = 0,
    PANEL_FM6126A,
};

void create_hub75_driver(uint w, uint h, PanelType pt, bool stb_inverted);
void start_hub75_driver();
void update_bgr(const uint8_t *src);
void update(PicoGraphics const *graphics);

void setBasisBrightness(uint8_t factor);
void setIntensity(float intensity);