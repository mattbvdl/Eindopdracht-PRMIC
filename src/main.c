
/* Copyright (C) 2026, HvA
** All rights reserved.
**
** Unauthorized copying of this file, via any medium is strictly prohibited
** Proprietary and confidential
**
** Written by M.B. van der Linden, October 2026
**
** ADC, Timers, PWM, Grafisch LCD
*/


#define F_CPU 32000000UL                                // Xmega clock speed

#define MINRES 3569                                     // ADC-waarde bij 22,9 graden
#define MAXRES 2760                                     // ADC-waarde bij 40 graden
#define MINTEMP 22.9f                                   // Gemeten temperatuur
#define MAXTEMP 40.0f                                   // Gemeten temperatuur

#define FAN_ON_TEMP 30.0f                               // Ventilator AAN vanaf 30 graden
#define FAN_OFF_TEMP 25.0f                              // Ventilator UIT bij 25 graden
#define PWM_MAX 16000                                   // PWM 50%

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "clock.h"
#include "serialF0.h"
#include "ucglib_xmega.h"

// Variabele voor de timerinterrupt
volatile uint8_t TimerFlag = 0;             // Teller voor het aantal timer-overflows

/*
 * Bron: Wim Dolman, De taal C en de Xmega,
 * Hoofdstuk 20, code 20.1, pagina 338.
 */
void init_adc(void)
{
    PORTB.DIRCLR = PIN1_bm;                             // Stel PB1 in als ingang
    ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc;           // Verbind PB1 met ADC-kanaal 0
    ADCB.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;    // Stel kanaal 0 in op single-ended
    ADCB.REFCTRL = ADC_REFSEL_INTVCC_gc;                // Gebruik VCC/1,6 als referentiespanning
    ADCB.CTRLB = ADC_RESOLUTION_12BIT_gc;               // Stel de ADC in op 12-bit unsigned
    ADCB.PRESCALER = ADC_PRESCALER_DIV256_gc;           // Stel de ADC-klok in op 32 MHz / 256 = 125 kHz
    ADCB.CTRLA = ADC_ENABLE_bm;                         // Schakel ADCB in
}


/*
 * Bron: Wim Dolman, De taal C en de Xmega,
 * Hoofdstuk 20, code 20.1, pagina 338.
 */
uint16_t read_adc(void)
{
    uint16_t res;                                       // Variabele voor het 12-bit ADC-resultaat

    ADCB.CH0.CTRL |= ADC_CH_START_bm;                   // Start de ADC-conversie op kanaal 0
    while (!(ADCB.CH0.INTFLAGS & ADC_CH_CHIF_bm));      // Wacht totdat de conversie voltooid is

    res = ADCB.CH0.RES;                                 // Lees het ADC-resultaat uit
    ADCB.CH0.INTFLAGS |= ADC_CH_CHIF_bm;                // Wis de interruptvlag van de ADC

    return res;                                         // Geef de gemeten ADC-waarde terug
}


// Initialiseer het grafische LCD
void ucg_init(ucg_t *ucg)
{
    ucg_com_fnptr ucg_xmega_func = &ucg_com_xmega_cb;   // Selecteer de communicatiefunctie voor de Xmega
    ucg_Init(ucg, ucg_dev_st7735_18x128x160,
             ucg_ext_st7735_18, ucg_xmega_func);        // Initialiseer het ST7735-display
}


// Initialiseer timer TCE0
void timer_init(void)
{
    TCE0.CTRLB = TC_WGMODE_NORMAL_gc;                   // Stel de timer in op normale telmodus
    TCE0.PER = 63999;                                   // Genereer iedere 16 ms een timer-overflow
    TCE0.INTCTRLA = TC_OVFINTLVL_LO_gc;                 // Schakel overflow-interrupt op laag niveau in
    TCE0.CTRLA = TC_CLKSEL_DIV8_gc;                     // Gebruik de systeemklok met prescaler 8
}

/*
 * Bron: Wim Dolman, De taal C en de Xmega,
 * Hoofdstuk 22, code 22.3 en 22.4, pagina 392-393.
 * Aangepast voor PWM op D1 (PD1) bij 32 MHz.
 */
