#ifndef PIR_H
#define PIR_H

#include <Arduino.h>
#include "defs.h"

#define PIR_PIN 7               // pinul la care este conectat senzorul PIR
#define PIR_DDR_REGISTER DDRD   // registrul care controleaza directia pinilor aferenti portului la care este conectat senzorul
#define PIR_PIN_REGISTER PIND   // registrul pentru citirea starii logice a pinilor corspunzator portului la care este conectat senzorul

#define en_pir_init() pir_init(&pir, &PIR_DDR_REGISTER, &PIR_PIN_REGISTER, PIR_PIN)
#define en_pir_detect() pir_detect(&pir)

// structura aferenta unui senzor PIR
typedef struct {
    volatile uint8_t* ddr_reg;   // registrul DDR
    volatile uint8_t* pin_reg;   // registrul PIN
    uint8_t pin;                 // numarul pinului
} pir_sensor_t;

extern pir_sensor_t pir; // variabila de tip structura "PIRSensor" se foloseste si in alte fisiere

// Se declara functiile utile pentru lucrul cu un senzor PIR
void pir_init(pir_sensor_t* sensor, volatile uint8_t* ddr_reg, volatile uint8_t* pin_reg, uint8_t pin);
bool pir_detect(pir_sensor_t* sensor);

#endif // PIR_H
