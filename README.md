# FK743M2-IIT6 + 7" RGB LCD 

A minimal **STM32CubeIDE** bring-up project for the **FK743M2-IIT6** development board featuring the **STM32H743IIT6**, external **SDRAM**, and a **7-inch 800x480 RGB LCD**.

This repository provides a **known-good starting point** for this specific board/display combination. Because documentation is scarce, this project aims to save developers significant debugging time by providing a validated hardware configuration.

This is NOT "vibe coded" — it is a validated hardware configuration designed to save debugging time on LTDC timing, FMC signal integrity, DMA2D integration, and LVGL bring-up.

---

## 🚀 Overview

This demo validates the complete low-level display path:

* **Core:** STM32H743IIT6 initialization at 480MHz.
* **Memory:** FMC + External SDRAM setup.
* **Display:** LTDC configuration & RGB LCD output.
* **Control:** PWM backlight and GPIO signal integrity.
* **Rendering:** Framebuffer validation (Color Bars) + DMA2D accelerated blitting.
* **UI:** LVGL 9.x integration with partial rendering and double buffering.
* **Touch:** GT911 capacitive touch controller via bit-banged I2C.
* **Flash:** W25Q64JV QSPI flash with memory-mapped mode for instant splash image display.

---

## 🛠 Hardware Specifications

| Component    | Detail                                          |
|--------------|-------------------------------------------------|
| MCU          | STM32H743IIT6 (ARM Cortex-M7, up to 480MHz)    |
| Board        | FK743M2-IIT6                                    |
| SDRAM        | W9825G6KH (32MB: 4M x 4 banks x 16 bits)       |
| QSPI Flash   | W25Q64JV (8MB, 133MHz max)                      |
| LCD Panel    | 7-inch TFT LCD (800x480)                        |
| Interface    | 24-bit Parallel RGB via LTDC                    |
| Touch        | Goodix GT911 (bit-banged I2C, address 0x5D)     |

---

## ⚡ Key Findings & Critical Settings

### 1. GPIO Speed Requirement

**LTDC GPIO speed must be set to "Very High Speed".**
Using "Low" or "Medium" speeds results in no image, unstable colors, or significant artifacts due to the high pixel clock frequency. The same applies to QSPI GPIO pins — all must be `GPIO_SPEED_FREQ_VERY_HIGH`.

### 2. Why RGB565?

While the hardware supports ARGB8888, **RGB565** was chosen because it offers lower bandwidth requirements, uses 50% less memory, and provides better stability during early bring-up.

### 3. DMA2D — Critical HAL Bug on STM32H7

`HAL_DMA2D_PollForTransfer()` returns **before the AXI bus transaction has fully completed to SDRAM**. The HAL flag clears early, but data may not yet be visible to the LTDC controller.

**Always follow `HAL_DMA2D_PollForTransfer()` with:**

```c
while (DMA2D->CR & DMA2D_CR_START);  /* poll hardware START bit directly */
__DSB();                               /* data synchronization barrier */
```

Without this, partial renders, black bars, and repeated UI elements will appear — especially with double buffering.

### 4. DMA2D OutputOffset is Required for Partial Renders

When flushing a partial rectangle to the framebuffer, DMA2D must be told how many pixels to skip at the end of each row in the **destination**:

```c
hdma2d.Init.OutputOffset = LCD_HOR_RES - w;  /* pixels to skip per row */
```

`OutputOffset = 0` tells DMA2D the destination is a contiguous buffer, producing scrambled output.

### 5. LVGL Double Buffer Race Condition

With two render buffers, LVGL renders into buffer B while DMA2D flushes buffer A. If DMA2D completes (according to HAL) but hasn't actually finished on the bus, LVGL overwrites buffer data mid-transfer. The fix from point 3 (`while (DMA2D->CR & DMA2D_CR_START); __DSB();`) resolves this before calling `lv_display_flush_ready()`.

### 6. QSPI Memory-Mapped Mode Locks the Peripheral

