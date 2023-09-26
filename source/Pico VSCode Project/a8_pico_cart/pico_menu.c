/**
 *    _   ___ ___ _       ___          _   
 *   /_\ ( _ ) _ (_)__ _ / __|__ _ _ _| |_ 
 *  / _ \/ _ \  _/ / _/_\ (__/ _` | '_|  _|
 * /_/ \_\___/_| |_\__\_/\___\__,_|_|  \__|
 *                                         
 * 
 * Atari 8-bit cartridge for Raspberry Pi Pico
 *
 * Robin Edwards 2023
 *
 * Needs to be a release NOT debug build for the cartridge emulation to work
 * 
 * Changes from UnoCart:
 * - Attempts to get S4/S5, RD4/RD5 MMU behaviour correct on 400/800
 *   https://forums.atariage.com/topic/241888-ultimate-cart-sd-multicart-technical-thread/page/10/#comment-4266797
 * - Adds 4k carts (CAR type 58)
 * - Adds Turbsoft carts (CAR types 50,51)
 * - Adds ATRAX 128k cars (CAR type 17)
 */

#include "cart_bus.h"
#include "sdcard.h"
#include "pico_menu.h"
#include "atr_emu.h"
#include "cart_emu.h"
#include "xex_ldr.h"
#include "Mode7.h"
#include "fatfs_disk.h"

#include "osrom.h"
#include "rom.h"

/* ATR Handling */

// ATR format
#define ATR_HEADER_SIZE 16
#define ATR_SIGNATURE 0x0296
typedef struct {
  uint16_t signature;
  uint16_t pars;
  uint16_t secSize;
  uint16_t parsHigh;
  uint8_t flags;
  uint16_t protInfo;
  uint8_t unused[5];
} ATRHeader;

static char path[256];
static char curPath[256] = "";
static char searchStr[32];

static uint8_t __not_in_flash_func(pico_menu_cmd)(void)
{
	uint8_t cmd = cart_d5xx[0xDF];
	uint8_t cmd_arg = cart_d5xx[0x00];
	DIR_ENTRY *entry = (DIR_ENTRY *)&cart_ram[0];
	int ret, len;

	// OPEN ITEM
	if (cmd == CART_CMD_OPEN_ITEM) 
	{
		if (entry[cmd_arg].isDir)
		{	// directory
			strcat(curPath, "/");
			strcat(curPath, entry[cmd_arg].filename);
			cart_d5xx[0x01] = CMD_SUCCESS; // path changed
		}
		else
		{	// file/search result
			if (entry[cmd_arg].full_path[0])
				strcpy(path, entry[cmd_arg].full_path);	// search result
			else
				strcpy(path, curPath); // file in current directory
			strcat(path, "/");
			strcat(path, entry[cmd_arg].filename);
			if (strcasecmp(get_filename_ext(entry[cmd_arg].filename), "ATR")==0)
			{	// ATR
				cart_d5xx[0x01] = 3;	// ATR
				cartType = CART_TYPE_ATR;
			}
			else
			{	// ROM,CAR or XEX
				cartType = load_file(path);
				if (cartType)
					cart_d5xx[0x01] = (cartType != CART_TYPE_XEX ? 1 : 2);	// file was loaded
				else
				{
					cart_d5xx[0x01] = 4;	// error
					strcpy((char*)&cart_d5xx[0x02], errorBuf);
				}
			}
		}
	}
	// READ DIR
	else if (cmd == CART_CMD_READ_CUR_DIR)
	{
		ret = read_directory(curPath);
		if (ret) {
			cart_d5xx[0x01] = CMD_SUCCESS;	// ok
			cart_d5xx[0x02] = num_dir_entries;
		}
		else
		{
			cart_d5xx[0x01] = CMD_ERROR;	// error
			strcpy((char*)&cart_d5xx[0x02], errorBuf);
		}           
	}
	// GET DIR ENTRY n
	else if (cmd == CART_CMD_GET_DIR_ENTRY)
	{
		cart_d5xx[0x01] = entry[cmd_arg].isDir;
		strcpy((char*)&cart_d5xx[0x02], entry[cmd_arg].long_filename);
	}
	// UP A DIRECTORY LEVEL
	else if (cmd == CART_CMD_UP_DIR)
	{
		len = strlen(curPath);
		while (len && curPath[--len] != '/');
		curPath[len] = 0;
	}
	// ROOT DIR (when atari reset pressed)
	else if (cmd == CART_CMD_ROOT_DIR)
		curPath[0] = 0;
	// SEARCH str
	else if (cmd == CART_CMD_SEARCH)
	{
		strcpy(searchStr, (char*)&cart_d5xx[0x00]);
		int	ret = search_directory(curPath, searchStr);
		if (ret) {
			cart_d5xx[0x01] = CMD_SUCCESS;	// ok
			cart_d5xx[0x02] = num_dir_entries;
		}
		else
		{
			cart_d5xx[0x01] = CMD_ERROR;	// error
			strcpy((char*)&cart_d5xx[0x02], errorBuf);
		}
	}
	// LOAD PATCHED ATARI OS
	else if (cmd == CART_CMD_LOAD_SOFT_OS)
	{
		ret = load_file("UNO_OS.ROM");
		if (!ret) {
			for (int i=0; i<16384; i++)
				cart_ram[i] = os_rom[i];
		}
		cart_d5xx[0x01] = CMD_SUCCESS;	// ok
	}
	// COPY OS CHUNK
	else if (cmd == CART_CMD_SOFT_OS_CHUNK)
	{
		for (int i=0; i<128; i++)
			cart_d5xx[0x01+i] = cart_ram[cmd_arg*128+i];
	}
	// RESET FLASH FS (when boot with joystick 0 fire pressed)
	else if (cmd == CART_CMD_RESET_FLASH)
		create_fatfs_disk();
	// NO CART
	else if (cmd == CART_CMD_NO_CART)
		cartType = 0;
	// REBOOT TO CART
	else if (cmd == CART_CMD_ACTIVATE_CART)
	{
		return 1;
	}
	return 0;
}

