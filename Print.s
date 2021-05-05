; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; SSD1306_OutChar   outputs a single 8-bit ASCII character
; SSD1306_OutString outputs a null-terminated string 

    IMPORT   SSD1306_OutChar
    IMPORT   SSD1306_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix
    PRESERVE8
    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB



;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutDec
	 ;Binding		
N EQU 4 ; the number to get unit place for
CNT EQU 0 ; count of number
FRM RN 11 ; stack frame pointer
	 
	 PUSH {R4-FRM}
	 PUSH {R0}
	 SUB SP, #4 ; go up stack by one element, allocation
	 MOV FRM, SP ; make FRM stack pointer
	 
	 PUSH {LR}
	 LDR R1, [FRM, #CNT] ; load count variable
	 MOV R2, #0 ; make it 0 and store
	 STR R2, [FRM,#CNT]
	 MOV R3, #10 ; for division later
	 
read LDR R2, [FRM,#CNT] ; load CNT
	 ADD R2, #1 ; increment CNT
	 STR R2, [FRM,#CNT] ; store back
	 
	 LDR R2, [FRM,#N]
	 LDR R12, [FRM,#N] ; copy
	 UDIV R2, R2, R3 ; unsigned div by 10, truncate smallest unit place
	 STR R2, [FRM,#N]
	 MUL R2, R3 ; multiply by 10, set smallest unit place to 0
	 SUB R1, R12, R2 ; subtract to isolate smallest unit place
	 PUSH {R1}
	 
	 LDR R1, [FRM,#N]
	 CMP R1, #0
	 BNE read
	 
write
	 POP {R0}
	 ADD R0, #0x30
	 BL SSD1306_OutChar
	 
	 LDR R1, [FRM, #CNT]
	 SUBS R1, #1
	 STR R1, [FRM, #CNT]
	 BNE write ; for every 1 CNT display one character
	 
	 POP {LR}
	 ADD SP, #8
	 POP {R4-FRM}


      BX  LR
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.01, range 0.00 to 9.99
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.00 "
;       R0=3,    then output "0.03 "
;       R0=89,   then output "0.89 "
;       R0=123,  then output "1.23 "
;       R0=999,  then output "9.99 "
;       R0>999,  then output "*.** "
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutFix
	 PUSH {R4-FRM}
	 PUSH {R0}
	 SUB SP, #4
	 MOV FRM, SP ;
	 
	 PUSH {LR}
	 LDR R1, [FRM, #CNT] ; clear count
	 MOV R2, #0
	 STR R2, [FRM,#CNT]
	 MOV R3, #10 ; for division later
	 
	 LDR R1, [FRM, #N]
	 CMP R1, #1000
	 BLO inrange ; if less than 1000, number can be displayed

;greater than 1000 output

	 MOV R0, #0x2A
	 BL SSD1306_OutChar ; output *
	 
	 MOV R0, #0x2E
	 BL SSD1306_OutChar ; .
	 
	 MOV R0, #0x2A
	 BL SSD1306_OutChar ; *
	 
	 MOV R0, #0x2A
	 BL SSD1306_OutChar ; *
	 B exit
	 
inrange 	 
	 LDR R2, [FRM,#CNT] ; load CNT
	 ADD R2, #1 ; increment CNT
	 STR R2, [FRM,#CNT] ; store back
	 
	 LDR R2, [FRM,#N]
	 LDR R12, [FRM,#N] ; copy
	 UDIV R2, R2, R3 ; unsigned div by 10 to truncate least significant digit
	 STR R2, [FRM,#N] ; store for later use
	 MUL R2, R3 ; multiply truncated number
	 SUB R1, R12, R2 ; subtract from original value to get number at specific unit
	 PUSH {R1}
	 
	 LDR R1, [FRM, #CNT]
	 CMP R1, #3 ; limit number to 3 for fixed output
	 BLO inrange
	 
	 POP {R0}
	 ADD R0, #0x30
	 BL SSD1306_OutChar

	 MOV R0, #0x2E
	 BL SSD1306_OutChar ; output .
	 
	 POP {R0}
	 ADD R0, #0x30
	 BL SSD1306_OutChar
	 
	 POP {R0}
	 ADD R0, #0x30
	 BL SSD1306_OutChar

exit
	 POP{LR}
	 ADD SP, #8
	 POP {R4-FRM}

     BX   LR
 
     ALIGN
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN          ; make sure the end of this section is aligned
     END            ; end of file
