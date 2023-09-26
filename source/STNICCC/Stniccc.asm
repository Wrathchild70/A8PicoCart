/* STNICCC Ported Demo ROM for use with UnoCart
 * by Mark Keates/Wrathchild@AtariAge
 * This file builds with WUDSN/MADS into an 8K Atari ROM
 * The original scene1.bin is streamed from the SD card
 */

CART_CMD_REFRESH_ST_SCREEN = $30
CART_CMD_INIT_ST = $31

;@com.wudsn.ide.asm.outputfileextension=.rom

CARTFG_DIAGNOSTIC_CART = $80       ;Flag value: Directly jump via CARTAD during RESET.
CARTFG_START_CART      = $04       ;Flag value: Jump via CARTAD and then via CARTCS.
CARTFG_BOOT            = $01       ;Flag value: Boot peripherals, then start the module.

SETVBV = $E45C
XITVBV = $E462
COLDSV = $E477				; Coldstart (powerup) entry point
WARMSV = $E474				; Warmstart entry point

COLPM2 = $D014
COLPF0 = $D016
COLPF1 = $D017
COLPF2 = $D018
COLBAK = $D01A
CONSOL = $D01F
PORTB = $D301
DMACTL = $D400
HSCROL = $D404
PMBASE = $D407
NMIEN = $D40E

SDMCTL = $22F
GPRIOR = $26F
COLOR0 = $2C4
COLOR1 = $2C5
COLOR2 = $2C6
COLOR3 = $2C7
COLOR4 = $2C8
CHBAS = $2F4

; ************************ VARIABLES ****************************

DOSINI	equ $0C
MEMLO	equ $02E7
RunVec	equ $02E0
IniVec	equ $02E2
VCOUNT	equ $D40B
WSYNC	equ $D40A
GINTLK	equ $03FA
TRIG3	equ $D013
SDLSTL	equ $230
SDLSTH	equ $231
CIOV	equ $E456

TimerLo	equ	$E0
TimerMid equ $E1
TimerHi	equ	$E2
VbiTmr	equ $E3
Jiffies	equ $E4
LockVbi	equ $E5

; ************************ CODE ****************************


	opt h-                     ;Disable Atari COM/XEX file headers

	org $a000,$800             ;RD5 cartridge base
	opt f+                     ;Activate fill mode
.proc BufToScreen1
	ldx #0
loop1:
	lda $a000,x
	sta $2000,x
	lda $a100,x
	sta $2100,x
	lda $a200,x
	sta $2200,x
	lda $a300,x
	sta $2300,x
	lda $a400,x
	sta $2400,x
	lda $a500,x
	sta $2500,x
	lda $a600,x
	sta $2600,x
	lda $a700,x
	sta $2700,x
	lda $a800,x
	sta $2800,x
	lda $a900,x
	sta $2900,x
	lda $aa00,x
	sta $2a00,x
	lda $ab00,x
	sta $2b00,x
	lda $ac00,x
	sta $2c00,x
	lda $ad00,x
	sta $2d00,x
	lda $ae00,x
	sta $2e00,x
	lda $af00,x
	sta $2f00,x
	lda $b000,x
	sta $3000,x
	lda $b100,x
	sta $3100,x
	lda $b200,x
	sta $3200,x
	lda $b300,x
	sta $3300,x
	lda $b400,x
	sta $3400,x
	lda $b500,x
	sta $3500,x
	lda $b600,x
	sta $3600,x
	lda $b700,x
	sta $3700,x
	lda $b800,x
	sta $3800,x
	lda $b900,x
	sta $3900,x
	lda $ba00,x
	sta $3a00,x
	lda $bb00,x
	sta $3b00,x
	lda $bc00,x
	sta $3c00,x
	lda $bd00,x
	sta $3d00,x
	lda $be00,x
	sta $3e00,x
	lda $bf00,x
	sta $3f00,x
	inx
	beq @+
	jmp .ADR loop1
@	rts
.endp

	.align $a200,$FF
	org $a200,$900             ;RD5 cartridge base
	opt f+                     ;Activate fill mode
.proc BufToScreen2
	ldx #0
