#include "system_init.h"
#include "gpio.h"
#include "tim.h"

volatile uint16_t timestamp = 0;

int main(void)
{
    system_init();

    /*-------------------------------
     * TIM2 : Output Compare (PA0)
     *------------------------------*/
    gpio_pin_config(GPIOA,
                    GPIO_PIN_0,
                    GPIO_MODE_AF_PP_50MHZ);

    tim_init_t tim2_cfg =
    {
        .instance = TIM2,
        .prescaler = 7200,
        .period = 10000,
        .auto_reload_preload = 0
    };

    tim_init(&tim2_cfg);

    /* Toggle output on Channel 1 */
    tim_oc_channel_init(
        TIM2,
        TIM_CHANNEL_1,
        TIM_OCMODE_TOGGLE,
        0);

    tim_start(TIM2);

    /*-------------------------------
     * TIM3 : Input Capture (PA6)
     *------------------------------*/
    gpio_pin_config(GPIOA,
                    GPIO_PIN_6,
                    GPIO_MODE_INPUT_FLOATING);

    tim_init_t tim3_cfg =
    {
        .instance = TIM3,
        .prescaler = 7200,
        .period = 65535,
        .auto_reload_preload = 0
    };

    tim_init(&tim3_cfg);

    tim_ic_channel_init(TIM3,
                        TIM_CHANNEL_1,
                        TIM_IC_EDGE_RISING);

    tim_start(TIM3);

    /*-------------------------------
     * Main Loop
     *------------------------------*/
    while (1)
    {
        if (tim_ic_data_ready(TIM3, TIM_CHANNEL_1))
        {
            timestamp = tim_ic_read(TIM3, TIM_CHANNEL_1);
        }
    }
}
