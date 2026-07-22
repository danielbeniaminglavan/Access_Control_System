#include "PIR.h"

// Se declara o variabila de tip pir_sensor_t, care va fi facuta disponibila si in celelalte fisiere
pir_sensor_t pir;

// Functia de initializare pentru un senzor PIR
void pir_init(pir_sensor_t* sensor, volatile uint8_t* ddr_reg, volatile uint8_t* pin_reg, uint8_t pin) 
{
    sensor->ddr_reg = ddr_reg;
    sensor->pin_reg = pin_reg;
    sensor->pin = pin;

    // Pinul la care este conectat senzorul este configurat ca intrare
    clrbit(*(sensor->ddr_reg), sensor->pin);
}

bool pir_detect(pir_sensor_t* sensor) {
    // Se citeste starea pinului din registrul PIN
    return (*(sensor->pin_reg) & (1 << sensor->pin)) != 0;
}
