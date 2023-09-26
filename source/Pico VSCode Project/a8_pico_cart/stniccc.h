/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STNICCC_H
#define __STNICCC_H

extern uint8_t *ScreenRAM;
extern uint8_t *VRAM;

extern void __not_in_flash_func(initStniccc)(void);
extern void __not_in_flash_func(refreshStniccc)(void);

#endif // __STNICCC_H
