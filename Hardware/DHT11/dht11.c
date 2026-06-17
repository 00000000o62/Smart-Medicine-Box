/*
 * DHT11 single-wire driver for CH32V307 @ 96MHz
 *
 * Strategy:
 *   - 20ms start pulse:  Delay_Ms() (SysTick-based, reliable)
 *   - <100µs timing:     NOP busy-loop (addi+bnez = 2cyc/iter, us*48 ≈ 1µs)
 *   - No IRQ disable:    occasional SysTick jitter tolerated, checksum catches
 */
#include "dht11.h"
#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"
#include "debug.h"

DHT11_Data dht11 = {0};

#define DQ_SET() GPIO_SetBits(DHT11_PORT, DHT11_PIN)
#define DQ_CLR() GPIO_ResetBits(DHT11_PORT, DHT11_PIN)
#define DQ_RD()  GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)

static GPIO_InitTypeDef g;

static void dq_out(void)
{
    g.GPIO_Pin   = DHT11_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &g);
}

static void dq_in(void)
{
    g.GPIO_Pin  = DHT11_PIN;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_PORT, &g);
}

/*
 * NOP-based µs delay.
 * Assembler loop: addi + bnez = 2 cycles/iter.
 * 48 iters × 2 cyc = 96 cyc ≈ 1.0 µs @ 96 MHz.
 */
static void dht_us(uint32_t us)
{
    uint32_t n = us * 48;
    __asm__ volatile(
        "1:\n\t"
        "addi %0, %0, -1\n\t"
        "bnez %0, 1b"
        : "+r"(n)
    );
}

/* Wait for DQ == expect, timeout ~limit µs */
static uint8_t dht_wait(uint8_t expect, uint32_t limit_us)
{
    while(limit_us--) {
        if(DQ_RD() == expect) return 1;
        dht_us(1);
    }
    return 0;
}

void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    dq_out();
    DQ_SET();
}

static uint8_t DHT11_ReadByte(void)
{
    uint8_t i, byte = 0;
    for(i = 0; i < 8; i++)
    {
        if(!dht_wait(1, 80)) return 0xFF;
        dht_us(40);
        byte <<= 1;
        if(DQ_RD()) byte |= 1;
        if(!dht_wait(0, 80)) return 0xFF;
    }
    return byte;
}

uint8_t DHT11_Read(DHT11_Data *data)
{
    uint8_t buf[5], i;

    /* ── Start: 20ms LOW (SysTick-based, reliable) ── */
    dq_out();
    DQ_CLR();
    Delay_Ms(20);
    DQ_SET();
    dht_us(30);   /* 30µs HIGH using NOP */
    dq_in();

    /* ── DHT11 response: LOW 80µs → HIGH 80µs → LOW (data start) ── */
    if(!dht_wait(0, 120)) return 0;
    if(!dht_wait(1, 120)) return 0;
    if(!dht_wait(0, 120)) return 0;

    /* ── Read 40 bits ── */
    for(i = 0; i < 5; i++) {
        buf[i] = DHT11_ReadByte();
        if(buf[i] == 0xFF) return 0;
    }

    dq_out();
    DQ_SET();

    /* ── Checksum ── */
    if((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) {
        data->valid = 0;
        return 0;
    }

    data->humi_int = buf[0];  data->humi_dec = buf[1];
    data->temp_int = buf[2];  data->temp_dec = buf[3];
    data->checksum = buf[4];
    data->humidity    = (float)buf[0] + (float)buf[1] * 0.1f;
    data->temperature = (float)buf[2] + (float)buf[3] * 0.1f;
    data->valid = 1;
    return 1;
}
