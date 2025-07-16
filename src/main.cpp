#include <nrf.h>
#include <stddef.h>
#include <core_cm4.h>

#define PWM_PIN (41UL)                // P1.09 = 32 + 9

#define STEP_COUNT 50
#define STEP_DELAY_MS 100

uint16_t buf[] = {0};  // 初期デューティ比0%

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
    // Start accurate HFCLK (XOSC)
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) ;
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  
  // Configure PWM_PIN as output, and set it to 0
  NRF_GPIO->DIRSET = (1ULL << PWM_PIN);
  NRF_GPIO->OUTCLR = (1ULL << PWM_PIN);
  
  
  NRF_PWM0->PRESCALER   = PWM_PRESCALER_PRESCALER_DIV_16; // 1 us
  NRF_PWM0->PSEL.OUT[0] = PWM_PIN;
  NRF_PWM0->MODE        = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);
  NRF_PWM0->DECODER     = (PWM_DECODER_LOAD_Common       << PWM_DECODER_LOAD_Pos) | 
                          (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
  NRF_PWM0->LOOP        = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);
  
  NRF_PWM0->COUNTERTOP = 20000; // 20ms period
  
  
  NRF_PWM0->SEQ[0].CNT = ((sizeof(buf) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);
  NRF_PWM0->SEQ[0].ENDDELAY = 0;
  NRF_PWM0->SEQ[0].PTR = (uint32_t)&buf[0];
  NRF_PWM0->SEQ[0].REFRESH = 0;
  NRF_PWM0->SHORTS = 0;
  
  NRF_PWM0->ENABLE = 1;
  NRF_PWM0->TASKS_SEQSTART[0] = 1;
}

int main(void) {
    pwm_init();  // PWM初期化（COUNTERTOP=1000など）

    while (1) {
        // 上昇フェード（0 → 20000）
        for (int i = 0; i <= STEP_COUNT; i++) {
            uint16_t duty = (NRF_PWM0->COUNTERTOP * i) / STEP_COUNT;
            buf[0] = duty;
            NRF_PWM0->TASKS_SEQSTART[0] = 1;
            delay_ms(STEP_DELAY_MS);
        }

        // 下降フェード（20000 → 0）
        for (int i = STEP_COUNT; i >= 0; i--) {
            uint16_t duty = (NRF_PWM0->COUNTERTOP * i) / STEP_COUNT;
            buf[0] = duty;
            NRF_PWM0->TASKS_SEQSTART[0] = 1;
            delay_ms(STEP_DELAY_MS);
        }
    }

    return 0;
}

