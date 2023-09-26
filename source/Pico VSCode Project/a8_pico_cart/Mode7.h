/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODE7_H
#define __MODE7_H

#define CART_TYPE_M7				252

#define CART_CMD_REFRESH_M7_SCREEN		0x30
#define CART_CMD_LOAD_M7_VRAM			0x31
#define CART_CMD_PREPARE_M7_SCREEN		0x32
#define CART_CMD_PROCESS_M7_MOVEMENT	0x33

extern void __not_in_flash_func(emulate_Mode7_vram)(void);

extern uint8_t *VRAM;
extern uint8_t *ScreenRAM;

#endif // __MODE7_H
