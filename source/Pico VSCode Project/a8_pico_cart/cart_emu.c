#include "cart_bus.h"
#include "cart_emu.h"

uint8_t cart_ram[128*1024];
uint8_t cart_d5xx[PAGE_SIZE] = {0};

int cartType = CART_TYPE_NONE;

static void __not_in_flash_func(emulate_standard_8k)() {
	// 8k
	RD4_LOW;
	RD5_HIGH;

    uint32_t pins;
    uint16_t addr;
	while (1)
	{      
		// wait for s5 low
        while ((pins = gpio_get_all()) & S5_GPIO_MASK) ;
        SET_DATA_MODE_OUT;
		// while s5 low
		while(!((pins = gpio_get_all()) & S5_GPIO_MASK)) {
			addr = pins & ADDR_GPIO_MASK;
			gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[addr]) << 13);
		}
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_standard_16k)() {
	// 16k
	RD4_HIGH;
	RD5_HIGH;

    uint32_t pins;
    uint16_t addr;
	while (1)
	{
		// wait for either s4 or s5 low
		while (((pins = gpio_get_all()) & S4_S5_GPIO_MASK) == S4_S5_GPIO_MASK) ;
		SET_DATA_MODE_OUT;
		if (!(pins & S4_GPIO_MASK)) {
			// while s4 low
			while(!((pins = gpio_get_all()) & S4_GPIO_MASK)) {
				addr = pins & ADDR_GPIO_MASK;
				gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[addr]) << 13);
			}
		}
		else {
			// while s5 low
			while(!((pins = gpio_get_all()) & S5_GPIO_MASK)) {
				addr = pins & ADDR_GPIO_MASK;
				gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[0x2000|addr]) << 13);
			}
		}
		SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_XEGS_32k)(char switchable) {
	// 32k
	RD4_HIGH;
	RD5_HIGH;

    uint32_t pins, last;
    uint16_t addr;
    uint8_t data;
	unsigned char *bankPtr = &cart_ram[0];
	bool rd4_high = true, rd5_high = true;	// 400/800 MMU

	while (1)
	{
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S4_GPIO_MASK) && rd4_high)
		{	// s4 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr+addr))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
		else if (!(pins & S5_GPIO_MASK) && rd5_high)
		{	// s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[0x6000|addr]) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
		else if (!(pins & CCTL_RW_GPIO_MASK))
		{	// CCTL low + write
            last = pins;
            // read data bus on falling edge of phi2
            while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
            	last = pins;
			data = (last & DATA_GPIO_MASK) >> 13;
			// new bank is the low 2 bits written to $D5xx
			bankPtr = &cart_ram[0] + (8192*(data & 3));
			if (switchable) {
				if (data & 0x80) {
					RD4_LOW; RD5_LOW;
					rd4_high = rd5_high = false;
				} else {
					RD4_HIGH; RD5_HIGH;
					rd4_high = rd5_high = true;
				}
			}
		}
	}
}

static void __not_in_flash_func(emulate_XEGS_64k)(char switchable) {
	// 64k
	RD4_HIGH;
	RD5_HIGH;

    uint32_t pins, last;
    uint16_t addr;
    uint8_t data;
	unsigned char *bankPtr = &cart_ram[0];
	bool rd4_high = true, rd5_high = true;	// 400/800 MMU

	while (1)
	{
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S4_GPIO_MASK) && rd4_high)
		{	// s4 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr+addr))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
		else if (!(pins & S5_GPIO_MASK) && rd5_high)
		{	// s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[0xE000|addr]) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
		else if (!(pins & CCTL_RW_GPIO_MASK))
		{	// CCTL low + write
            last = pins;
            // read data bus on falling edge of phi2
            while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
            	last = pins;
			data = (last & DATA_GPIO_MASK) >> 13;
			// new bank is the low 3 bits written to $D5xx
			bankPtr = &cart_ram[0] + (8192*(data & 7));
			if (switchable) {
				if (data & 0x80) {
					RD4_LOW; RD5_LOW;
					rd4_high = rd5_high = false;
				} else {
					RD4_HIGH; RD5_HIGH;
					rd4_high = rd5_high = true;
				}
			}
		}
	}
}

