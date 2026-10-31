
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

#define F_CPU 32000000UL                                // Systeemklok 32 MHz

#define MINRES 3569                                     // ADC-waarde bij 22,9 graden
#define MAXRES 2760                                     // ADC-waarde bij 40,0 graden
#define MINTEMP 22.9f                                   // Minimale kalibratietemperatuur
#define MAXTEMP 40.0f                                   // Maximale kalibratietemperatuur

#define FAN_ON_TEMP 30.0f                               // Ventilator AAN vanaf 30 graden
#define FAN_OFF_TEMP 25.0f                              // Ventilator UIT bij 25 graden
#define PWM_MAX 16000                                   // PWM-dutycycle 50%

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "clock.h"
#include "serialF0.h"
#include "ucglib_xmega.h"
#include "init.h"


/*
 * Bron: PRMIC, week 4, les 2, dia 11.
 * TCE0 overflow-interrupt.
 */

// Globale variabele voor de timerinterrupt
volatile uint8_t TimerFlag = 0;                         // flag variabele voor ISR

ISR(TCE0_OVF_vect)
{
    TimerFlag = 1;                                      // timer flag voor gebruik in main
}

/*
* Bron: PRMIC, huiswerk week 6, opdracht 1.
* Wim Dolman, De taal C en de Xmega,
* hoofdstuk 20, code 20.1, p. 338.
*/
uint16_t ADC_read(void)
{
    uint16_t res;                                       // Variabele voor het 12-bit ADC-resultaat
    ADCB.CH0.CTRL |= ADC_CH_START_bm;                   // Start de ADC-conversie op kanaal 0
    while (!(ADCB.CH0.INTFLAGS & ADC_CH_CHIF_bm));      // Wacht totdat de conversie voltooid is
    res = ADCB.CH0.RES;                                 // Lees het ADC-resultaat uit
    ADCB.CH0.INTFLAGS = ADC_CH_CHIF_bm;                 // Wis de interruptvlag van de ADC
    return res;                                         // Geef de gemeten ADC-waarde terug
}


int main(void)
{
    // Variabelen
    ucg_t ucg;                                          // Grafisch LCD
    char buffer[20];                                    // Buffer voor de LCD-tekst

    uint16_t ADCresultaat;                              // Gemeten ADC-waarde
    float TEMPresultaat;                                // Berekende temperatuur
    uint8_t fan_on = 0;                                 // Ventilator staat aanvankelijk UIT

    // Initialiseer functies (zie init.c)
    init_clock();
    init_stream(F_CPU);
    ADC_init();                                         // ADCB voor PB1
    timer_init();                                       // TCE0: overflow iedere 16 ms
    pwm_init();                                         // TCD0: PWM van 1 kHz
    ucg_init(&ucg);                                     // Initialiseer het ST7735-display

    // Grafische LCD setup
    ucg_SetFontMode(&ucg, UCG_FONT_MODE_TRANSPARENT);
    ucg_ClearScreen(&ucg);
    ucg_SetFont(&ucg, ucg_font_8x13_mr);
    ucg_SetColor(&ucg, 0, 255, 255, 0);
    ucg_SetRotate180(&ucg);

    // Status headers 
    ucg_DrawString(&ucg, 0, 50, 0, "ADC :");
    ucg_DrawString(&ucg, 0, 75, 0, "TEMP:");
    ucg_DrawString(&ucg, 0, 100, 0, "FAN :");

    // Toon de beginstatus van de ventilator
    ucg_SetColor(&ucg, 0, 255, 0, 0);
    ucg_DrawString(&ucg, 50, 100, 0, "OFF");

    sei();                                              // Schakel globale interrupts in

    while (1)
    {
        if (TimerFlag)                                  // Controleer of een nieuwe meting nodig is
        {
            TimerFlag = 0;                              // Wis de meetvlag

            // Meet de temperatuur
            ADCresultaat = ADC_read();
            TEMPresultaat = MINTEMP + (((MAXTEMP - MINTEMP) / (MAXRES - MINRES)) * ((float)ADCresultaat - MINRES));

            // Toon de ADC-waarde
            ucg_SetColor(&ucg, 0, 255, 255, 255);       // Tekstkleur wit
            sprintf(buffer, "%u", ADCresultaat);
            ucg_DrawString(&ucg, 50, 50, 0, buffer);

            // Toon de temperatuur
            sprintf(buffer, "%.1f C", TEMPresultaat);
            ucg_DrawString(&ucg, 50, 75, 0, buffer);

            // Ventilatorregeling met hysterese
            if (TEMPresultaat >= FAN_ON_TEMP && !fan_on)
            {
                fan_on = 1;                             // Ventilatorstatus AAN
                TCD0.CCBBUF = PWM_MAX;                  // Schakel PWM naar 50%

                ucg_SetColor(&ucg, 0, 0, 255, 0);       // Tekstkleur groen
                ucg_DrawString(&ucg, 50, 100, 0, "ON ");
            }
            else if (TEMPresultaat <= FAN_OFF_TEMP && fan_on)
            {
                fan_on = 0;                             // Ventilatorstatus UIT
                TCD0.CCBBUF = 0;                        // Schakel PWM uit

                ucg_SetColor(&ucg, 0, 255, 0, 0);       // Tekstkleur rood
                ucg_DrawString(&ucg, 50, 100, 0, "OFF");
            }
        }
    }
}
