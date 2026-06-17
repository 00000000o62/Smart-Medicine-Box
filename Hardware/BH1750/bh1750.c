/*
 * BH1750 ambient light sensor driver
 * Software I2C (bit-bang) on PA12(SCL) / PA15(SDA) @ ~100kHz
 * CH32V307 @ 96MHz, calibrated loop delay.
 */

#include "bh1750.h"
#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"
#include "debug.h"

#define SCL_H()  GPIO_SetBits(BH_SCL_PORT, BH_SCL_PIN)
#define SCL_L()  GPIO_ResetBits(BH_SCL_PORT, BH_SCL_PIN)
#define SDA_H()  GPIO_SetBits(BH_SDA_PORT, BH_SDA_PIN)
#define SDA_L()  GPIO_ResetBits(BH_SDA_PORT, BH_SDA_PIN)
#define SDA_IN() GPIO_ReadInputDataBit(BH_SDA_PORT, BH_SDA_PIN)

static GPIO_InitTypeDef g;

/* ~5µs delay for 100kHz I2C half-period */
static void i2c_delay(void)
{
    uint32_t n = 160;     /* ~160*3cyc ≈ 480cyc ≈ 5µs @ 96MHz */
    while(n--) __asm__ volatile("nop");
}

static void sda_out(void)
{
    g.GPIO_Pin   = BH_SDA_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BH_SDA_PORT, &g);
}

static void sda_in(void)
{
    g.GPIO_Pin  = BH_SDA_PIN;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(BH_SDA_PORT, &g);
}

static void i2c_start(void)
{
    sda_out();
    SDA_H(); SCL_H(); i2c_delay();
    SDA_L(); i2c_delay();
    SCL_L();
}

static void i2c_stop(void)
{
    sda_out();
    SDA_L(); SCL_H(); i2c_delay();
    SDA_H(); i2c_delay();
}

static uint8_t i2c_write_byte(uint8_t byte)
{
    uint8_t i, ack;
    for(i = 0; i < 8; i++)
    {
        if(byte & 0x80) SDA_H(); else SDA_L();
        byte <<= 1;
        i2c_delay();
        SCL_H(); i2c_delay();
        SCL_L();
    }
    /* ACK */
    sda_in();
    i2c_delay();
    SCL_H(); i2c_delay();
    ack = (SDA_IN() == 0);
    SCL_L();
    sda_out();
    return ack;
}

static uint8_t i2c_read_byte(uint8_t ack)
{
    uint8_t i, byte = 0;
    sda_in();
    for(i = 0; i < 8; i++)
    {
        SCL_H(); i2c_delay();
        byte = (byte << 1) | (SDA_IN() ? 1 : 0);
        SCL_L(); i2c_delay();
    }
    /* ACK/NACK */
    sda_out();
    if(ack) SDA_L(); else SDA_H();
    i2c_delay();
    SCL_H(); i2c_delay();
    SCL_L();
    SDA_H();
    return byte;
}

static void bh1750_write_cmd(uint8_t cmd)
{
    i2c_start();
    i2c_write_byte(BH1750_ADDR << 1);    /* write address */
    i2c_write_byte(cmd);
    i2c_stop();
}

void BH1750_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* SCL = PA12, output push-pull, HIGH idle */
    g.GPIO_Pin   = BH_SCL_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BH_SCL_PORT, &g);
    SCL_H();

    /* SDA = PA15, output push-pull, HIGH idle */
    sda_out();
    SDA_H();

    /* Power on */
    bh1750_write_cmd(0x01);
    Delay_Ms(10);

    /* Continuous High Resolution mode (1lx, 120ms) */
    bh1750_write_cmd(0x10);
    Delay_Ms(180);  /* first measurement takes 120-180ms */

    printf("BH1750: init done\r\n");
}

/* Read lux. Returns 1 on success, 0 on error (no ACK). */
uint8_t BH1750_Read(float *lux)
{
    uint8_t hi, lo;

    i2c_start();
    if(!i2c_write_byte((BH1750_ADDR << 1) | 1))   /* read address */
    {
        i2c_stop();
        return 0;
    }
    hi = i2c_read_byte(1);    /* ACK after first byte */
    lo = i2c_read_byte(0);    /* NACK after last byte */
    i2c_stop();

    uint16_t raw = ((uint16_t)hi << 8) | lo;
    *lux = (float)raw / 1.2f;

    return 1;
}