static void __not_in_flash_func(emulate_XEGS_128k)(char switchable) {
	// 128k
	RD4_HIGH;
	RD5_HIGH;

    uint32_t pins, last;
    uint16_t addr;
    uint8_t data;
	unsigned char *bankPtr = &cart_ram[0];
	bool rd4_high = true, rd5_high = true;	// 400/800 MMU

	while (1)
	{
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S4_GPIO_MASK) && rd4_high)
		{	// s4 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr+addr))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
		else if (!(pins & S5_GPIO_MASK) && rd5_high)
		{	// s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[0x1E000|addr]) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
		else if (!(pins & CCTL_RW_GPIO_MASK))
		{	// CCTL low + write
            last = pins;
            // read data bus on falling edge of phi2
            while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
            	last = pins;
			data = (last & DATA_GPIO_MASK) >> 13;
			// new bank is the low 4 bits written to $D5xx
			bankPtr = &cart_ram[0] + (8192*(data & 0xF));
			if (switchable) {
				if (data & 0x80) {
					RD4_LOW; RD5_LOW;
					rd4_high = rd5_high = false;
				} else {
					RD4_HIGH; RD5_HIGH;
					rd4_high = rd5_high = true;
				}
			}
		}
	}
}

static void __not_in_flash_func(emulate_bounty_bob)() {
	// 40k
	RD4_HIGH;
	RD5_HIGH;

    uint32_t pins;
    uint16_t addr;
	unsigned char *bankPtr1 = &cart_ram[0], *bankPtr2 =  &cart_ram[0x4000];
	while (1)
	{
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;
		
        if (!(pins & S4_GPIO_MASK))
		{	// s4 low
			SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
			if (addr & 0x1000) {
	            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr2+(addr&0xFFF)))) << 13);
				if (addr == 0x1FF6) bankPtr2 = &cart_ram[0x4000];
				else if (addr == 0x1FF7) bankPtr2 = &cart_ram[0x5000];
				else if (addr == 0x1FF8) bankPtr2 = &cart_ram[0x6000];
				else if (addr == 0x1FF9) bankPtr2 = &cart_ram[0x7000];
			}
			else {
				gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr1+(addr&0xFFF)))) << 13);
				if (addr == 0x0FF6) bankPtr1 = &cart_ram[0];
				else if (addr == 0x0FF7) bankPtr1 = &cart_ram[0x1000];
				else if (addr == 0x0FF8) bankPtr1 = &cart_ram[0x2000];
				else if (addr == 0x0FF9) bankPtr1 = &cart_ram[0x3000];
			}
		}
		else if (!(pins & S5_GPIO_MASK))
		{	// s5 low
			SET_DATA_MODE_OUT;
			addr = pins & ADDR_GPIO_MASK;
			gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[0x8000|addr]) << 13);
		}
		// wait for phi2 low
		while (gpio_get_all() & PHI2_GPIO_MASK) ;
		SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_atarimax_128k)() {
	// 128k
	RD4_LOW;
	RD5_HIGH;

    uint32_t bank = 0;
    unsigned char *ramPtr;
    uint32_t pins;
    uint16_t addr;
	bool rd5_high = true;	// 400/800 MMU

	while (1)
	{
        // select the right SRAM base, based on the cartridge bank
		ramPtr = &cart_ram[0] + (8192 * (bank & 0xF));

        // wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(ramPtr + addr))) << 13);
        }
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & ADDR_GPIO_MASK;
            if ((addr & 0xE0) == 0) {
				bank = addr & 0xF;
				if (addr & 0x10)
					{ RD5_LOW; rd5_high = false; }
				else
					{ RD5_HIGH; rd5_high = true; }
            }
        }
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_williams)() {
	// williams 32k, 64k
	RD4_LOW;
	RD5_HIGH;

    uint32_t bank = 0;
    unsigned char *bankPtr;
    uint32_t pins;
    uint16_t addr;
	bool rd5_high = true;	// 400/800 MMU

	while (1)
	{
        // select the right SRAM base, based on the cartridge bank
		bankPtr = &cart_ram[0] + (8192*bank);
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr + addr))) << 13);
        }
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & ADDR_GPIO_MASK;
            if ((addr & 0xF0) == 0) {
				bank = addr & 0x07;
				if (addr & 0x08)
					{ RD5_LOW; rd5_high = false; }
				else
					{ RD5_HIGH; rd5_high = true; }
            }
        }
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_OSS_B)() {
	// OSS type B
	RD5_HIGH;
	RD4_LOW;
    uint32_t pins;
    uint16_t addr;
	uint32_t bank = 1;
	unsigned char *bankPtr;
	bool rd5_high = true;	// 400/800 MMU

	while (1)
	{
		// select the right SRAM block, based on the cartridge bank
		bankPtr = &cart_ram[0] + (4096*bank);
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
			if (addr & 0x1000)
	            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[addr&0xFFF]) << 13);
			else
	            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr+addr))) << 13);
		}
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & ADDR_GPIO_MASK;
			int a0 = addr & 1, a3 = addr & 8;
			if (a3 && !a0) { RD5_LOW; rd5_high = false; }
			else {
				RD5_HIGH;
				rd5_high = true;
				if (!a3 && !a0) bank = 1;
				else if (!a3 && a0) bank = 3;
				else if (a3 && a0) bank = 2;
			}
		}
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_OSS_A)(char is034M) {
	// OSS type A (034M, 043M)
	RD5_HIGH;
	RD4_LOW;
    uint32_t pins;
    uint16_t addr;
	uint32_t bank = 0;
	unsigned char *bankPtr;
	bool rd5_high = true;	// 400/800 MMU

	while (1)
	{
		// select the right SRAM block, based on the cartridge bank
		bankPtr = &cart_ram[0] + (4096*bank);
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
			if (addr & 0x1000)
	            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[addr|0x2000]) << 13);	// 4k bank #3 always mapped to $Bxxx
			else
	            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr+addr))) << 13);
		}
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & 0xF;
			if (addr & 0x8) { RD5_LOW; rd5_high = false; }
			else {
				RD5_HIGH; rd5_high = true;
				if (addr == 0x0) bank = 0;
				if (addr == 0x3 || addr == 0x7) bank = is034M ? 1 : 2;
				if (addr == 0x4) bank = is034M ? 2 : 1;
			}
		}
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_megacart)(int size) {
	// 16k - 128k
	RD4_HIGH;
	RD5_HIGH;

    uint32_t pins, last;
    uint16_t addr;
    uint8_t data;
	uint32_t bank_mask = 0x00;
	if (size == 32) bank_mask = 0x1;
	else if (size == 64) bank_mask = 0x3;
	else if (size == 128) bank_mask = 0x7;

	bool rd4_high = true, rd5_high = true;	// 400/800 MMU

	unsigned char *ramPtr = &cart_ram[0];
	while (1)
	{
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S4_GPIO_MASK) && rd4_high)
		{	// s4 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(ramPtr+addr))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
        else if (!(pins & S5_GPIO_MASK) && rd5_high)
		{	// s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(ramPtr+(addr|0x2000)))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
		}
		else if (!(pins & CCTL_RW_GPIO_MASK))
		{	// CCTL low + write
            last = pins;
            // read data bus on falling edge of phi2
            while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
            	last = pins;
			data = (last & DATA_GPIO_MASK) >> 13;
			// new bank is the low n bits written to $D5xx
			int bank = data & bank_mask;
			ramPtr = &cart_ram[0] + 16384 * (bank&0x7);
			if (data & 0x80) {
				RD4_LOW; RD5_LOW;
				rd4_high = rd5_high = false;
			} else {
				RD4_HIGH; RD5_HIGH;
				rd4_high = rd5_high = true;
			}
		}
	}
}

