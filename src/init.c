/* Copyright (C) 2026, HvA
** All rights reserved.
**
** Unauthorized copying of this file, via any medium is strictly prohibited
** Proprietary and confidential
**
** Written by M.B. van der Linden, October 2026
**
** Initialisatiefuncties voor de PRMIC-eindopdracht.
*/

#include <avr/io.h>

#include "init.h"

/*
 * Bron: Wim Dolman, De taal C en de Xmega,
 * hoofdstuk 20, code 20.1, p. 338.
 * Aangepast voor PB1 (ADCB, kanaal 0).
 */
void ADC_init(void)
{
    PORTB.DIRCLR = PIN1_bm;                             // Stel PB1 in als ingang
    ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc;           // Verbind PB1 met ADC-kanaal 0
    ADCB.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;    // Stel kanaal 0 in op single-ended
    ADCB.REFCTRL = ADC_REFSEL_INTVCC_gc;                // Gebruik VCC/1,6 als referentiespanning
    ADCB.CTRLB = ADC_RESOLUTION_12BIT_gc;               // Stel de ADC in op 12-bit unsigned
    ADCB.PRESCALER = ADC_PRESCALER_DIV256_gc;           // Stel de ADC-klok in op 125 kHz
    ADCB.CTRLA = ADC_ENABLE_bm;          
}


/*
 * Bron: PRMIC, week 4, les 2, dia 11.
 * Timer telt van 0 t/m PER, dus PER + 1 kloktikken.
 * Formule: PER = (F_CPU * tijd / prescaler) - 1
 * Berekening: (32000000 * 0.016 / 8) - 1 = 63999
 * Resultaat: iedere 16 ms een timer-overflow.
 */
void timer_init(void)
{
    TCE0.CTRLB = TC_WGMODE_NORMAL_gc;                   // Normale telmodus
    TCE0.PER = 63999;                                   // Overflow iedere 16 ms
    TCE0.INTCTRLA = TC_OVFINTLVL_LO_gc;                 // Low-level overflow-interrupt
    PMIC.CTRL |= PMIC_LOLVLEN_bm;                       // Schakel low-level interrupts in
    TCE0.CTRLA = TC_CLKSEL_DIV8_gc;                     // Gebruik een prescaler van 8
}


/*
 * Bron: Wim Dolman, De taal C en de Xmega,
 * hoofdstuk 22, code 22.3 en 22.4, p. 391-392.
 * Aangepast voor PWM op PD1 bij 1 kHz.
 * Formule: PER = (F_CPU / (prescaler * frequentie)) - 1
 * Berekening: (32000000 / (1 * 1000)) - 1 = 31999
 */
void pwm_init(void)
{
    PORTD.OUTCLR = PIN1_bm;                             // Zet D1 (PD1) laag
    PORTD.DIRSET = PIN1_bm;                             // Stel D1 in als uitgang
    TCD0.CTRLB = TC0_CCBEN_bm |TC_WGMODE_SINGLESLOPE_gc;// Activeer PWM op PD1
    TCD0.PER = 31999;                                   // PWM-frequentie van 1 kHz
    TCD0.CCB = 0;                                       // Begin met 0% dutycycle
    TCD0.CTRLA = TC_CLKSEL_DIV1_gc;                     // Gebruik de klok zonder prescaler
}


/*
 * Gebruikte bibliotheek: ucglib_xmega.h.
 * Initialisatie van het ST7735 grafisch LCD.
 */
void ucg_init(ucg_t *ucg)
{
    ucg_com_fnptr ucg_xmega_func = &ucg_com_xmega_cb;   // Selecteer de communicatiefunctie
    ucg_Init(ucg, ucg_dev_st7735_18x128x160,
             ucg_ext_st7735_18, ucg_xmega_func);        // Initialiseer het ST7735-display
}
