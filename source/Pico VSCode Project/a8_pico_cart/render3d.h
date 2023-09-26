/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RENDER3D_H
#define __RENDER3D_H

extern uint8_t *ScreenRAM;
extern uint8_t *VRAM;

extern void __not_in_flash_func(initRender3D)(void);
extern void __not_in_flash_func(refreshRender3D)(void);

#endif // __RENDER3D_H
