/* =========================================================================
 * gt911.c — Goodix GT911 capacitive touch driver
 *
 * Board:    FK743M2-IIT6
 * CPU:      STM32H743
 * Touch IC: Goodix GT911
 *
 * I2C is bit-banged on GPIO because the touch pins are NOT connected
 * to hardware I2C on this board:
 *   TOUCH_SDA  → PI8
 *   TOUCH_SCLK → PI11
 *   TOUCH_RST  → PH4
 *   TOUCH_INT  → PG3
 *
 * I2C address: 0x5D (INT held low during reset sequence)
 * ========================================================================= */

#include "gt911.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Bit-bang I2C timing
 * At 480MHz SYSCLK with AHB/2 = 240MHz, each NOP ≈ 4ns.
 * We target ~100kHz I2C → half-period = 5us ≈ 1250 NOPs.
 * Using HAL_Delay(1) is safer and more portable.
 * ------------------------------------------------------------------------- */
#define I2C_DELAY()   do { \
    volatile uint32_t _d = 120; \
    while(_d--); \
} while(0)

/* -------------------------------------------------------------------------
 * GPIO helpers
 * ------------------------------------------------------------------------- */
static inline void SDA_HIGH(void) {
    HAL_GPIO_WritePin(GT911_SDA_PORT, GT911_SDA_PIN, GPIO_PIN_SET);
}
static inline void SDA_LOW(void) {
    HAL_GPIO_WritePin(GT911_SDA_PORT, GT911_SDA_PIN, GPIO_PIN_RESET);
}
static inline void SCL_HIGH(void) {
    HAL_GPIO_WritePin(GT911_SCL_PORT, GT911_SCL_PIN, GPIO_PIN_SET);
}
static inline void SCL_LOW(void) {
    HAL_GPIO_WritePin(GT911_SCL_PORT, GT911_SCL_PIN, GPIO_PIN_RESET);
}
static inline void RST_HIGH(void) {
    HAL_GPIO_WritePin(GT911_RST_PORT, GT911_RST_PIN, GPIO_PIN_SET);
}
static inline void RST_LOW(void) {
    HAL_GPIO_WritePin(GT911_RST_PORT, GT911_RST_PIN, GPIO_PIN_RESET);
}
static inline void INT_HIGH(void) {
    HAL_GPIO_WritePin(GT911_INT_PORT, GT911_INT_PIN, GPIO_PIN_SET);
}
static inline void INT_LOW(void) {
    HAL_GPIO_WritePin(GT911_INT_PORT, GT911_INT_PIN, GPIO_PIN_RESET);
}
static inline uint8_t SDA_READ(void) {
    return HAL_GPIO_ReadPin(GT911_SDA_PORT, GT911_SDA_PIN);
}

/* Configure SDA as output (for sending) or input (for reading ACK/data) */
static void SDA_OUTPUT(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GT911_SDA_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;   /* open-drain */
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GT911_SDA_PORT, &GPIO_InitStruct);
}

static void SDA_INPUT(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GT911_SDA_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GT911_SDA_PORT, &GPIO_InitStruct);
}

static void INT_OUTPUT(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GT911_INT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GT911_INT_PORT, &GPIO_InitStruct);
}

static void INT_INPUT(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GT911_INT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GT911_INT_PORT, &GPIO_InitStruct);
}

/* -------------------------------------------------------------------------
 * Bit-bang I2C primitives
 * ------------------------------------------------------------------------- */

static void i2c_start(void)
{
    SDA_OUTPUT();
    SDA_HIGH();
    SCL_HIGH();
    I2C_DELAY();
    SDA_LOW();
    I2C_DELAY();
    SCL_LOW();
    I2C_DELAY();
}

static void i2c_stop(void)
{
    SDA_OUTPUT();
    SDA_LOW();
    SCL_LOW();
    I2C_DELAY();
    SCL_HIGH();
    I2C_DELAY();
    SDA_HIGH();
    I2C_DELAY();
}

/* Returns 0 on ACK, 1 on NACK */
static uint8_t i2c_write_byte(uint8_t byte)
{
    SDA_OUTPUT();
    for (int i = 7; i >= 0; i--) {
        if (byte & (1 << i))
            SDA_HIGH();
        else
            SDA_LOW();
        I2C_DELAY();
        SCL_HIGH();
        I2C_DELAY();
        SCL_LOW();
        I2C_DELAY();
    }
    /* Read ACK */
    SDA_INPUT();
    I2C_DELAY();
    SCL_HIGH();
    I2C_DELAY();
    uint8_t ack = SDA_READ();
    SCL_LOW();
    I2C_DELAY();
    SDA_OUTPUT();
    return ack;   /* 0 = ACK, 1 = NACK */
}

static uint8_t i2c_read_byte(uint8_t send_ack)
{
    uint8_t byte = 0;
    SDA_INPUT();
    for (int i = 7; i >= 0; i--) {
        I2C_DELAY();
        SCL_HIGH();
        I2C_DELAY();
        if (SDA_READ())
            byte |= (1 << i);
        SCL_LOW();
    }
    /* Send ACK or NACK */
    SDA_OUTPUT();
    if (send_ack)
        SDA_LOW();
    else
        SDA_HIGH();
    I2C_DELAY();
    SCL_HIGH();
    I2C_DELAY();
    SCL_LOW();
    I2C_DELAY();
    SDA_HIGH();
    return byte;
}

