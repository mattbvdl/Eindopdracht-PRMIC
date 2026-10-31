/* Initialisatiefuncties voor de PRMIC-eindopdracht. */
#ifndef INIT_H
#define INIT_H

#include "ucglib_xmega.h"

void ADC_init(void);
void timer_init(void);
void pwm_init(void);
void ucg_init(ucg_t *ucg);

#endif /* INIT_H */
