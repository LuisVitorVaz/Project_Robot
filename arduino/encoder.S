;----------------------------------------------------
; 4 Rotary Encoders — ATmega328P
;
; Encoder 1: CLK=PC0 (A0), DT=PC1 (A1)
; Encoder 2: CLK=PC2 (A2), DT=PC3 (A3)
; Encoder 3: CLK=PC4 (A4), DT=PC5 (A5)
; Encoder 4: CLK=PD2 (D2), DT=PD3 (D3)
;
; Logica de contagem identica ao codigo original:
;   - INC contador, se overflow (0xFF->0x00) = INC voltas
;   - DEC contador, se underflow (0x00->0xFF) = DEC voltas
;
; Estados e contadores salvos em SRAM para persistir
; entre chamadas do loop() — corrige perda de estado
;
; Retorno R24 = encoder que moveu (1-4), 0 = nenhum
; Variaveis exportadas para C:
;   enc_dir   — 1=direita, 0=esquerda
;   enc_passo — passo atual do encoder que moveu (0-255)
;   enc_volta — voltas do encoder que moveu
;----------------------------------------------------

#define __SFR_OFFSET 0x00
#include "avr/io.h"

.global rotary_encoder

;----------------------------------------------------
; Variaveis em SRAM
;----------------------------------------------------
.section .bss

; Exportadas para leitura no C
.global enc_dir
.global enc_passo
.global enc_volta

enc_dir:    .skip 1
enc_passo:  .skip 1
enc_volta:  .skip 1

; Estado anterior dos CLKs (persistem entre chamadas)
prev1:      .skip 1    ; CLK enc1 anterior
prev2:      .skip 1    ; CLK enc2 anterior
prev3:      .skip 1    ; CLK enc3 anterior
prev4:      .skip 1    ; CLK enc4 anterior

; Contadores individuais por encoder
cnt1:       .skip 1    ; passo enc1 (0-255)
vol1:       .skip 1    ; voltas enc1
cnt2:       .skip 1
vol2:       .skip 1
cnt3:       .skip 1
vol3:       .skip 1
cnt4:       .skip 1
vol4:       .skip 1

initialized: .skip 1

.global init_flag
init_flag:  .skip 1

;----------------------------------------------------
.section .text

rotary_encoder:

    CLR R24    ; retorno padrao = nenhum encoder

    ;========================
    ; Inicializa apenas na primeira chamada
    ;========================
    LDS R16, initialized
    CPI R16, 0xAA
    BREQ main_loop

    ; PC0..PC5 = entradas com pull-up (encoders 1,2,3)
    LDI R16, 0x00
    OUT DDRC, R16
    LDI R16, 0xFF
    OUT PORTC, R16

    ; PD2, PD3 = entradas com pull-up (encoder 4)
    ; NAO toca PD0/PD1 = UART
    CBI DDRD, 2
    CBI DDRD, 3
    SBI PORTD, 2
    SBI PORTD, 3

    ; Zera contadores
    CLR R16
    STS cnt1, R16
    STS vol1, R16
    STS cnt2, R16
    STS vol2, R16
    STS cnt3, R16
    STS vol3, R16
    STS cnt4, R16
    STS vol4, R16

    ; Salva estado inicial dos CLKs
    IN R16, PINC
    ANDI R16, 0x01
    STS prev1, R16

    IN R16, PINC
    LSR R16
    LSR R16
    ANDI R16, 0x01
    STS prev2, R16

    IN R16, PINC
    LSR R16
    LSR R16
    LSR R16
    LSR R16
    ANDI R16, 0x01
    STS prev3, R16

    IN R16, PIND
    LSR R16
    LSR R16
    ANDI R16, 0x01
    STS prev4, R16

    LDI R16, 0xAA
    STS initialized, R16

main_loop:
    RCALL encoder1
    CPI R24, 0
    BRNE enc_done

    RCALL encoder2
    CPI R24, 0
    BRNE enc_done

    RCALL encoder3
    CPI R24, 0
    BRNE enc_done

    RCALL encoder4

enc_done:
    RET


;============================================================
; ENCODER 1 — CLK=PC0, DT=PC1
;============================================================
encoder1:

    IN R22, PINC
    ANDI R22, 0x01

    LDS R21, prev1
    CP R22, R21
    BREQ enc1_end

    IN R23, PINC
    ANDI R23, 0x02
    LSR R23

    CP R22, R23
    BREQ enc1_left

    LDS R20, cnt1
    INC R20
    STS cnt1, R20
    LDI R24, 1
    LDI R16, 1
    STS enc_dir, R16
    BRNE enc1_print
    LDS R26, vol1
    INC R26
    STS vol1, R26
    STS enc_volta, R26
    STS enc_passo, R20
    RJMP enc1_store

enc1_print:
    STS enc_passo, R20
    LDS R26, vol1
    STS enc_volta, R26
    RJMP enc1_store

enc1_left:
    LDS R20, cnt1
    DEC R20
    STS cnt1, R20
    LDI R24, 1
    CLR R16
    STS enc_dir, R16
    CPI R20, 0xFF
    BRNE enc1_print2
    LDS R26, vol1
    DEC R26
    STS vol1, R26
    STS enc_volta, R26
    STS enc_passo, R20
    RJMP enc1_store

enc1_print2:
    STS enc_passo, R20
    LDS R26, vol1
    STS enc_volta, R26

