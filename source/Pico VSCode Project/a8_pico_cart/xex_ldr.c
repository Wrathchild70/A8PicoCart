/*
 *    _   ___ ___ _       ___          _   
 *   /_\ ( _ ) _ (_)__ _ / __|__ _ _ _| |_ 
 *  / _ \/ _ \  _/ / _/_\ (__/ _` | '_|  _|
 * /_/ \_\___/_| |_\__\_/\___\__,_|_|  \__|
 *                                         
 * 
 * Atari 8-bit cartridge for Raspberry Pi Pico
 *
 * Robin Edwards 2023
 * Source split: Mark Keates, September 2023
 *
 * Changes for Source Split:
 * - extracted (flash) xex-loader related functions
 */

#include <stdio.h>
#include <string.h>

#include "cart_bus.h"
#include "cart_emu.h" // for cart_ram1&2
#include "xex_ldr.h"

static uint32_t bank = 0;
static unsigned char *ramPtr = &cart_ram[0];

void __not_in_flash_func(feed_XEX_loader)(void) {
	RD4_LOW;
	RD5_LOW;

    uint32_t pins, last;
    uint16_t addr;
    uint8_t data;
	
	while (1)
	{
        // wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            if (pins & RW_GPIO_MASK)
            {   // atari is reading
                SET_DATA_MODE_OUT;
                addr = pins & ADDR_GPIO_MASK;
                gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(ramPtr[addr&0xFF])) << 13);
            }
			else
            {   // atari is writing
                addr = pins & 0xFF;
                last = pins;
                // read data bus on falling edge of phi2
                while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
                    last = pins;
				data = (last & DATA_GPIO_MASK) >> 13;
				if (addr == 0)
					bank = (bank&0xFF00) | data;
				else if (addr == 1)
					bank = (bank&0x00FF) | ((data<<8) & 0xFF00);
				ramPtr = &cart_ram[0] + 256 * (bank & 0x01FF);
			}
		}
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}
