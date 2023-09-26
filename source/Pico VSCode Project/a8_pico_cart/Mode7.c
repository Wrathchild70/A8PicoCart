#include "cart_bus.h"
#include "cart_emu.h"
//#include "vram_m7.h"
//#include "glenz.h"
//#include "mandelbrot.h"
//#include "render3d.h"
#include "stniccc.h"
#include "Mode7.h"

uint8_t *VRAM = &cart_ram[0x2000]; // 0x8000 -> 0x9FFF (used up to 0x9800+0x400 (32*32) = $9C00)
uint8_t *ScreenRAM = &cart_ram[0]; // 0xA000 -> 0xBDFF (40*192 = 0x1E00)

static void __not_in_flash_func(emulate_mode7)(void) {
    uint32_t pins, last;
    uint16_t addr;
    uint8_t data;
//	uint16_t *copyLen = (uint16_t *)(&cart_d5xx[1]);
//	uint16_t *copySrc = (uint16_t *)(&cart_d5xx[4]);
//	uint16_t *copyDst = (uint16_t *)(&cart_d5xx[7]);
	while (1) {
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        addr = pins & ADDR_GPIO_MASK;
        if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
			addr &= 0xFF;
            if (pins & RW_GPIO_MASK)
            {   // atari is reading
                SET_DATA_MODE_OUT;
                gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(cart_d5xx[addr])) << 13);
                // wait for phi2 low
                while (gpio_get_all() & PHI2_GPIO_MASK) ;
                SET_DATA_MODE_IN;
//				switch (addr)
//				{
//				case 6:
//					*copySrc = *copySrc + 1;
//					break;
//				case 9:
//					*copyDst = *copyDst + 1;
//					break;
//				case 10:
//					*copyLen = *copyLen - 1;
//					if (*copyLen == 0)
//					{
//						cart_d5xx[9] = 0x38; // SEC
//					}
//					break;
//				}
            }
            else
            {   // atari is writing
				last = pins;
                // read data bus on falling edge of phi2
                while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
                    last = pins;
                data = (last & DATA_GPIO_MASK) >> 13;
                cart_d5xx[addr] = data;
                if (addr == 0xDF)	// write to $D5DF
                    break;
            }
        }
		else if (!(pins & S5_GPIO_MASK)) {
//			if (pins & RW_GPIO_MASK) {
				SET_DATA_MODE_OUT;
                gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(ScreenRAM[addr])) << 13);
				// wait for phi2 low
                while (gpio_get_all() & PHI2_GPIO_MASK) ;
                SET_DATA_MODE_IN;
/*			} else {
				data = DATA_IN;
				// read data bus on falling edge of phi2
				while (CONTROL_IN & PHI2)
					data = DATA_IN;
				ScreenRAM[addr] = data >> 8;
			}
*/		}
		else if (!(pins & S4_GPIO_MASK)) {
			if (pins & RW_GPIO_MASK) {
				SET_DATA_MODE_OUT;
                gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(VRAM[addr])) << 13);
				// wait for phi2 low
                while (gpio_get_all() & PHI2_GPIO_MASK) ;
                SET_DATA_MODE_IN;
			} else {
				last = pins;
				// read data bus on falling edge of phi2
                while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
                    last = pins;
                data = (last & DATA_GPIO_MASK) >> 13;
				VRAM[addr] = data;
			}
		}
	}
}

#define CmdCommence() cart_d5xx[0xDE] = 0xFF
#define CmdComplete() cart_d5xx[0xDE] = 'X'

static void __not_in_flash_func(pico_Mode7_cmd)(void) {
	uint8_t cmd = cart_d5xx[0xDF];
	switch (cmd) {
	// rebuild the screen from VRAM
	case CART_CMD_PREPARE_M7_SCREEN:
		//update_glenz();
		//prepare_m7_vram();
		//refreshMandelbrot();
		//refreshRender3D();
		refreshStniccc();
		break;
	case CART_CMD_REFRESH_M7_SCREEN:
		//update_glenz();
		//refresh_m7_vram();
		//refreshMandelbrot();
		//refreshRender3D();
		refreshStniccc();
		break;
	case CART_CMD_LOAD_M7_VRAM:
		//init_glenz();
		//init_m7_vram();
		//initMandelbrot();
		//initRender3D();
		initStniccc();
		ScreenRAM[0x1FFD] = 0xFF;
		break;
//	case CART_CMD_PROCESS_M7_MOVEMENT:
//		refresh_m7_process(cart_d5xx[0xDC], cart_d5xx[0xDD]);
//		break;
	}
}

void __not_in_flash_func(emulate_Mode7_vram)(void) {
	RD5_HIGH;
	RD4_HIGH;
	CmdComplete();
	while (1) {
		emulate_mode7();
		CmdCommence();
		pico_Mode7_cmd();
		CmdComplete();
	}
}