Once `HAL_QSPI_MemoryMapped()` is called, no indirect commands (`QSPI_ReadData`, `QSPI_WriteEnable`, etc.) can be issued. Always call `HAL_QSPI_Abort()` followed by `MX_QUADSPI_Init()` to return to indirect mode.

---

## 📺 Display Configuration

### Timings

The following values are optimized for the 800x480 panel:

| Parameter | Value | Parameter | Value |
|-----------|-------|-----------|-------|
| HSYNC     | 10    | VSYNC     | 10    |
| HBP       | 46    | VBP       | 33    |
| HFP       | 210   | VFP       | 22    |

### Framebuffer Mapping

The framebuffer is mapped to the beginning of external SDRAM Bank 1:

```
LCD Framebuffer:  0xC0000000  (800 x 480 x 2 = 768KB)
LVGL Buf 1:       0x24060000  (AXI SRAM, non-cacheable)
LVGL Buf 2:       0x24070000  (AXI SRAM, non-cacheable)
```

---

## 🏃 Demo Behavior

Upon startup, the firmware performs the following sequence:

1. **MPU Configuration:** Sets memory attributes for SDRAM and AXI SRAM (non-cacheable regions for DMA2D buffers).
2. **Peripherals:** Initializes HAL, System Clock (HSE, 480MHz), FMC, LTDC, DMA2D, QSPI.
3. **Backlight:** Starts PWM at 2kHz to enable the display backlight.
4. **Color Bars:** Fills the SDRAM framebuffer with a vertical color bar pattern to validate the display path.
5. **QSPI Splash:** Checks QSPI flash for a magic number (`0xDEADBEEF`). If not present and SD card is available, programs `splash.bin` from SD card to QSPI flash. On all subsequent boots, displays the splash image instantly via DMA2D memory-mapped read — no SD card required.
6. **LVGL Init:** Initializes LVGL 9.x with partial rendering mode and DMA2D-accelerated flush callback.
7. **Touch Init:** Initializes GT911 via bit-banged I2C and registers it as an LVGL input device.
8. **UI:** Loads the EEZ Studio generated UI.
9. **Heartbeat:** Toggles the onboard User LED (PH7) every 500ms.

---

## 🖥 LVGL Integration

LVGL 9.x runs in `LV_DISPLAY_RENDER_MODE_PARTIAL` with two 40-line render buffers (800 × 40 × 2 bytes each) located in non-cacheable AXI SRAM.

The `flush_cb` uses DMA2D for hardware-accelerated blitting:

```c
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;

    uint32_t dst = LCD_FB_ADDR
                 + ((uint32_t)area->y1 * LCD_HOR_RES + (uint32_t)area->x1)
                   * sizeof(uint16_t);

    hdma2d.Init.OutputOffset       = LCD_HOR_RES - w;
    hdma2d.LayerCfg[1].InputOffset = 0;
    hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;

    HAL_DMA2D_Init(&hdma2d);
    HAL_DMA2D_ConfigLayer(&hdma2d, 1);

    HAL_DMA2D_Start(&hdma2d, (uint32_t)px_map, dst, (uint32_t)w, (uint32_t)h);
    HAL_DMA2D_PollForTransfer(&hdma2d, HAL_MAX_DELAY);

    /* HAL flag clears early on STM32H7 — poll hardware directly */
    while (DMA2D->CR & DMA2D_CR_START);
    __DSB();

    lv_display_flush_ready(disp);
}
```

---

## 💾 QSPI Flash Splash Image

The W25Q64JV is used to store a splash image for instant boot display:

- **Magic number** (`0xDEADBEEF`) at flash address `0x000000` signals that the flash has been programmed.
- **Image data** starts at `0x001000` (sector 1), preserving sector 0 for metadata.
- On first boot with SD card present, `splash.bin` is automatically programmed page by page.
- On all subsequent boots, memory-mapped mode is enabled and DMA2D copies directly from `0x90001000` to the framebuffer — skipping the 12-byte LVGL image header.