static void __not_in_flash_func(emulate_SIC)() {
	// 128k
	RD5_HIGH;
	RD4_LOW;

    uint32_t pins, last;
    uint16_t addr;
	uint8_t SIC_byte = 0;
	unsigned char *ramPtr = &cart_ram[0];
	bool rd4_high = false, rd5_high = true;	// 400/800 MMU

	while (1)
	{
        // wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S4_GPIO_MASK) && rd4_high)
        {   // s4 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(ramPtr + addr))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
        }
        else if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(ramPtr + (addr|0x2000)))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
        }
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & ADDR_GPIO_MASK;
			if ((addr & 0xE0) == 0)
			{
				if (pins & RW_GPIO_MASK)
				{   // read from $D5xx
		            SET_DATA_MODE_OUT;
					gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)SIC_byte) << 13);
					// wait for phi2 low
					while (gpio_get_all() & PHI2_GPIO_MASK) ;
					SET_DATA_MODE_IN;
				}
				else
				{	// write to $D5xx
					last = pins;
					// read data bus on falling edge of phi2
					while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
						last = pins;
					SIC_byte = (last & DATA_GPIO_MASK) >> 13;
					// switch bank
					ramPtr = &cart_ram[0] + 16384 * (SIC_byte&0x7);
					if (SIC_byte & 0x40) { RD5_LOW; rd5_high = false; } else { RD5_HIGH; rd5_high = true; }
					if (SIC_byte & 0x20) { RD4_HIGH; rd4_high = true; } else { RD4_LOW; rd4_high = false; }
				}
			}
		}
	}
}

