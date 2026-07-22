#ifndef BUTTON_H
#define BUTTON_H

#include "defs.h"
#include "comms.h"
#include "LCD.h"

// configurare pentru butonul START/STOP
#define START_STOP_PIN           1
#define START_STOP_DDR_REGISTER  DDRC
#define START_STOP_PIN_REGISTER  PINC

#define as_button_init() button_init(&start_stop_btn, START_STOP_DDR_REGISTER, &START_STOP_PIN_REGISTER, START_STOP_PIN)
#define as_ss_btn_update() ss_btn_update(&start_stop_btn)

// structura aferenta unui buton
typedef struct {
    volatile uint8_t* ddr_reg;   // pointer catre registrul DDR corespunzator portului la care este conectat butonul
    volatile uint8_t* pin_reg;   // pointer catre registrul PIN corespunzator portului la care este conectat butonul
    uint8_t pin;                // numarul pinului in cadrul portului
    uint8_t state_now;           // starea actuala citita
    uint8_t state_prev;          // starea precedenta
} button_t;

extern button_t start_stop_btn; // variabila de tip structura "start_stop_btn" se foloseste si in alte fisiere

// functie pentru initializarea unui buton
void button_init(button_t* btn, volatile uint8_t* ddr_reg, volatile uint8_t* pin_reg, uint8_t pin);

// functie care actualizeaza starea butonului START/STOP
void ss_btn_update(button_t* btn);

#endif // BUTTON_H
