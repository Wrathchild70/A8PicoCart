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
 */

#ifndef __PICO_MENU_H
#define __PICO_MENU_H

#define CMD_SUCCESS		0x00
#define CMD_ERROR		0x01

#define CART_CMD_OPEN_ITEM			0x00
#define CART_CMD_READ_CUR_DIR		0x01
#define CART_CMD_GET_DIR_ENTRY		0x02
#define CART_CMD_UP_DIR				0x03
#define CART_CMD_ROOT_DIR			0x04
#define CART_CMD_SEARCH				0x05
#define CART_CMD_LOAD_SOFT_OS		0x10
#define CART_CMD_SOFT_OS_CHUNK		0x11
#define CART_CMD_MOUNT_ATR			0x20	// unused, done automatically by firmware
#define CART_CMD_READ_ATR_SECTOR	0x21
#define CART_CMD_WRITE_ATR_SECTOR	0x22
#define CART_CMD_ATR_HEADER			0x23
#define CART_CMD_RESET_FLASH		0xF0
#define CART_CMD_NO_CART			0xFE
#define CART_CMD_ACTIVATE_CART  	0xFF

extern void __not_in_flash_func(atari_cart_main)(void);
//extern char path[];

#endif // __PICO_MENU_H