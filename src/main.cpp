#include <nrf.h>
#include <stddef.h>
#include <core_cm4.h>

#define LED_PIN_NUM 9                // P1.09
#define LED_PIN     ((1 << 5) | LED_PIN_NUM)  // 41 = (1 * 32 + 9)

static uint16_t pwm_seq[1] = {0};   // デューティ比用バッファ

void delay_us(uint32_t us) {
    SysTick->LOAD = (SystemCoreClock / 1000000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;

    for (uint32_t i = 0; i < us; ++i) {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
    }

    SysTick->CTRL = 0;
}

void delay_ms(uint32_t ms) {
    while (ms--) {
        delay_us(1000);
    }
}

// malloc/free ラッパー
extern "C" void *__wrap__malloc_r(void *, size_t size) {
    extern void *malloc(size_t);
    return malloc(size);
}
extern "C" void __wrap__free_r(void *, void *ptr) {
    extern void free(void *);
    free(ptr);
}
extern "C" void *__wrap__realloc_r(void *, void *ptr, size_t size) {
    extern void *realloc(void *, size_t);
    return realloc(ptr, size);
}
extern "C" void *__wrap__calloc_r(void *, size_t n, size_t size) {
    extern void *calloc(size_t, size_t);
    return calloc(n, size);
}

void pwm_init(void) {
    NRF_P1->PIN_CNF[9] = (1 << 1);  // 出力

    NRF_PWM0->PSEL.OUT[0] = ((1 << 5) | 9) | (0 << 31);  // P1.09に接続
    NRF_PWM0->ENABLE = 1;
    NRF_PWM0->MODE = PWM_MODE_UPDOWN_Up;
    NRF_PWM0->PRESCALER = PWM_PRESCALER_PRESCALER_DIV_16;  // 1MHz
    NRF_PWM0->COUNTERTOP = 1000;  // 周期 = 1ms = 1kHz

    NRF_PWM0->SEQ[0].PTR = (uint32_t)pwm_seq;
    NRF_PWM0->SEQ[0].CNT = 1;
    NRF_PWM0->SEQ[0].REFRESH = 0;
    NRF_PWM0->SEQ[0].ENDDELAY = 0;

    NRF_PWM0->DECODER = (PWM_DECODER_LOAD_Individual << PWM_DECODER_LOAD_Pos) |
                        (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    NRF_PWM0->LOOP = 0;
    NRF_PWM0->SHORTS = PWM_SHORTS_LOOPSDONE_SEQSTART0_Enabled << PWM_SHORTS_LOOPSDONE_SEQSTART0_Pos;

    NRF_PWM0->TASKS_SEQSTART[0] = 1;
}



int main(void) {
    pwm_init();  // PWM初期化（COUNTERTOP=1000など）

    while (1) {
        // Duty を 0 → 1000 に1秒かけて上げる（50ステップ × 20ms = 1s）
        for (int duty = 0; duty <= 1000; duty += 20) {
            pwm_seq[0] = duty;
            NRF_PWM0->TASKS_SEQSTART[0] = 1;
            delay_ms(20);
        }

        // Duty を 1000 → 0 に1秒かけて下げる
        for (int duty = 1000; duty >= 0; duty -= 20) {
            pwm_seq[0] = duty;
            NRF_PWM0->TASKS_SEQSTART[0] = 1;
            delay_ms(20);
        }
    }

    return 0;
}

