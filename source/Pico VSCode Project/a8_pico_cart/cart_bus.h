/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CART_BUS_H
#define __CART_BUS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#define ALL_GPIO_MASK   	0x3FFFFFFF
#define ADDR_GPIO_MASK  	0x00001FFF
#define DATA_GPIO_MASK  	0x001FE000
#define CCTL_GPIO_MASK  	0x00200000  // gpio 21
#define PHI2_GPIO_MASK  	0x00400000  // gpio 22
#define RW_GPIO_MASK    	0x00800000  // gpio 23
#define S4_GPIO_MASK    	0x01000000  // gpio 24
#define S5_GPIO_MASK    	0x02000000  // gpio 25

#define S4_S5_GPIO_MASK 	0x03000000
#define CCTL_RW_GPIO_MASK 	0x00A00000

#define ATARI_PHI2_PIN        22    // used on boot to check if we are plugged into an atari or usb

#define RD4_PIN         26
#define RD5_PIN         27

#define RD4_LOW             gpio_put(RD4_PIN, 0)
#define RD4_HIGH            gpio_put(RD4_PIN, 1)
#define RD5_LOW             gpio_put(RD5_PIN, 0)
#define RD5_HIGH            gpio_put(RD5_PIN, 1)
#define SET_DATA_MODE_OUT   gpio_set_dir_out_masked(DATA_GPIO_MASK)
#define SET_DATA_MODE_IN    gpio_set_dir_in_masked(DATA_GPIO_MASK)

extern void config_gpio(void);

#endif // __CART_BUS_H