enc1_store:
    STS prev1, R22

enc1_end:
    RET


;============================================================
; ENCODER 2 — CLK=PC2, DT=PC3
;============================================================
encoder2:

    IN R22, PINC
    LSR R22
    LSR R22
    ANDI R22, 0x01

    LDS R21, prev2
    CP R22, R21
    BREQ enc2_end

    IN R23, PINC
    LSR R23
    LSR R23
    LSR R23
    ANDI R23, 0x01

    CP R22, R23
    BREQ enc2_left

    LDS R27, cnt2
    INC R27
    STS cnt2, R27
    LDI R24, 2
    LDI R16, 1
    STS enc_dir, R16
    BRNE enc2_print
    LDS R28, vol2
    INC R28
    STS vol2, R28
    STS enc_volta, R28
    STS enc_passo, R27
    RJMP enc2_store

enc2_print:
    STS enc_passo, R27
    LDS R28, vol2
    STS enc_volta, R28
    RJMP enc2_store

enc2_left:
    LDS R27, cnt2
    DEC R27
    STS cnt2, R27
    LDI R24, 2
    CLR R16
    STS enc_dir, R16
    CPI R27, 0xFF
    BRNE enc2_print2
    LDS R28, vol2
    DEC R28
    STS vol2, R28
    STS enc_volta, R28
    STS enc_passo, R27
    RJMP enc2_store

enc2_print2:
    STS enc_passo, R27
    LDS R28, vol2
    STS enc_volta, R28

enc2_store:
    STS prev2, R22

enc2_end:
    RET


;============================================================
; ENCODER 3 — CLK=PC4, DT=PC5
; FIX: branches longos substituidos por BRXX/.+4/RJMP
;      para corrigir "relocation truncated R_AVR_7_PCREL"
;============================================================
encoder3:

    IN R22, PINC
    LSR R22
    LSR R22
    LSR R22
    LSR R22
    ANDI R22, 0x01

    LDS R21, prev3
    CP R22, R21
    ; BREQ enc3_end  <- era o branch longo problemático
    BRNE enc3_active       ; se diferente, continua
    RJMP enc3_end          ; se igual, sai

enc3_active:
    IN R23, PINC
    LSR R23
    LSR R23
    LSR R23
    LSR R23
    LSR R23
    ANDI R23, 0x01

    CP R22, R23
    ; BREQ enc3_left  <- possível branch longo
    BRNE enc3_right        ; se diferente = direita
    RJMP enc3_left         ; se igual = esquerda

enc3_right:
    LDS R29, cnt3
    INC R29
    STS cnt3, R29
    LDI R24, 3
    LDI R16, 1
    STS enc_dir, R16
    ; BRNE enc3_print  <- possível branch longo
    BREQ enc3_volta3       ; se zerou = volta
    RJMP enc3_print        ; senao = passo normal

enc3_volta3:
    LDS R30, vol3
    INC R30
    STS vol3, R30
    STS enc_volta, R30
    STS enc_passo, R29
    RJMP enc3_store

enc3_print:
    STS enc_passo, R29
    LDS R30, vol3
    STS enc_volta, R30
    RJMP enc3_store

enc3_left:
    LDS R29, cnt3
    DEC R29
    STS cnt3, R29
    LDI R24, 3
    CLR R16
    STS enc_dir, R16
    CPI R29, 0xFF
    ; BRNE enc3_print2  <- possível branch longo
    BREQ enc3_undervolt    ; se underflow = volta negativa
    RJMP enc3_print2       ; senao = passo normal

enc3_undervolt:
    LDS R30, vol3
    DEC R30
    STS vol3, R30
    STS enc_volta, R30
    STS enc_passo, R29
    RJMP enc3_store

enc3_print2:
    STS enc_passo, R29
    LDS R30, vol3
    STS enc_volta, R30

enc3_store:
    STS prev3, R22

enc3_end:
    RET


;============================================================
; ENCODER 4 — CLK=PD2, DT=PD3
;============================================================
encoder4:

    IN R22, PIND
    LSR R22
    LSR R22
    ANDI R22, 0x01

    LDS R21, prev4
    CP R22, R21
    BREQ enc4_end

    IN R23, PIND
    LSR R23
    LSR R23
    LSR R23
    ANDI R23, 0x01

    CP R22, R23
    BREQ enc4_left

    LDS R16, cnt4
    INC R16
    STS cnt4, R16
    LDI R24, 4
    LDI R17, 1
    STS enc_dir, R17
    BRNE enc4_print
    LDS R17, vol4
    INC R17
    STS vol4, R17
    STS enc_volta, R17
    STS enc_passo, R16
    RJMP enc4_store

enc4_print:
    STS enc_passo, R16
    LDS R17, vol4
    STS enc_volta, R17
    RJMP enc4_store

enc4_left:
    LDS R16, cnt4
    DEC R16
    STS cnt4, R16
    LDI R24, 4
    CLR R17
    STS enc_dir, R17
    CPI R16, 0xFF
    BRNE enc4_print2
    LDS R17, vol4
    DEC R17
    STS vol4, R17
    STS enc_volta, R17
    STS enc_passo, R16
    RJMP enc4_store

enc4_print2:
    STS enc_passo, R16
    LDS R17, vol4
    STS enc_volta, R17

enc4_store:
    STS prev4, R22

enc4_end:
    RET