static void __not_in_flash_func(emulate_SDX)(int size) {
	// 64k/128k
	RD5_HIGH;
	RD4_LOW;

	unsigned char *ramPtr = &cart_ram[0];
    uint32_t pins;
    uint16_t addr;
	bool rd5_high = true;	// 400/800 MMU

	while (1)
	{
        // wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(ramPtr + addr))) << 13);
        }
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & ADDR_GPIO_MASK;
			if ((addr & 0xF0) == 0xE0) {
				// 64k & 128k versions
				if (size == 64) ramPtr = &cart_ram[0]; else ramPtr = &cart_ram[65536];
				ramPtr += ((~addr) & 0x7) * 8192;
				if (addr & 0x8)
					{ RD5_LOW; rd5_high = false; }
				else
					{ RD5_HIGH; rd5_high = true; }
			}
			if (size == 128 && (addr & 0xF0) == 0xF0) {
				// 128k version only
				ramPtr = &cart_ram[0] + ((~addr) & 0x7) * 8192;
				if (addr & 0x8)
					{ RD5_LOW; rd5_high = false; }
				else
					{ RD5_HIGH; rd5_high = true; }
			}
		}
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_diamond_express)(uint8_t cctlAddr) {
	// 64k
	RD5_HIGH;
	RD4_LOW;

	unsigned char *ramPtr = &cart_ram[0];
    uint32_t pins;
    uint16_t addr;
	bool rd5_high = true;	// 400/800 MMU

	while (1)
	{
        // wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(ramPtr + addr))) << 13);
        }
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & ADDR_GPIO_MASK;
			if ((addr & 0xF0) == cctlAddr) {
				ramPtr = &cart_ram[0] + ((~addr) & 0x7) * 8192;
				if (addr & 0x8)
					{ RD5_LOW; rd5_high = false; }
				else
					{ RD5_HIGH;  rd5_high = true; }
			}
		}
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_blizzard)() {
	//16k
	RD4_HIGH;
	RD5_HIGH;
    uint32_t pins;
    uint16_t addr;
	bool rd4_high = true, rd5_high = true;	// 400/800 MMU

	while (1)
	{
        // wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S4_GPIO_MASK) && rd4_high)
        {   // s4 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[addr]) << 13);
		}
        else if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)cart_ram[0x2000|addr]) << 13);
        }
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
			RD4_LOW; RD5_LOW;
			rd4_high = rd5_high = false;
		}
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_turbosoft)(int size) {
	// 64k/128k
	RD4_LOW;
	RD5_HIGH;

    uint32_t bank = 0;
    unsigned char *bankPtr;
    uint32_t pins;
    uint16_t addr;
	bool rd5_high = true;	// 400/800 MMU

	uint32_t bank_mask = 0x00;
	if (size == 64) bank_mask = 0x7;
	else if (size == 128) bank_mask = 0xF;

	while (1)
	{
        // select the right SRAM base, based on the cartridge bank
		bankPtr = &cart_ram[0] + (8192*bank);
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr + addr))) << 13);
        }
        else if (!(pins & CCTL_GPIO_MASK))
        {   // CCTL low
            addr = pins & ADDR_GPIO_MASK;
			bank = addr & bank_mask;
			if (addr & 0x10)
				{ RD5_LOW; rd5_high = false; }
			else
				{ RD5_HIGH; rd5_high = true; }
        }
        // wait for phi2 low
        while (gpio_get_all() & PHI2_GPIO_MASK) ;
        SET_DATA_MODE_IN;
	}
}

