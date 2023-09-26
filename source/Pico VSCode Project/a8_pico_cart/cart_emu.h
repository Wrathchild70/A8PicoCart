/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CART_EMU_H
#define __CART_EMU_H

#include <stdio.h>

#define BANK_SIZE 0x2000
#define PAGE_SIZE 0x100

extern uint8_t cart_ram[];
extern uint8_t cart_d5xx[PAGE_SIZE];

extern int cartType;

#define CART_TYPE_NONE				0
#define CART_TYPE_8K				1	// 8k
#define CART_TYPE_16K				2	// 16k
#define CART_TYPE_XEGS_32K			3	// 32k
#define CART_TYPE_XEGS_64K			4	// 64k
#define CART_TYPE_XEGS_128K			5	// 128k
#define CART_TYPE_SW_XEGS_32K		6	// 32k
#define CART_TYPE_SW_XEGS_64K		7	// 64k
#define CART_TYPE_SW_XEGS_128K		8	// 128k
#define CART_TYPE_MEGACART_16K		9	// 16k
#define CART_TYPE_MEGACART_32K		10	// 32k
#define CART_TYPE_MEGACART_64K		11	// 64k
#define CART_TYPE_MEGACART_128K		12	// 128k
#define CART_TYPE_BOUNTY_BOB		13	// 40k
#define CART_TYPE_ATARIMAX_1MBIT	14	// 128k
#define CART_TYPE_WILLIAMS_64K		15	// 32k/64k
#define CART_TYPE_OSS_16K_TYPE_B	16	// 16k
#define CART_TYPE_OSS_8K			17	// 8k
#define CART_TYPE_OSS_16K_034M		18	// 16k
#define CART_TYPE_OSS_16K_043M		19	// 16k
#define CART_TYPE_SIC_128K			20	// 128k
#define CART_TYPE_SDX_64K			21	// 64k
#define CART_TYPE_SDX_128K			22	// 128k
#define CART_TYPE_DIAMOND_64K		23	// 64k
#define CART_TYPE_EXPRESS_64K		24	// 64k
#define CART_TYPE_BLIZZARD_16K		25	// 16k
#define CART_TYPE_4K				26	// 4k
#define CART_TYPE_TURBOSOFT_64K		27	// 64k
#define CART_TYPE_TURBOSOFT_128K	28	// 128k
#define CART_TYPE_ATRAX_128K		29	// 128k

extern void  __not_in_flash_func(emulate_cartridge)(void);

#endif // __CART_EMU_H
