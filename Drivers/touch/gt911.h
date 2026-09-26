#ifndef GT911_H
#define GT911_H

#include "main.h"
//#include "lvgl.h"

/* -----------------------------------------------------------------------
 * GT911 I2C address
 * Address is selected during reset by the state of INT pin:
 *   INT low  during reset → 0x5D (7-bit) → 0xBA/0xBB (8-bit w/ R/W)
 *   INT high during reset → 0x14 (7-bit) → 0x28/0x29 (8-bit w/ R/W)
 * On FK743M2-IIT6 we hold INT low → use 0x5D
 * ----------------------------------------------------------------------- */
#define GT911_I2C_ADDR          0x5D

/* GT911 register addresses */
#define GT911_REG_CONFIG        0x8047
#define GT911_REG_PRODUCT_ID    0x8140
#define GT911_REG_STATUS        0x814E
#define GT911_REG_POINT1        0x814F

/* Maximum number of touch points GT911 supports */
#define GT911_MAX_POINTS        5

/* Touch screen physical resolution — must match your LCD */
#define GT911_LCD_WIDTH         800
#define GT911_LCD_HEIGHT        480

/* GPIO pin definitions — from FK743M2-IIT6 schematic
 * TOUCH_SDA  → PI8
 * TOUCH_SCLK → PI11
 * TOUCH_RST  → PH4
 * TOUCH_INT  → PG3  */
#define GT911_SDA_PIN           GPIO_PIN_8
#define GT911_SDA_PORT          GPIOI
#define GT911_SCL_PIN           GPIO_PIN_11
#define GT911_SCL_PORT          GPIOI
#define GT911_RST_PIN           GPIO_PIN_4
#define GT911_RST_PORT          GPIOH
#define GT911_INT_PIN           GPIO_PIN_3
#define GT911_INT_PORT          GPIOG

/* Public API */
void     gt911_init(void);
void     gt911_lvgl_indev_init(void);
uint8_t  gt911_read_touch(uint16_t *x, uint16_t *y);

#endif /* GT911_H */
