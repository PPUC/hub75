- [HUB75 DMA-Based Driver](#hub75-dma-based-driver)
  - [Documentation and References](#documentation-and-references)
  - [Achievements of the Revised Driver](#achievements-of-the-revised-driver)
  - [Motivation](#motivation)
  - [Evolution of Pico HUB75 Drivers](#evolution-of-pico-hub75-drivers)
    - [Raspberry Pi Pico HUB75 Example](#raspberry-pi-pico-hub75-example)
    - [Pimoroni HUB75 Driver](#pimoroni-hub75-driver)
  - [Eliminating `hub75_wait_tx_stall`](#eliminating-hub75_wait_tx_stall)
    - [Original `hub75_wait_tx_stall` Implementation](#original-hub75_wait_tx_stall-implementation)
    - [Alternative Approach](#alternative-approach)
  - [DMA Chains and PIO State Machines in the Revised HUB75 Driver](#dma-chains-and-pio-state-machines-in-the-revised-hub75-driver)
    - [Overview](#overview)
    - [Step-by-Step Breakdown](#step-by-step-breakdown)
    - [Refresh Rate Performance](#refresh-rate-performance)
    - [Key Benefits of this Approach](#key-benefits-of-this-approach)
  - [Conclusion for DMA and PIO based Approach](#conclusion-for-dma-and-pio-based-approach)
  - [Improved Colour Perception](#improved-colour-perception)
    - [Increased Perceptual Colour Depth (Temporal Dithering)](#increased-perceptual-colour-depth-temporal-dithering)
    - [✅ Advantages](#-advantages)
    - [⚠️ Trade-offs](#️-trade-offs)
  - [Brightness Control](#brightness-control)
    - [API Functions](#api-functions)
    - [How it Works](#how-it-works)
    - [Default Settings](#default-settings)
    - [Practical Notes](#practical-notes)
  - [Demo Effects](#demo-effects)
  - [How to Use This Project in VSCode](#how-to-use-this-project-in-vscode)
  - [Next Steps](#next-steps)
- [Prerequisites for the Hub75 Driver](#prerequisites-for-the-hub75-driver)
  - [Wiring Details](#wiring-details)
    - [Colour Data Pins](#colour-data-pins)
    - [Address (Row Select) Pins](#address-row-select-pins)
    - [Control Pins](#control-pins)
    - [One Glance Mapping HUB75 Connector → Pico GPIOs](#one-glance-mapping-hub75-connector--pico-gpios)
  - [Allowed Deviations  ](#allowed-deviations--)
    - [Example: Custom Pin Mapping](#example-custom-pin-mapping)
  - [How to Use a 64×32 HUB75 Matrix Panel](#how-to-use-a-6432-hub75-matrix-panel)
    - [Wiring](#wiring)
    - [Initialization of Matrix Panel Dimension](#initialization-of-matrix-panel-dimension)
    - [Frame Buffer Layout](#frame-buffer-layout)
    - [Practical Notes](#practical-notes-1)
  - [Scan Rate Support](#scan-rate-support)
    - [Multiplexing Modes (Rows Lit Simultaneously)](#multiplexing-modes-rows-lit-simultaneously)
    - [Example: 64×64 panel, 1:32 scan](#example-6464-panel-132-scan)
    - [How to Configure](#how-to-configure)

# HUB75 DMA-Based Driver

<https://github.com/user-attachments/assets/7c41193c-c724-4fae-8823-af36d70fcedd>

*Demo video: Colours are much brighter and more brilliant in reality*

## Documentation and References

This project is based on:

- [Raspberry Pi's pico-examples/pio/hub75](https://github.com/raspberrypi/pico-examples)
- [Pimoroni's HUB75 driver](https://github.com/pimoroni/pimoroni-pico/tree/main/drivers/hub75)

To understand how RGB matrix panels work, refer to the article **[Everything You Didn't Want to Know About RGB Matrix Panels](https://learn.adafruit.com/adafruit-gfx-graphics-library/what-is-the-gfx-library)**.
For details on Binary Coded Modulation (BCM), see **[LED Dimming Using Binary Code Modulation](https://www.ti.com/lit/an/slva377a/slva377a.pdf)**.

---

## Achievements of the Revised Driver

The modifications to the Pimoroni HUB75 driver result in the following improvements:

- **Offloading Work**: Moves processing from the CPU to DMA and PIO co-processors.
- **Performance Boost**: Implements self-paced, interlinked DMA and PIO processes.
- **Eliminates Synchronization Delays**: No need for `hub75_wait_tx_stall`, removing blocking synchronization.
- **Optimized Interrupt Handling**: Reduces code complexity in the interrupt handler.

These enhancements lead to significant performance improvements. In tests up to a **250 MHz system clock**, no ghost images were observed.

---

## Motivation

As part of a private project, I sought to gain deeper knowledge of the Raspberry Pi Pico microcontroller. I highly recommend **[Raspberry Pi Pico Lectures 2022 by Hunter Adams](https://youtu.be/CAMTBzPd-WI?feature=shared)**—they provide excellent insights!

If you are specifically interested in **PIO (Programmable Input/Output)**, start with [Lecture 14: Introducing PIO](https://youtu.be/BVdaw56Ln8s?feature=shared) and [Lecture 15: PIO Overview and Examples](https://youtu.be/wet9CYpKZOQ).

Inspired by Adams' discussion on **[DMA](https://youtu.be/TGjUHChO1kM?feature=shared&t=1475) and PIO co-processors**, I optimized the HUB75 driver as a self-assigned challenge.

😊 **[Raspberry Pi Pico Lectures 2025 by Hunter Adams](https://youtu.be/a4uLrfqHZQU?feature=shared")** is available now!

---

## Evolution of Pico HUB75 Drivers

### Raspberry Pi Pico HUB75 Example

The **Pico HUB75 example** demonstrates connecting an **HUB75 LED matrix panel** using PIO. This educational example prioritizes clarity and ease of understanding.

- The color palette is generated by modulating the **Output Enable (OE)** signal.
- **Binary Coded Modulation (BCM)** is applied row-by-row, modulating all color bits before advancing to the next row.
- Synchronization depends on `hub75_wait_tx_stall`.
- **No DMA is used**, leading to lower performance.

### Pimoroni HUB75 Driver

The **Pimoroni HUB75 driver** improves performance by:

- Switching from **row-wise** to **plane-wise** modulation handling.
- Using **DMA** to transfer pixel data to the PIO state machine.
- Still relying on `hub75_wait_tx_stall` for synchronization.

![hub_pimoroni](assets/pimoroni_dma.png)

*Picture 1: Pimoroni's Hub75 Driver DMA Section*

---

## Eliminating `hub75_wait_tx_stall`

Both the **Raspberry Pi and Pimoroni** implementations use `hub75_wait_tx_stall`, which ensures:

- The state machine **stalls** on an empty TX FIFO.
- The system **waits** until the OEn pulse has finished.

However, this blocking method **prevents an efficient DMA-based approach**.

### Original `hub75_wait_tx_stall` Implementation

```c
static inline void hub75_wait_tx_stall(PIO pio, uint sm) {
    uint32_t txstall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
    pio->fdebug = txstall_mask;
    while (!(pio->fdebug & txstall_mask)) {
        tight_loop_contents();
    }
}
```

### Alternative Approach

Instead of waiting for TX FIFO stalling, we can:

1. Modify the **PIO program** to emit a signal once the OEn pulse completes.
2. Set up a **DMA channel** to listen for this signal.
3. Establish an **interrupt handler** to trigger once the signal is received.

This approach allows fully **chained DMA execution** without CPU intervention.

<img src="assets/hub75_row.png" width="360" height="186">

*Picture 2: Modified hub75_row Program*

---

## DMA Chains and PIO State Machines in the Revised HUB75 Driver

### Overview

The following diagram illustrates the interactions between **DMA channels** and **PIO state machines**:

```
[ Pixel Data DMA ] -> [ hub75_data_rgb888 PIO ]
       |
       |--> [ Dummy Pixel Data DMA ] -> [ hub75_data_rgb888 PIO ]
                  |
                  |--> [ OEn Data DMA ] -> [ hub75_row PIO ]
                           |
                           |--> [ OEn Finished DMA ] (Triggers interrupt)
```

### Step-by-Step Breakdown

1. **Pixel Data Transfer**
   - Pixel data is streamed via **DMA** to the **hub75_rdata_gb888** PIO state machine.
   - This handles shifting pixel data into the LED matrix.

2. **Dummy Pixel Handling**
   - A secondary **dummy pixel DMA channel** adds additional pixel data.
   - This ensures correct clocking of the final piece of genuine data.

3. **OEn Pulse Generation**
   - The **OEn data DMA channel** sends 32-bit words - 5 bit address information (row select) and 27 bit puls width - to the **hub75_row** PIO state machine.
   - This output enable signal switches on those LEDs in the current row with bit set in the current bitplane for the specified number of cycles.

4. **Interrupt-Driven Synchronization**
   - A final **OEn finished DMA channel** listens for the end of the pulse.
   - An **interrupt handler** (`oen_finished_handler`) resets DMA for the next cycle.

![hub75_dma](assets/hub75_dma.png)

*Picture 3: Chained DMA Channels and assigned PIOs*

---

### Refresh Rate Performance

With a **bit-depth of 10**, the HUB75 driver achieves the following refresh rates for a 64 x 64 matrix depending on the system clock:

| System Clock | Refresh Rate |
|--------------|---------------|
| 100 MHz      | 179 Hz        |
| 150 MHz      | 268 Hz        |
| 200 MHz      | 358 Hz        |
| 250 MHz      | 448 Hz        |

These results demonstrate stable operation and high-performance display rendering across a wide range of system clocks.

### Key Benefits of this Approach

✅ Fully **automated** data transfer using **chained DMA channels**.

✅ Eliminates **CPU-intensive** busy-waiting (`hub75_wait_tx_stall`).

✅ Ensures **precise timing** without unnecessary stalling.

---

## Conclusion for DMA and PIO based Approach

By offloading tasks to **DMA and PIO**, the revised HUB75 driver achieves **higher performance**, **simpler interrupt handling**, and **better synchronization**. This approach significantly reduces CPU overhead while eliminating artifacts like **ghosting** at high clock speeds.

If you're interested in optimizing **RGB matrix panel drivers**, this implementation serves as a valuable reference for efficient DMA-based rendering.

---

## Improved Colour Perception

The graphics system for the demo effects operates in **RGB888** format (8 bits per channel, 24 bits per pixel). To better match human vision, colours are mapped using the [CIE 1931 lightness curve](https://jared.geek.nz/2013/02/linear-led-pwm/). This mapping effectively expands the usable range to **10 bits per channel**.

The HUB75 driver takes advantage of this: its PIO/DMA pipeline packs each pixel as a **32-bit word** with 10 bits for red, 10 bits for green, and 10 bits for blue.

---

### Increased Perceptual Colour Depth (Temporal Dithering)

To go beyond native 10-bit precision without changing the data format, the driver employs  **temporal dithering** (an accumulator-based technique):

- Each pixel maintains a high-precision accumulator (e.g. 16 bits).  
- On every refresh, the top 10 bits are sent to the panel, while the lower bits remain stored.  
- Over successive frames, these residuals accumulate, averaging out to produce smoother gradients.  

This results in a perceived colour depth equivalent to **12–14 bits per channel**.

### ✅ Advantages

- Noticeable improvement in gradients and subtle colour transitions.  
- Minimal CPU overhead (shifts and adds only).  

### ⚠️ Trade-offs

- Requires additional RAM for accumulators.  
  For a 64×64 panel: `64 × 64 × 3 × sizeof(uint32_t) ≈ 48 KB`.

## Brightness Control

In addition to bitplane modulation, the driver supports **software-based brightness regulation**. This allows easy adjustment of overall panel brightness without hardware changes.

### API Functions

```cpp
// Set the baseline brightness scaling factor (default = 6, range 1–255).
// Larger values increase brightness but also raise OEn frequency.
void setBasisBrightness(uint8_t factor);

// Set fine-grained brightness intensity as a fraction [0.0 – 1.0].
void setIntensity(float intensity);
```

### How it Works

- <code>setBasisBrightness(basis)</code>

  Defines the top brightness.

  Example: <code>setBasisBrightness(6)</code> → default brightness range for typical 64×64 panels. \
  Larger factors give more headroom for brightness but consume more **Binary Coded Modulation (BCM)** time slices.

- <code>setIntensity(intensity)</code>
  
  Fine-grained adjustment from 0.0 (dark/off) to 1.0 (full brightness).\
  This function scales the effective duty cycle within the current baseline brightness range.

```cpp
// Example: brighten the panel, then dim at runtime
setBasisBrightness(8); // Start with baseline factor 8 for a brighter panel
setIntensity(0.5f);    // Show at 50% of that baseline
```

### Default Settings

- <code>basis_factor = 6u</code>
- <code>intensity = 1.0f</code>
  (full brightness within the baseline)

This corresponds to the same brightness as earlier driver revisions without adjustment.

### Practical Notes

- Increasing the basis factor may increase peak current consumption.
- For indoor use, values between 4–8 are usually sufficient.
- For dimmer environments, you can keep the baseline factor low (e.g. 4) and rely on setIntensity() for smooth runtime control.
- Both functions are non-blocking and can be called during normal operation.

## Demo Effects

⚠️ The examples contained in hub75_lvgl.cpp have been tested with a Raspberry Pi Pico 2 microcontroller (RP2350). For a RP2040 processor you might have to comment out some demo effects due to minor memory capabilities. Ask if you need support 🙂.

## How to Use This Project in VSCode

You can easily use this project with VSCode, especially with the **Raspberry Pi Pico plugin** installed. Follow these steps:

1. **Open VSCode and start a new window**.
2. **Clone the repository**:
   - Press `Ctrl+Shift+P` and select `Git: Clone`.
   - Paste the URL: `https://github.com/JuPfu/hub75`

      <img src="assets/VSCode_1.png" width="460" height="116">

   - Choose a local directory to clone the repository into.

      <img src="assets/VSCode_2.png" width="603" height="400">

3. **Project Import Prompt**:
   - Consent to open the project.

      <img src="assets/VSCode_3.png" width="603" height="400">

   - When prompted, "Do you want to import this project as Raspberry Pi Pico project?", click **Yes** or wait a few seconds until the dialog prompt disappears by itself.

4. **Configure Pico SDK Settings**:
   - A settings page will open automatically.
   - Use the default settings unless you have a specific setup.

      <img src="assets/VSCode_4.png" width="603" height="400">

   - Click **Import** to finalize project setup.
   - Switch the board-type to your Pico model.

      <img src="assets/VSCode_5.png" width="599" height="415">

5. **Wait for Setup Completion**:
   - VSCode will download required tools, the Pico SDK, and any plugins.

6. **Connect the Hardware**:
   - Make sure the HUB75 LED matrix is properly connected to the Raspberry Pi Pico.
   - Attach the Rasberry Pi Pico USB cable to your computer

7. **Build and Upload**:
   - Compiling the project can be done without a Pico attached to the computer.

      <img src="assets/VSCode_6.png" width="600" height="416">

   - Click the **Run** button in the bottom taskbar.
   - VSCode will compile and upload the firmware to your Pico board.

> 💡 If everything is set up correctly, your matrix should come to life with the updated HUB75 DMA driver.

---

## Next Steps

- **Add another chained DMA channel** to further reduce calls to the oen_finished_handler, trading memory for reduced CPU load.

- **Investigate removing the hub75_data_rgb888_set_shift method**, potentially achieving a completely DMA- and PIO-based solution with no CPU involvement.

For any questions or discussions, feel free to contribute or open an issue!

# Prerequisites for the Hub75 Driver

This driver is designed for a **64×64 LED matrix panel**. It can be adapted for **64×32, 32×32**, or other HUB75-compatible panels.

The PIO implementation requires that **data pins (colours)** and **row-select pins** must be in **consecutive GPIO blocks**.

The default implementation looks like this (see hub75.cpp). An example of a valid alternative pin defintion is shown in [Allowed Deviations](#allowed_deviations_anchor)

   ```cpp
   // Default wiring of HUB75 matrix to RP2350
   #define DATA_BASE_PIN    0   // first color data pin
   #define DATA_N_PINS      6   // number of color data pins (R0,G0,B0,R1,G1,B1)
   #define ROWSEL_BASE_PIN  6   // first row-select (address) pin
   #define ROWSEL_N_PINS    5   // number of row-select pins (A0–A4)
   #define CLK_PIN          11  // clock
   #define STROBE_PIN       12  // latch (LAT)
   #define OEN_PIN          13  // output enable (OE)
   ```

## Wiring Details

### Colour Data Pins

- `DATA_BASE_PIN` = **GPIO 0** (first in a consecutive block)
- `DATA_N_PINS` = **6** (for R0, G0, B0, R1, G1, B1)

| Hub75 Colour Bit   | connected to      | Pico GPIO |
|:-------------------|-------------------|:-----:|
| R0                 |                   | 0    |
| G0                 |                   | 1    |
| B0                 |                   | 2    |
| R1                 |                   | 3    |
| G1                 |                   | 4    |
| B1                 |                   | 5    |

### Address (Row Select) Pins

- `ROWSEL_BASE_PIN` = **GPIO 6**
- `ROWSEL_N_PINS` = **5** (A0–A4)

- **Consecutiveness is required** by the PIO program.

| Address bit |  connected to      | Pico GPIO |
| ----------- |--------------------|:---------:|
| A0          |                    | 6    |
| A1          |                    | 7    |
| A2          |                    | 8    |
| A3          |                    | 9    |
| A4          |                    | 10   |

### Control Pins

- **CLK** (clock): GPIO 11
- **LAT** (strobe/latch): GPIO 12
- **OE** (output enable): GPIO 13

### One Glance Mapping HUB75 Connector → Pico GPIOs

The diagram shows the default mapping as defined in the hub75.cpp file.
  
<img src="assets/pico_wiring.png">

## Allowed Deviations  <a id='allowed_deviations_anchor'></a>

The **only strict requirement** is that **data pins** and **row-select pins** must be in **consecutive GPIO blocks**.
Clock, Latch, and OE pins may be freely chosen.

### Example: Custom Pin Mapping

```cpp
#define ROWSEL_BASE_PIN  15  // Row select pins moved to GPIO 15–19
#define ROWSEL_N_PINS    5   // number of row-select pins (A0–A4)

#define DATA_BASE_PIN    3   // Color data pins starting at GPIO 3
#define DATA_N_PINS      6   // number of color data pins (R0,G0,B0,R1,G1,B1)

// Control pins assigned to arbitrarily GPIO pins
#define CLK_PIN          0
#define STROBE_PIN       1
#define OEN_PIN          2
```

## How to Use a 64×32 HUB75 Matrix Panel

The Hub75 driver is designed for 64×64 panels, but it also supports **64×32 panels** (half the height) and other panel dimensions e.g. **128x64**.  
The electrical connections for 64x32 panels are nearly identical to 64x64 panels — the difference lies in how rows are addressed and how the frame buffer is filled.

### Wiring

In file `hub75_driver.cpp` use the same pin definitions as for a 64×64 panel, **except** for the `ROWSEL_N_PINS` definition which must be changed to `4` (A0–A3). Address line **A4 is not connected**.

   ```cpp
   // Default wiring of HUB75 matrix to RP2350
   #define DATA_BASE_PIN    0   // first color data pin
   #define DATA_N_PINS      6   // number of color data pins (R0,G0,B0,R1,G1,B1)
   #define ROWSEL_BASE_PIN  6   // first row-select (address) pin
   #define ROWSEL_N_PINS    4   // number of row-select pins (A0–A3)
   #define CLK_PIN          11  // clock
   #define STROBE_PIN       12  // latch (LAT)
   #define OEN_PIN          13  // output enable (OE)
   ```

### Initialization of Matrix Panel Dimension

At the top of file `hub75_driver.cpp` set the `MATRIX_PANEL_HEIGHT` to the height of the panel (32 in this example).

   ```cpp
   #define MATRIX_PANEL_WIDTH 64
   #define MATRIX_PANEL_HEIGHT 32
   #define OFFSET MATRIX_PANEL_WIDTH *(MATRIX_PANEL_HEIGHT >> 1)
   ```

### Frame Buffer Layout

- A 64×32 panel uses half the memory of a 64×64 panel.
- Each refresh cycle addresses 16 row pairs (instead of 32).
- The driver automatically adjusts its addressing logic based on the height you specify.

### Practical Notes

Not all of the demo effects will show correctly for a 64x32 matrix panel. The first two demo effects use image data for a 64x64 layout. You will see some output, but it will look weird.

The `bouncing balls` effect will not show the complete text as the position is hard coded. The `fire_effect` and the `rotator`might look as they should be.

Have fun with adapting the source code or with implementing your own effects.

Do not hesitate to contact me - I will gladly answer your questions!

## Scan Rate Support

### Multiplexing Modes (Rows Lit Simultaneously)

Many HUB75 panel datasheets describe the **scan rate** using terms like *1:16* or *1:32*.  
This can be confusing, because the actual driver behavior is about **how many rows are lit simultaneously**.

To make things explicit, this driver uses **multiplexing defines**:

| Multiplexing Mode         | Rows Lit at Once | Typical Datasheet Scan Rate | Example Panels               |
|----------------------------|------------------|-----------------------------|------------------------------|
| `#define HUB75_MULTIPLEX_2_ROWS` | 2 rows          | 1:32                        | 64×64 (1:32), 64×32 (1:16)   |
| `#define HUB75_MULTIPLEX_4_ROWS` | 4 rows          | 1:16 or 1:8                 | 64×64 (1:16), 64×32 (1:8)    |

### Example: 64×64 panel, 1:32 scan

- Datasheet says **1:32** (one out of 32 row groups active at a time).  
- This means **2 rows lit simultaneously**.  
- In code, use:

  ```c
  #define HUB75_MULTIPLEX_2_ROWS
  ```

### How to Configure

In your build, define the scan rate that matches your panel:

```cpp
// Example for 64×64 panels (1/32 scan) - two rows lit simultaneously
#define HUB75_MULTIPLEX_2_ROWS
// Set the number of address lines - 2 rows lit simultaneously leaves 32 rows to be adressed via row select.
// That is 32 = 2 to the power of 5 - we need 5 row select pins  
#define ROWSEL_N_PINS 5

// Example for 64×32 panels (1/8 scan) - four rows lit simultaneously
#define HUB75_MULTIPLEX_4_ROWS
// Set the number of address lines - 4 rows lit simultaneously leaves 8 rows to be adressed via row select.
// That is 8 = 2 to the power of 3 - we need 3 row select pins  
#define ROWSEL_N_PINS 3
```

**⚠️ Do not forget to adapt the number of address lines to fit your matrix panel**

74HC138B 

74HC138; 74HCT138
3-to-8 line decoder/demultiplexer; inverting

The 74HC138; 74HCT138 decodes three binary weighted address inputs (A0, A1 and A2) to
eight mutually exclusive outputs (Y0 to Y7).

https://community.pixelmatix.com/t/mapping-assistance-32x16-p10/889/15

Hello, I’m new to this community and to Smartmatrix. But not in coding and interfacing things that are not suppose to.
BTW I’m a french ingineer.
I have in stock a number of 16x32 4S matrix.
They are divided in two types :
_ The one that works perfectly using SMARTMATRIX_HUB75_16ROW_32COL_MOD4SCAN_V3 model

    The ones that display a colmun of 4 or 3 pixels wtith MultiRowRefreshMapping test.

After sone further research on the second case, I found that A,B,C and D must be used this way
Line 0 : A=0, B=1, C=1, D=1, ( 7  - A) 0
Line 1 : A=1, B=0, C=1, D=1, ( B  - C) 2
Line 2 : A=1, B=1, C=0, D=1, ( C  - B) 4
Line 4 : A=1, B=1, C=1, D=0, ( D  - 7) 8

use R1,G1,B1 for the 4 first lines (0 to 3)
use R2,G2,B2 for the 4 last lines (4 to 7)

Just have to implement this logic in Smartmatrix.

My regular Matrix use a ship : LEDASIC PR4538 to do this job on the card by decoding A and B signal and split then in 4 outputs (line select)

Hope this wil help you.
I Have a piece of code that run on a ESP32 to prove this algorithm.
Best regards
Eric





Section of text on page 11 is as follows:
Inside the RUL6024 is a 16-bit shift register. Multiple RUL6024 serial data shifts. Each clock cycle CLK transfers 1 bit of data.

SDI, serial data input driver on/off control. Schmidt buffered input. When data "1" is written to SDI the switch controls the shift
Register / on the *rising* edge of CLK.

CLK serial data shift clock. Schmidt buffered input. All data/off control transitions are synchronized by the MSB of 1

On the rising edge of CLK, single-channel data is shifted to SD at the same time. After the rising edge input of CLK is approved, the rising edge lasts for 100ns.

**The number of CLK rising edges contained in the LE high level triggers different instructions. **

When the LE high level contains 3 CLKs, the data in the shift register it is latched into the latch and moved into the display register when waiting for the next display. 

Similarly, when LE contains 11 CLKs, the shift register the data is written into the status register 1. 

When the LED contains 12 CLKs, the shift register data is moved into the status register 2. 

By rewriting the status the value of the register can configure some parameters of display and blanking.

The rising edge of OE moves the value in the latch into the display register, and OE serves as a display enable. When OE is low, the display is turned on, and the data Ports with a register of 1 will be enabled.

SDO is the output of the shift register. When the shift data exceeds 16, the first data moved in by SDI will be shifted out as the next
Chip input.


Hi Jakub,

It's been quite some time since I last got in touch. After the first 64x64 matrix panel worked out of the box with my driver, I tried to control the other two Matrix panels as well.
I did start to work on the 32x16 Matrix Panel but came to the insight that I first have to look at the matrix panels hardware.  One type of chip used on this matrix panel is the 74HC138B. During my internet research, I came across a data sheet for 74HC138; 74HCT138, a  3-to-8-line decoder/demultiplexer; inverting. The 74HC138; 74HCT138 decodes three binary-weighted address inputs (A0, A1, and A2) to eight mutually exclusive outputs (Y0 to Y7).
I could not find a data sheet for the 74HC138B chip. I assume that this chip is a variant of the chip types mentioned above and has also 3 row select pins which allows 8 rows to be addressed.
Based on this assumption the 32x16 panel should be multiplexing 4 rows simultaneously.
After making some adjustments to the current driver, I can address all pixels of the matrix. The pixel mapping still needs to be finalized. I still need to think about the exact mappings so that circles, lines, etc. are displayed correctly. Once this is done, I need to deal with ghosting, as this has confused me quite a bit.

For the last 64x64 matrix panel I could not lit any pixel up to now. Does the panel expect a certain voltage ? Do you know a driver with which this matrix panel is working ?

Kind regards
Jürgen

There had been several impediments to be surmounted with this 32x16 matrix panel.
- I couldn't find a data sheet for the 74HC138**B** chip. I had assumed that this chip was a variant of the 74HC138 and 74HCT138 chip types, which are 3-to-8-line decoders/demultiplexers. If the 74HC138**B** chip were the same as these chips, the matrix panel would perform 2-wire multiplexing and use 3 pins for row addressing. However, since this 32x16 matrix panel performs 4-wire multiplexing, it only needs to address 4 lines to cover all 16 rows, which is possible with 2 address pins. This result is reflected in the code with this definition `#define ROWSEL_N_PINS 2`.
- It took me some time to understand and program the pixel mapping for this matrix panel. During development, I had to reduce the system clock frequency to 25,000 Hz. Ghosting misled me several times regarding the pixel mapping of the matrix panel.
- I conducted some experiments in the .program hub75_row method in the hub75.pio file to reduce ghosting. But this is based on guesswork. More research needs to be done to find out how this matrix panel can handle higher system clock frequencies. If that fails one way to go might be to reduce DMA or PIO speed to make the matrix panel independent of the system clock.

A test version for this matrix panel is available in the (temporary) branch []().





	
	

	