/* -------------------------------------------------------------------------
 * GT911 register read/write
 * GT911 uses 16-bit register addresses, MSB first.
 * ------------------------------------------------------------------------- */

static uint8_t gt911_write_reg(uint16_t reg, uint8_t *data, uint8_t len)
{
    i2c_start();
    if (i2c_write_byte(GT911_I2C_ADDR << 1)) {   /* write address */
        i2c_stop();
        return 1;
    }
    i2c_write_byte((reg >> 8) & 0xFF);            /* register MSB */
    i2c_write_byte(reg & 0xFF);                   /* register LSB */
    for (uint8_t i = 0; i < len; i++)
        i2c_write_byte(data[i]);
    i2c_stop();
    return 0;
}

static uint8_t gt911_read_reg(uint16_t reg, uint8_t *data, uint8_t len)
{
    /* Write register address */
    i2c_start();
    if (i2c_write_byte(GT911_I2C_ADDR << 1)) {
        i2c_stop();
        return 1;
    }
    i2c_write_byte((reg >> 8) & 0xFF);
    i2c_write_byte(reg & 0xFF);
    i2c_stop();

    /* Read data */
    i2c_start();
    if (i2c_write_byte((GT911_I2C_ADDR << 1) | 0x01)) {
        i2c_stop();
        return 1;
    }
    for (uint8_t i = 0; i < len; i++) {
        data[i] = i2c_read_byte(i < (len - 1));  /* ACK all except last */
    }
    i2c_stop();
    return 0;
}

/* -------------------------------------------------------------------------
 * GT911 initialization
 *
 * Address selection via INT pin state during reset:
 *   INT low  → 0x5D
 *   INT high → 0x14
 *
 * Sequence (from GT911 datasheet):
 *   1. RST low, INT low (>10ms)
 *   2. RST high, hold INT for >100us to latch address
 *   3. Release INT (set as input)
 *   4. Wait >5ms for GT911 to boot
 * ------------------------------------------------------------------------- */
void gt911_init(void)
{
    /* Make sure GPIO clocks are enabled (already done in MX_GPIO_Init) */

    /* Configure SCL as output open-drain with pull-up */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GT911_SCL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GT911_SCL_PORT, &GPIO_InitStruct);

    /* Configure SDA as output open-drain with pull-up */
    SDA_OUTPUT();

    /* Step 1: Assert reset, set INT low to select address 0x5D */
    INT_OUTPUT();
    RST_LOW();
    INT_LOW();
    HAL_Delay(20);

    /* Step 2: Release reset, keep INT low to latch address */
    RST_HIGH();
    HAL_Delay(1);

    /* Step 3: Release INT — set as input */
    INT_INPUT();

    /* Step 4: Wait for GT911 to complete boot */
    HAL_Delay(50);

    /* Verify communication — read product ID (should be "911\0") */
    uint8_t product_id[5] = {0};
    gt911_read_reg(GT911_REG_PRODUCT_ID, product_id, 4);
    /* product_id should contain "911" if communication is working */
    (void)product_id;  /* suppress unused warning if not logging */
}

/* -------------------------------------------------------------------------
 * Read touch point
 * Returns number of active touch points (0 = no touch)
 * Fills x, y with first touch point coordinates
 * ------------------------------------------------------------------------- */
uint8_t gt911_read_touch(uint16_t *x, uint16_t *y)
{
    uint8_t status = 0;
    gt911_read_reg(GT911_REG_STATUS, &status, 1);

    /* Bit 7 = buffer status ready, bits 0-3 = number of touch points */
    if (!(status & 0x80)) {
        return 0;   /* buffer not ready */
    }

    uint8_t num_points = status & 0x0F;

    if (num_points > 0 && num_points <= GT911_MAX_POINTS) {
        /* Each touch point is 8 bytes:
         * byte 0:   track ID
         * byte 1-2: X (little-endian)
         * byte 3-4: Y (little-endian)
         * byte 5-6: size
         * byte 7:   reserved */
        uint8_t point_data[8];
        gt911_read_reg(GT911_REG_POINT1, point_data, 8);

        *x = point_data[1] | ((uint16_t)point_data[2] << 8);
        *y = point_data[3] | ((uint16_t)point_data[4] << 8);

        /* Clamp to screen bounds */
        if (*x >= GT911_LCD_WIDTH)  *x = GT911_LCD_WIDTH  - 1;
        if (*y >= GT911_LCD_HEIGHT) *y = GT911_LCD_HEIGHT - 1;
    }

    /* Clear status register so GT911 can write next frame */
    uint8_t clear = 0;
    gt911_write_reg(GT911_REG_STATUS, &clear, 1);

    return num_points;
}

/* -------------------------------------------------------------------------
 * LVGL input device callback
 * Called by lv_timer_handler() every refresh cycle
 * ------------------------------------------------------------------------- */
#ifdef OLD
static void gt911_lvgl_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    static uint16_t last_x = 0;
    static uint16_t last_y = 0;

    uint16_t x, y;
    uint8_t touched = gt911_read_touch(&x, &y);

    if (touched) {
        last_x = x;
        last_y = y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state   = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = last_x;
    data->point.y = last_y;
}

/* -------------------------------------------------------------------------
 * Register GT911 as LVGL touch input device
 * Call this AFTER lvgl_initialization() and AFTER gt911_init()
 * ------------------------------------------------------------------------- */
void gt911_lvgl_indev_init(void)
{
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, gt911_lvgl_read_cb);
}

#endif
