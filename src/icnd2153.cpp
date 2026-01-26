#include "pico/stdlib.h"

#include "hub75.hpp"
#include "icnd2153.h"

namespace
{
constexpr uint8_t kRegNum = 5;
constexpr uint8_t kCmdLatchs[kRegNum] = {4, 6, 8, 10, 2};

uint16_t reg_val[kRegNum][3];

void icnd2153_init_gpio()
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

inline void icnd2153_send_clock()
{
    gpio_put(CLK_PIN, 1);
    gpio_put(CLK_PIN, 0);
}

inline void icnd2153_send_latch(uint8_t clocks)
{
    gpio_put(STROBE_PIN, 1);
    while (clocks--)
    {
        icnd2153_send_clock();
    }
    gpio_put(STROBE_PIN, 0);
}

void icnd2153_send_configuration(const uint16_t reg_dat[3], uint8_t reg_id, uint8_t chip_num)
{
    icnd2153_send_latch(14); // pre-active command

    for (uint8_t chip = 0; chip < chip_num; chip++)
    {
        for (uint8_t bit = 0; bit < 16; bit++)
        {
            if ((chip == (chip_num - 1)) && (bit == (16 - kCmdLatchs[reg_id])))
            {
                gpio_put(STROBE_PIN, 1);
            }

            uint16_t data_mask = (uint16_t)(0x8000u >> bit);
            bool r = (reg_dat[0] & data_mask) != 0;
            bool g = (reg_dat[1] & data_mask) != 0;
            bool b = (reg_dat[2] & data_mask) != 0;

            gpio_put(DATA_BASE_PIN + 0, r);
            gpio_put(DATA_BASE_PIN + 1, g);
            gpio_put(DATA_BASE_PIN + 2, b);
            gpio_put(DATA_BASE_PIN + 3, r);
            gpio_put(DATA_BASE_PIN + 4, g);
            gpio_put(DATA_BASE_PIN + 5, b);

            icnd2153_send_clock();
        }

        gpio_put(DATA_BASE_PIN + 0, 0);
        gpio_put(DATA_BASE_PIN + 1, 0);
        gpio_put(DATA_BASE_PIN + 2, 0);
        gpio_put(DATA_BASE_PIN + 3, 0);
        gpio_put(DATA_BASE_PIN + 4, 0);
        gpio_put(DATA_BASE_PIN + 5, 0);
        gpio_put(STROBE_PIN, 0);
    }
}
} // namespace

void ICND2153_setup(uint8_t chip_num)
{
    icnd2153_init_gpio();

    // reg1
    reg_val[0][0] = (0x07 << 8) | (1 << 6) | (3 << 4) | (0 << 3) | (0 << 0);
    reg_val[0][1] = reg_val[0][0];
    reg_val[0][2] = reg_val[0][0];
    // reg2
    reg_val[1][0] = (31 << 10) | (0 << 9) | (255 << 1) | (0 << 0);
    reg_val[1][1] = (reg_val[1][0] & 0x83FF) | (28 << 10);
    reg_val[1][2] = (reg_val[1][0] & 0x83FF) | (23 << 10);
    // reg3
    reg_val[2][0] = (4 << 12) | (0 << 10) | (1 << 9) | (0 << 8) | (0 << 4) | (0 << 3) | (1 << 2) | (3 << 0);
    reg_val[2][1] = reg_val[2][0];
    reg_val[2][2] = reg_val[2][0];
    // reg4
    reg_val[3][0] = 0x0040;
    reg_val[3][1] = reg_val[3][0];
    reg_val[3][2] = reg_val[3][0];
    // reg5 / debug
    reg_val[4][0] = 0x0008; // Reg5:0x003C
    reg_val[4][1] = reg_val[4][0];
    reg_val[4][2] = reg_val[4][0];

    icnd2153_send_latch(14); // pre-active command
    icnd2153_send_latch(12); // enable all output channels

    for (uint8_t id = 0; id < kRegNum; id++)
    {
        icnd2153_send_configuration(reg_val[id], id, chip_num);
    }
}