```
QSPI Flash layout:
  0x000000  — Magic number (4 bytes)
  0x001000  — splash.bin image data (768,012 bytes)
```

---

## 👆 Touch Support

The GT911 capacitive touch controller is driven via **bit-banged I2C** because the touch pins are not connected to a hardware I2C peripheral on this board:

| Signal      | Pin  |
|-------------|------|
| TOUCH_SDA   | PI8  |
| TOUCH_SCLK  | PI11 |
| TOUCH_RST   | PH4  |
| TOUCH_INT   | PG3  |

The GT911 is configured at I2C address `0x5D` (INT held low during reset). It is registered as an LVGL pointer input device via `lv_indev_create()`.

---

## 📁 Project Structure

- `Core/Src/main.c` — Application logic, DMA2D flush, QSPI splash, LVGL init.
- `Core/Src/gt911.c` — GT911 bit-banged I2C touch driver.
- `Core/Src/stm32h7xx_hal_msp.c` — GPIO, clock, and peripheral MSP initialization.
- `*.ioc` — STM32CubeMX configuration file.

---

## 🛠 How to Build & Run

1. Open the project in **STM32CubeIDE**.
2. Build the project (`Project → Build All`).
3. Flash to the FK743M2 board via SWD.
4. On first boot with SD card containing `splash.bin`: the image is programmed to QSPI flash automatically.
5. **Observe:**
   - Color bars for 2 seconds
   - Splash image displayed instantly from QSPI
   - LVGL UI loads with touch support active
   - PH7 LED blinking every 500ms

---

## 🔍 Troubleshooting

| Issue | Potential Cause |
|-------|-----------------|
| Blank screen | Check LTDC GPIO speeds (must be Very High) or timing values |
| Wrong colors | Verify pixel format (RGB565) and pin mapping |
| DMA2D artifacts / black bars | Missing `while (DMA2D->CR & DMA2D_CR_START); __DSB();` after PollForTransfer |
| LVGL renders multiple times | Double buffer race — DMA2D not truly complete before `lv_display_flush_ready()` |
| No backlight | Check PWM signal on backlight enable pin (PH6) |
| QSPI not responding | Check GPIO alternate function and speed — all QSPI pins must be `GPIO_SPEED_FREQ_VERY_HIGH` |
| Splash not showing | Verify magic number in flash; check `HAL_QSPI_Abort()` + reinit after memory-mapped mode |
| HardFault after QSPI | Call `MX_QUADSPI_Init()` after `HAL_QSPI_Abort()` to fully reinitialize the peripheral |
| Touch not working | Verify GT911 I2C address (INT low during reset = 0x5D); check reset sequence timing |

---

# FK743M2-IIT6 + LVGL 9.4 Integration Notes

## Hardware
| Component | Detail |
|---|---|
| MCU | STM32H743IIT6 @ 480MHz |
| Board | FK743M2-IIT6 (反客科技) |
| Display | 800x480 RGB LCD (MY-RGB 40-pin) |
| Touch IC | Goodix GT911 (bit-banged I2C) |
| SDRAM | W9825G6 (32MB @ 0xC0000000) |
| Flash | 2MB internal |
| SD Card | microSD via SDMMC1 |

---

## Critical Fixes & Lessons Learned

### 1. `-mno-unaligned-access` Compiler Flag — MANDATORY

**Problem:** LVGL 9.x changed `lv_color_t` to a **3-byte struct** (RGB888 internally)
regardless of `LV_COLOR_DEPTH`. With `LV_COLOR_DEPTH 16`, the struct is still
3 bytes. When the compiler emits `strh` (store halfword) instructions to
3-byte-aligned struct fields, Cortex-M7 raises a HardFault (UNDEFINSTR).

**Symptom:** HardFault immediately on `lv_display_create()` inside
`lv_theme_default_init()` at a `strh` instruction.