loop2:
	lda $a000,x
	sta $4000,x
	lda $a100,x
	sta $4100,x
	lda $a200,x
	sta $4200,x
	lda $a300,x
	sta $4300,x
	lda $a400,x
	sta $4400,x
	lda $a500,x
	sta $4500,x
	lda $a600,x
	sta $4600,x
	lda $a700,x
	sta $4700,x
	lda $a800,x
	sta $4800,x
	lda $a900,x
	sta $4900,x
	lda $aa00,x
	sta $4a00,x
	lda $ab00,x
	sta $4b00,x
	lda $ac00,x
	sta $4c00,x
	lda $ad00,x
	sta $4d00,x
	lda $ae00,x
	sta $4e00,x
	lda $af00,x
	sta $4f00,x
	lda $b000,x
	sta $5000,x
	lda $b100,x
	sta $5100,x
	lda $b200,x
	sta $5200,x
	lda $b300,x
	sta $5300,x
	lda $b400,x
	sta $5400,x
	lda $b500,x
	sta $5500,x
	lda $b600,x
	sta $5600,x
	lda $b700,x
	sta $5700,x
	lda $b800,x
	sta $5800,x
	lda $b900,x
	sta $5900,x
	lda $ba00,x
	sta $5a00,x
	lda $bb00,x
	sta $5b00,x
	lda $bc00,x
	sta $5c00,x
	lda $bd00,x
	sta $5d00,x
	lda $be00,x
	sta $5e00,x
	lda $bf00,x
	sta $5f00,x
	inx
	beq @+
	jmp .ADR loop2
@	rts
.endp

	.align $b1c0,$FF
	org $b1c0
;Cartridge initalization
;Only the minimum of the OS initialization is complete, you don't want to code here normally.
init    .proc
	lda #$34
	sta COLOR4
	sta COLBAK
	rts
	.endp ; proc init
	
	.align $b200,$FF
	org $b200
;Cartridge start
;Copy main code to RAM and transfer control there
start   .proc
	lda #$84
	sta COLOR4
	sta COLBAK
	jsr copy_buf_to_screen
	jsr copy_wait_for_cart
	jsr copy_display_list
	jmp .ADR wait_for_cart
	.endp
	
.proc	copy_buf_to_screen
	ldy #0
@
	lda BufToScreen1,y
	sta .ADR BufToScreen1,y
	iny
	cpy #(.len[BufToScreen1] & 0xFF)
	bne @-
	ldy #0
@
	lda BufToScreen2,y
	sta .ADR BufToScreen2,y
	iny
	cpy #(.len[BufToScreen2] & 0xFF)
	bne @-
	rts
	.endp

.proc	copy_wait_for_cart
	ldy #0
@
	lda wait_for_cart,y
	sta .ADR wait_for_cart,y
	iny
	bne @-
@	lda wait_for_cart+$100,y
	sta .ADR wait_for_cart+$100,y
	iny
	cpy #(.len[wait_for_cart] & 0xFF)
	bne @-
	rts
	.endp

.proc	copy_display_list
	ldy #0
@
	lda G15_dlist_1,y
	sta .ADR G15_dlist_1,y
	iny
	cpy #.len[G15_dlist_1]
	bne @-
	ldy #0
@
	lda G15_dlist_2,y
	sta .ADR G15_dlist_2,y
	iny
	cpy #.len[G15_dlist_2]
	bne @-
	rts
	.endp

.macro wait_cart_ready
@	lda $D5DE
	cmp #'X'
	; wait for cart to become ready
	bne @-
	.endm

.macro show_msg msg
	ldy #<(.ADR :msg)
	ldx #>(.ADR :msg)
	jsr .ADR show_text
	.endm

.macro set_screen_1
	ldx #>(.ADR G15_dlist_1)
	lda #<(.ADR G15_dlist_1)
	stx SDLSTH
	sta SDLSTL
.endm

.macro set_screen_2
	ldx #>(.ADR G15_dlist_2)
	lda #<(.ADR G15_dlist_2)
	stx SDLSTH
	sta SDLSTL
.endm

	.align $ba00,$FF
	org $ba00, $a00

; cmd is in Accumulator
.proc wait_for_cart
	mva #$E4 COLOR4
	show_msg MsgWaitCart
	; dlist
	set_screen_1
	jsr .ADR wait_for_vbi

	wait_cart_ready
	
	mva #$FA COLOR0
	mva #$FE COLOR1
	mva #$F0 COLOR2
	mva #$00 COLOR3 ; unused
	sta COLOR4

	jsr .ADR wait_for_vbi
	lda SDMCTL
	pha
	lda #0
	sta NMIEN
	sta SDMCTL
	sta DMACTL

	; init
	mva #CART_CMD_INIT_ST $D5DF
	show_msg MsgInit
	wait_cart_ready
	
	lda #$F4
	jsr .ADR set_bg_colour
	
	lda $BFFD
	cmp #CARTFG_START_CART
	bne CartOK
	; show green if boot rom still present
	lda #$CA
	jsr .ADR set_bg_colour

CartOK
	show_msg MsgOk
	; each VBI will ask for a redraw
	LDA #7
	LDX #>(.ADR VBI)
	LDY #<(.ADR VBI)
	JSR SETVBV

	; clear up old dlist
	ldy #$7F
	lda #0