/*
 Theory of Operation
 -------------------
 Atari sends command to mcu on cart by writing to $D5DF ($D5E0-$D5FF = SDX)
 (extra paramters for the command in $D500-$D5DE)
 Atari must be running from RAM when it sends a command, since the mcu on the cart will
 go away at that point.
 Atari polls $D500 until it reads $11. At this point it knows the mcu is back
 and it is safe to rts back to code in cartridge ROM again.
 Results of the command are in $D501-$D5DF
*/

static void __not_in_flash_func(emulate_boot_rom)(void) {
	RD5_HIGH;
	RD4_LOW;
    cart_d5xx[0x00] = 0x11;	// signal that we are here
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
                gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(cart_d5xx[addr&0xFF])) << 13);
                // wait for phi2 low
                while (gpio_get_all() & PHI2_GPIO_MASK) ;
                SET_DATA_MODE_IN;
            }
            else
            {   // atari is writing
                addr = pins & 0xFF;
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
        else if (!(pins & S5_GPIO_MASK))
        {   // normal cartridge read
            SET_DATA_MODE_OUT;
            addr = pins & ADDR_GPIO_MASK;
            gpio_put_masked(DATA_GPIO_MASK, ((uint32_t)(A8PicoCart_rom[addr])) << 13);
            // wait for phi2 low
            while (gpio_get_all() & PHI2_GPIO_MASK) ;
            SET_DATA_MODE_IN;
        }
    }
}

void __not_in_flash_func(atari_cart_main)(void) {
    while (1)
    {
      emulate_boot_rom();

      if (pico_menu_cmd()) {
        break;
      }
    }

    if (cartType == CART_TYPE_ATR) {
        emulate_disk_access(path);
    } else if (cartType == CART_TYPE_XEX) {
        feed_XEX_loader();
    } else if (cartType == CART_TYPE_M7) {
        emulate_Mode7_vram();
    } else {
        emulate_cartridge();
    }
}