**Fix:** Add to compiler flags in STM32CubeIDE:
```
Project → Properties → C/C++ Build → Settings
→ MCU GCC Compiler → Miscellaneous → Other flags
→ Add: -mno-unaligned-access
```

This forces GCC to use byte-by-byte stores for unaligned fields,
eliminating the fault. Slightly slower but functionally correct.

---

### 2. Stack Must Be in Cacheable RAM Region

**Problem:** `_estack` defaulting to `ORIGIN(RAM_D1) + LENGTH(RAM_D1)` places
the stack at `0x2407FFFF` — inside the non-cacheable LVGL buffer region
(`0x24060000–0x2407FFFF`). On Cortex-M7, non-cacheable stack writes cause
speculative prefetch to read stale LR values → `lr = 0x0` → UNDEFINSTR HardFault.

**Symptom:** `lr = 0x0` on every HardFault, crash inside LVGL functions.

**Fix:** In `STM32H743IITX_FLASH.ld`:
```ld
/* WRONG — puts stack in non-cacheable region */
_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1);

/* CORRECT — stack stays in cacheable region */
_estack = 0x24060000;
```

---

### 3. MPU Configuration

AXI SRAM (RAM_D1, 512KB) is split into two zones via MPU:

| Address Range | Size | Cache | Purpose |
|---|---|---|---|
| `0x24000000–0x2405FFFF` | 384KB | Cacheable (WT) | .data, .bss, LVGL heap, stack |
| `0x24060000–0x2407FFFF` | 128KB | NON-cacheable | LVGL render buffers |

The split is achieved using `SubRegionDisable = 0xC0` on the 512KB cacheable
region (disables top 2 sub-regions of 64KB each), with a higher-priority
non-cacheable region covering `0x24060000`.

**SDRAM framebuffer must be NON-cacheable** — LTDC reads directly via AHB
and would see stale cached data otherwise.

**Peripherals must use Device memory** (TEX=0, S=1, B=1) — not Normal memory.

---

### 4. LVGL Render Buffers at Fixed Addresses

LVGL render buffers must be in the non-cacheable MPU region.
Using linker section attributes (`__attribute__((section(".lvgl_buf")))`)
caused heap/buffer collisions. The reliable solution is fixed absolute pointers:

```c
#define LVGL_BUF_1_ADDR  0x24060000UL
#define LVGL_BUF_2_ADDR  0x24070000UL

uint8_t *buf1 = (uint8_t *)LVGL_BUF_1_ADDR;
uint8_t *buf2 = (uint8_t *)LVGL_BUF_2_ADDR;
```

---

### 5. LVGL Heap Size

With `LV_MEM_ADR 0`, LVGL heap lives in `.bss` in RAM_D1.
`_ebss ≈ 0x24020B10` leaves ~253KB before the non-cacheable region.

```c
#define LV_MEM_SIZE (256U * 1024U)   /* safe maximum — do not exceed */
```

Going above 256KB causes the heap to spill into the non-cacheable region,
corrupting LVGL internal state and causing HardFaults.

---

### 6. Do NOT Call `lv_theme_default_init()` Manually

`lv_display_create()` calls `lv_theme_default_init()` internally.
Calling it again from EEZ-generated `screens.c` or `ui.c` causes
double-initialization, heap corruption and HardFault.

**Fix:** Delete these lines from EEZ-generated `create_screens()`:
```c
// DELETE these — lv_display_create() already handles theme init
lv_display_t *dispp = lv_display_get_default();
lv_theme_t *theme = lv_theme_default_init(dispp, ...);
lv_display_set_theme(dispp, theme);
```

---

### 7. LTDC GPIO Speed Must Be VERY HIGH

CubeMX regeneration resets LTDC GPIO pins from `GPIO_SPEED_FREQ_VERY_HIGH`
to `GPIO_SPEED_FREQ_LOW`. At 32MHz pixel clock, low-speed GPIO cannot
switch fast enough — the display goes blank.

**After every CubeMX regeneration**, restore `stm32h7xx_hal_msp.c` from
backup, or manually set all LTDC pins back to `GPIO_SPEED_FREQ_VERY_HIGH`.

