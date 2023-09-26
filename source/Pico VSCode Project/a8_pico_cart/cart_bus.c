#include "pico/stdlib.h"
#include "hardware/sync.h"

#include "cart_bus.h"

void config_gpio(void)
{
    gpio_init_mask(ALL_GPIO_MASK);

    gpio_set_dir_in_masked(ADDR_GPIO_MASK|DATA_GPIO_MASK|CCTL_GPIO_MASK|PHI2_GPIO_MASK|RW_GPIO_MASK|S4_GPIO_MASK|S5_GPIO_MASK);

    gpio_init(ATARI_PHI2_PIN);
    gpio_set_dir(ATARI_PHI2_PIN, GPIO_IN);

    gpio_set_dir(RD4_PIN, GPIO_OUT);
    gpio_set_dir(RD5_PIN, GPIO_OUT);

	// overclocking isn't necessary for most functions - but XEGS carts weren't working without it
	// I guess we might as well have it on all the time.
	set_sys_clock_khz(250000, true);
}