static void __not_in_flash_func(emulate_atrax)() {
	// 128k
	RD4_LOW;
	RD5_HIGH;

    uint32_t bank = 0;
    unsigned char *bankPtr;
    uint32_t pins, last;
    uint16_t addr;
    uint8_t data;
	bool rd5_high = true;	// 400/800 MMU

	while (1)
	{
        // select the right SRAM base, based on the cartridge bank
		bankPtr = &cart_ram[0] + (8192*bank);
		// wait for phi2 high
		while (!((pins = gpio_get_all()) & PHI2_GPIO_MASK)) ;

        if (!(pins & S5_GPIO_MASK) && rd5_high)
        {   // s5 low
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(*(bankPtr + addr))) << 13);
			// wait for phi2 low
			while (gpio_get_all() & PHI2_GPIO_MASK) ;
			SET_DATA_MODE_IN;
        }
		else if (!(pins & CCTL_RW_GPIO_MASK))
		{	// CCTL low + write
            last = pins;
            // read data bus on falling edge of phi2
            while ((pins = gpio_get_all()) & PHI2_GPIO_MASK)
            	last = pins;
			data = (last & DATA_GPIO_MASK) >> 13;
			// new bank is the low 4 bits written to $D5xx
			bank = data & 0xF;
			if (data & 0x80)
				{ RD5_LOW; rd5_high = false; }
			else
				{ RD5_HIGH; rd5_high = true; }
        }
	}
}

void  __not_in_flash_func(emulate_cartridge)() {
	if (cartType == CART_TYPE_8K) emulate_standard_8k();
	else if (cartType == CART_TYPE_16K) emulate_standard_16k();
	else if (cartType == CART_TYPE_XEGS_32K) emulate_XEGS_32k(0);
	else if (cartType == CART_TYPE_XEGS_64K) emulate_XEGS_64k(0);
	else if (cartType == CART_TYPE_XEGS_128K) emulate_XEGS_128k(0);
	else if (cartType == CART_TYPE_SW_XEGS_32K) emulate_XEGS_32k(1);
	else if (cartType == CART_TYPE_SW_XEGS_64K) emulate_XEGS_64k(1);
	else if (cartType == CART_TYPE_SW_XEGS_128K) emulate_XEGS_128k(1);
	else if (cartType == CART_TYPE_BOUNTY_BOB) emulate_bounty_bob();
	else if (cartType == CART_TYPE_ATARIMAX_1MBIT) emulate_atarimax_128k();
	else if (cartType == CART_TYPE_WILLIAMS_64K) emulate_williams();
	else if (cartType == CART_TYPE_OSS_16K_TYPE_B) emulate_OSS_B();
	else if (cartType == CART_TYPE_OSS_8K) emulate_OSS_B();
	else if (cartType == CART_TYPE_OSS_16K_034M) emulate_OSS_A(1);
	else if (cartType == CART_TYPE_OSS_16K_043M) emulate_OSS_A(0);
	else if (cartType == CART_TYPE_MEGACART_16K) emulate_megacart(16);
	else if (cartType == CART_TYPE_MEGACART_32K) emulate_megacart(32);
	else if (cartType == CART_TYPE_MEGACART_64K) emulate_megacart(64);
	else if (cartType == CART_TYPE_MEGACART_128K) emulate_megacart(128);
	else if (cartType == CART_TYPE_SIC_128K) emulate_SIC();
	else if (cartType == CART_TYPE_SDX_64K) emulate_SDX(64);
	else if (cartType == CART_TYPE_SDX_128K) emulate_SDX(128);
	else if (cartType == CART_TYPE_DIAMOND_64K) emulate_diamond_express(0xD0);
	else if (cartType == CART_TYPE_EXPRESS_64K) emulate_diamond_express(0x70);
	else if (cartType == CART_TYPE_BLIZZARD_16K) emulate_blizzard();
	else if (cartType == CART_TYPE_4K) emulate_standard_8k();	// patch in load_file()
	else if (cartType == CART_TYPE_TURBOSOFT_64K) emulate_turbosoft(64);
	else if (cartType == CART_TYPE_TURBOSOFT_128K) emulate_turbosoft(128);
	else if (cartType == CART_TYPE_ATRAX_128K) emulate_atrax();
	else
	{	// no cartridge (cartType = 0)
		RD4_LOW;
		RD5_LOW;
		while (1) ;
	}
}