---

### 8. GT911 Touch Controller (Bit-Banged I2C)

Touch pins are NOT connected to hardware I2C on this board:

| Signal | Pin | Direction |
|---|---|---|
| TOUCH_SDA | PI8 | Bit-bang I2C |
| TOUCH_SCLK | PI11 | Bit-bang I2C |
| TOUCH_RST | PH4 | Output |
| TOUCH_INT | PG3 | Input |

I2C address is selected by INT pin state during reset:
- INT low during reset → address `0x5D`
- INT high during reset → address `0x14`

This board uses `0x5D`.

---

### 9. SD Card / FatFS

- Format: **FAT32** (use guiformat.exe for cards >32GB on Windows)
- `MX_SDMMC1_SD_Init()` must be called **before** `MX_FATFS_Init()`
- Start with `ClockDiv = 16`, 1-bit bus for bring-up; increase speed after stable
- `MX_FATFS_Init()` mounts internally — call `f_mount()` with
  `f_mount(NULL, SDPath, 0)` first to unmount, then remount cleanly
- LVGL filesystem driver letter: `'S'` → use `"S:/image.bin"` in image paths

---

### 10. LVGL Binary Image Format (v9)

The LVGL v9 online converter only outputs C arrays.
For SD card binary files, the v9 header format is:

```
Offset 0:  uint8_t  magic  = 0x19
Offset 1:  uint8_t  cf     = 4 (RGB565)
Offset 2:  uint16_t flags  = 0
Offset 4:  uint16_t width
Offset 6:  uint16_t height
Offset 8:  uint16_t stride = width * 2
Offset 10: uint16_t reserved = 0
Offset 12: pixel data (RGB565, little-endian)
```

Use the Python LVGL image converter for correct v9 binary output:
```bash
pip install lvgl-image-converter
lv_img_conv image.png -f RGB565 -o binary --lvgl-v9
```

---

### 11. CubeMX Regeneration Checklist

Every time CubeMX regenerates code, check and restore:

| File | What gets reset | Fix |
|---|---|---|
| `STM32H743IITX_FLASH.ld` | `_estack` reset to end of RAM_D1 | Restore `_estack = 0x24060000` |
| `stm32h7xx_hal_msp.c` | LTDC GPIO speed reset to LOW | Restore from backup |
| `main.c` | All custom code outside USER CODE blocks | Restore from backup |

**Always backup before regenerating.**

---

## Memory Map Summary

```
0x08000000  2MB    FLASH        Code, rodata, vectors (cacheable, executable)
0x20000000  128KB  DTCMRAM      Available (non-cacheable, tightly coupled)
0x24000000  384KB  RAM_D1 code  .data, .bss, LVGL heap, stack (cacheable)
0x24060000  64KB   LVGL buf 1   Render buffer 1 (NON-cacheable)
0x24070000  64KB   LVGL buf 2   Render buffer 2 (NON-cacheable)
0x30000000  288KB  RAM_D2       General purpose (cacheable)
0x38000000  64KB   RAM_D3       General purpose (cacheable)
0x40000000  512MB  Peripherals  Device memory (shareable, bufferable)
0xC0000000  32MB   SDRAM        LTDC framebuffer (NON-cacheable)
```

---

## Build Flags

```
-mcpu=cortex-m7
-mfpu=fpv5-d16
-mfloat-abi=hard
-mno-unaligned-access    ← CRITICAL for LVGL 9.x
-mthumb
```

---

## Software Versions

| Software | Version |
|---|---|
| STM32CubeIDE | 2.1.1 |
| STM32CubeH7 FW | V1.13.0 |
| LVGL | 9.4.0 |
| EEZ Studio | Latest |
| GCC ARM | 12.3 (arm-none-eabi) |



## 📄 License

This project is shared as a practical reference for the community. Use, modify, and improve it as needed.

GPL-2.0 License — see [LICENSE](LICENSE) for details.