void pwm_init(void)
{
    TCD0.CTRLB = TC0_CCBEN_bm | TC_WGMODE_SINGLESLOPE_gc; // Activeer PWM op PD1 in single-slope-modus
    TCD0.PER = 31999;                                    // Stel de PWM-frequentie in op 1 kHz
    TCD0.CCB = 0;                                    // Stel de PWM-dutycycle aanvankelijk in op 50%
    TCD0.CTRLA = TC_CLKSEL_DIV1_gc;                      // Gebruik de systeemklok van 32 MHz zonder prescaler
}


int main(void)
{
    // Initialiseer de microcontroller
    init_clock();
    init_stream(F_CPU);

    // Initialiseer de PWM-uitgang
    PORTD.OUTCLR = PIN1_bm;                             // Zet D1 (PD1) laag
    PORTD.DIRSET = PIN1_bm;                             // Stel D1 in als uitgang

    timer_init();                                       // Initialiseer TCE0 voor de overflow-interrupt
    pwm_init();                                         // Initialiseer TCD0 voor PWM

    PMIC.CTRL |= PMIC_LOLVLEN_bm;                       // Schakel low-level interrupts in
    sei();                                              // Schakel globale interrupts in


    // Initialiseer het grafische LCD
    ucg_t ucg;
    ucg_init(&ucg);                                     // Initialiseer het display
    ucg_SetFontMode(&ucg, UCG_FONT_MODE_TRANSPARENT);   // Stel een transparante tekstachtergrond in
    ucg_ClearScreen(&ucg);                              // Wis het scherm
    ucg_SetFont(&ucg, ucg_font_8x13_mr);                // Selecteer het lettertype
    ucg_SetColor(&ucg, 0, 255, 255, 0);                 // Stel kleurindex 0 in op geel
    ucg_SetRotate180(&ucg);                             // Draai de schermoriëntatie 180 graden

    ucg_DrawString(&ucg, 0, 50, 0, "ADC :");
    ucg_DrawString(&ucg, 0, 75, 0, "TEMP:");
    ucg_DrawString(&ucg, 0, 100, 0, "FAN :");

    // Initialiseer de ADC
    uint16_t res;                                       // Variabele voor de gemeten ADC-waarde
    init_adc();                                         // Initialiseer ADCB

    while (1)
    {
        if (TimerFlag)                                      // Controleer of een nieuwe meting nodig is
        {
            TimerFlag = 0;                                  // Wis de meetvlag

            res = read_adc();

            float OUT = MINTEMP + (((MAXTEMP - MINTEMP) / (MAXRES - MINRES)) * ((float)res - MINRES));

            char buffer[20];

            ucg_SetFontMode(&ucg, UCG_FONT_MODE_SOLID);
            ucg_SetColor(&ucg, 0, 255, 255, 255);       // Tekstkleur wit
            ucg_SetColor(&ucg, 1, 0, 0, 0);             // Achtergrondkleur zwart

            sprintf(buffer, "%u", res);            
            ucg_DrawString(&ucg, 50, 50, 0, buffer);     // Toon ADC-waarde

            int temp10 = (int)(OUT * 10.0f + 0.5f);
            sprintf(buffer, "%d,%01d C", temp10 / 10, temp10 % 10);
            ucg_DrawString(&ucg, 50, 75, 0, buffer);     // Toon temperatuur

            // Ventilatorregeling met hysterese
            if (OUT >= FAN_ON_TEMP)
            {
                TCD0.CCBBUF = PWM_MAX;                 // Ventilator AAN bij 30 graden
                ucg_SetColor(&ucg, 0, 0, 255, 0);    
                ucg_DrawString(&ucg, 50, 100, 0, "ON ");
            }
            else if (OUT <= FAN_OFF_TEMP)
            {
                TCD0.CCBBUF = 0;                       // Ventilator UIT bij 25 graden
                ucg_SetColor(&ucg, 0, 255, 0, 0);      
                ucg_DrawString(&ucg, 50, 100, 0, "OFF");
            }
        }
    }
}


// Interrupt Service Routine voor timer TCE0
ISR(TCE0_OVF_vect)
{
    TimerFlag = 1;                                      // Geef aan dat een ADC-meting nodig is
}