@	sta $9C00,y
	dey
	bpl @-
	
	sta TimerLo
	sta TimerMid
	sta TimerHi
	sta LockVbi

	; Add colons for timer
	LDA #$1A
	STA $6FA
	STA $6FD
	
	LDX #6
	LDA COLPM2 ; reads PAL/NTSC
	AND #2
	BNE @+
	; PAL fixups
	LDX #5
@:  STX Jiffies ; 1/10th of a second
	STX VbiTmr
	
	; Restore DMA (screen turns on at next VBlank)
	pla
	sta SDMCTL
	; enable vbi & dli
	mva #$C0 NMIEN
	
endless
	lda #0
	sta .ADR LockVbi
	jsr .ADR wait_for_vbi
	wait_cart_ready
	; See if 1st pass is completed
	ldx $D5DD
	cpx #'Z'
	bne @+
	lda TimerHi
	ora #$80
	sta TimerHi
	; Check currently viewed screen
@	ldx #>(.ADR G15_dlist_1)
	cpx SDLSTH
	beq swap_to_2
swap_to_1
	jsr .ADR BufToScreen1
	set_screen_1
	jmp .ADR endless
swap_to_2
	jsr .ADR BufToScreen2
	set_screen_2
	jmp .ADR endless
	
wait_for_vbi
	lda #1
wait_n_vbi
	clc
	adc $14
@
	cmp $14
	bne @-
	rts
	
set_bg_colour
	sta COLOR4
	sta COLBAK
	rts

show_text
	sty .ADR st + 1
	stx .ADR st + 2
	ldy #$27
	lda #0
@	sta $6D8,y
	dey
	bpl @-
	ldy #0
st:	lda $ffff,y
	bmi @+
	sta $6D8,y
	iny
	bpl st
@	rts

UpdateTimer
	BIT TimerHi
	BPL @+
	RTS
@	DEC VbiTmr
	BEQ @+
	RTS
@	LDA Jiffies
	STA VbiTmr
	SED
	LDA TimerLo
	CLC
	ADC #$10
	STA TimerLo
	LDA TimerMid
	ADC #$00
	STA TimerMid
	CMP #$60
	BCC @+
	LDA #$00
	STA TimerMid
	LDA TimerHi
	ADC #$00
	STA TimerHi
	CMP #$60
	BCC @+
	LDA #$00
	STA TimerHi
@	CLD
	LDY #0
	LDA TimerHi
	JSR .ADR OutBcd
	LDA TimerMid
	JSR .ADR OutBcd
	LDA TimerLo
OutBcd
	PHA
	LSR
	LSR
	LSR
	LSR
	ORA #$10
	STA $6F8,Y
	INY
	PLA
	AND #$0F
	ORA #$10
	STA $6F8,Y
	INY
	INY ; skip over ':'
	RTS

DLI
	pha
	sta WSYNC
	mva #$8E COLPF2
	mva #$8A COLPF1
	mva #$86 COLPF0
	pla
	rti

VBI
	ldx #>(.ADR DLI)
	lda #<(.ADR DLI)
	stx $201
	sta $200
	mva #0 COLBAK
	jsr .ADR UpdateTimer
	lda .ADR LockVbi
	beq @+
	JMP XITVBV
@	ldx $D5DE
	inx
	bne @+
	JMP XITVBV
@	inc .ADR LockVbi
	lda #CART_CMD_REFRESH_ST_SCREEN
	sta $D5DF
	sta $4D ; attract
	JMP XITVBV

MsgWaitCart
	dta d'Waiting for Cartridge',$80
MsgInit
	dta d'Initialising...',$80
MsgOk
	dta d'A8PicoCart : ST-NICCC 2000 Demo',$80

	.endp

	.align $bd00,$FF
	org $bd00,$600 ;Dlist

.proc G15_dlist_1
:2	.byte $70
	.byte $C2
	.word $6D8
	.byte $00
	.byte $4E
	.word $2010
:101	.byte $E
	.byte $4E
	.word $3000
:97	.byte $E
	.byte $41
	.word .ADR G15_dlist_1
.endp

	.align $be00,$FF
	org $be00,$700 ;Dlist

.proc G15_dlist_2
:2	.byte $70
	.byte $C2
	.word $6D8
	.byte $00
	.byte $4E
	.word $4010
:101	.byte $E
	.byte $4E
	.word $5000
:97	.byte $E
	.byte $41
	.word .ADR G15_dlist_2
.endp
; ************************ CARTRIDGE CONTROL BLOCK *****************

	.align $bff8,$FF
	org $bff8                 ;Cartridge control block
	.byte 'M', '7'            ;Signal to the UNO Cart for MODE-7 VRAM support
	.word start               ;CARTCS
	.byte 0                   ;CART
	.byte CARTFG_START_CART   ;CARTFG
	.word init                ;CARTAD
