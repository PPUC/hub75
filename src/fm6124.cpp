#include "pico/stdlib.h"

#include "hub75.hpp"
#include "fm6124.h"

namespace
{
void fm6124_init_gpio()
{
    for (auto i = 0; i < DATA_N_PINS; i++)
    {
        gpio_init(DATA_BASE_PIN + i);
        gpio_set_function(DATA_BASE_PIN + i, GPIO_FUNC_SIO);
        gpio_set_dir(DATA_BASE_PIN + i, true);
        gpio_put(DATA_BASE_PIN + i, 0);
    }

    for (auto i = 0; i < ROWSEL_N_PINS; i++)
    {
        gpio_init(ROWSEL_BASE_PIN + i);
        gpio_set_function(ROWSEL_BASE_PIN + i, GPIO_FUNC_SIO);
        gpio_set_dir(ROWSEL_BASE_PIN + i, true);
        gpio_put(ROWSEL_BASE_PIN + i, 0);
    }

    gpio_init(CLK_PIN);
    gpio_set_function(CLK_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(CLK_PIN, true);
    gpio_put(CLK_PIN, 0);

    gpio_init(STROBE_PIN);
    gpio_set_function(STROBE_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(STROBE_PIN, true);
    gpio_put(STROBE_PIN, 0);

    gpio_init(OEN_PIN);
    gpio_set_function(OEN_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(OEN_PIN, true);
    gpio_put(OEN_PIN, 0);
}

inline void fm6124_set_data(bool bit)
{
    for (auto i = 0; i < DATA_N_PINS; i++)
    {
        gpio_put(DATA_BASE_PIN + i, bit);
    }
}

inline void fm6124_clock_pulse()
{
    gpio_put(CLK_PIN, 1);
    gpio_put(CLK_PIN, 0);
}
} // namespace

void FM6124_setup()
{
    fm6124_init_gpio();

    // FM6124/ICN2038S: REG1 sets global brightness power, REG2 enables output
    const bool reg1[16] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
    const bool reg2[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};

    gpio_put(OEN_PIN, 1); // disable display

    // Write REG1
    for (uint32_t l = 0; l < MATRIX_PANEL_WIDTH; l++)
    {
        fm6124_set_data(reg1[l % 16]);
        if (l > MATRIX_PANEL_WIDTH - 12)
        {
            gpio_put(STROBE_PIN, 1);
        }
        fm6124_clock_pulse();
    }
    gpio_put(STROBE_PIN, 0);

    // Write REG2
    for (uint32_t l = 0; l < MATRIX_PANEL_WIDTH; l++)
    {
        fm6124_set_data(reg2[l % 16]);
        if (l > MATRIX_PANEL_WIDTH - 13)
        {
            gpio_put(STROBE_PIN, 1);
        }
        fm6124_clock_pulse();
    }
    gpio_put(STROBE_PIN, 0);

    // Clear data registers
    fm6124_set_data(false);
    for (uint32_t l = 0; l < MATRIX_PANEL_WIDTH; l++)
    {
        fm6124_clock_pulse();
    }

    gpio_put(STROBE_PIN, 1);
    fm6124_clock_pulse();
    gpio_put(STROBE_PIN, 0);
    gpio_put(OEN_PIN, 0); // enable display
    fm6124_clock_pulse();
}
