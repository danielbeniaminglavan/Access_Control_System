#ifndef ACTUATOR_H
#define ACTUATOR_H

#include "defs.h"

#define SERVO_PIN                2  // pinul la care este conectat servomotorul
#define SERVO_DDR_REGISTER       DDRB // registrul care controleaza directia pinilor aferenti portului la care este conectat servomotorul
#define SERVO_OCR_REGISTER       OCR1B // registrul OCR care controleaza semnalul PWM pentru servomotor
#define SERVO_UPDATE_INTERVAL_MS 10 // perioada la care se actualizeaza starea servomotorului este de 10 ms
#define SERVO_MOVING_TIME_S      3  // durata miscarii servomotorului pentru deschidere/inchidere (exprimata in sec)
#define SERVO_OPEN_TIME_S        3  // perioada in care servomotorul trebuie sa ramana in pozitia "deschis" (exprimata in sec)
// timpul total pentru o secventa completa: deschidere, deschis, inchidere (exprimata in ms)
//#define SERVO_TOTAL_TIME         ((2 * SERVO_MOVING_TIME_S + SERVO_OPEN_TIME_S) * 1000)

// numarul de actualizari pentru starea SERVO_OPENING/SERVO_CLOSING
#define SERVO_MOVING_STEPS  ((SERVO_MOVING_TIME_S * 1000UL) / SERVO_UPDATE_INTERVAL_MS)
// numarul de actualizari pentru starea SERVO_OPEN
#define SERVO_OPEN_STEPS     ((SERVO_OPEN_TIME_S    * 1000UL) / SERVO_UPDATE_INTERVAL_MS)

// valori pulse pentru inchidere si deschidere (exprimate in microsecunde)
#define SERVO_CLOSE_PULSE  2500UL
#define SERVO_OPEN_PULSE   1500UL

// Valori pentru registrul OCR (Output Compare Register) corespunzatoare pulsurilor de mai sus
#define SERVO_CLOSE_OCR_VALUE  (2 * SERVO_CLOSE_PULSE)
#define SERVO_OPEN_OCR_VALUE   (2 * SERVO_OPEN_PULSE)

// diferenta dintre pulsul aplicat la inchidere si cel aplicat la deschidere
#define SERVO_DELTA_OCR  (SERVO_CLOSE_OCR_VALUE - SERVO_OPEN_OCR_VALUE)

#define as_servo_init() servo_init(&servo, &SERVO_DDR_REGISTER, &SERVO_OCR_REGISTER, SERVO_PIN, SERVO_UPDATE_INTERVAL_MS)
#define as_servo_command_open() servo_command_open(&servo)
#define as_servo_update() servo_update(&servo)

// cele 4 stari posibile in care se poate afla servomotorul
typedef enum 
{
  SERVO_IDLE,  
  SERVO_OPENING,
  SERVO_OPEN,
  SERVO_CLOSING
} servo_state_t;

typedef struct
{
  uint8_t moving_time;        // durata miscarii servomotorului pentru deschidere/inchidere (exprimata in sec)
  uint8_t open_time;          // perioada in care servomotorul trebuie sa ramana in pozitia "deschis" (exprimata in sec)
}servo_configurable_param_t;

// structura aferenta servomotorului
typedef struct
{
  volatile uint8_t *ddr_reg;  // registrul DDR corespunzator portului la care este conectat servomotorul
  volatile uint16_t *ocr_reg; // pointer catre registrul OCR folosit pentru generarea PWM-ului
  uint8_t pin;                // pinul la care este conectat servomotorul
  servo_state_t state;        // starea curenta a servomotorului
  uint16_t step_counter;      // contor pentru numarul de actualizari (folosit pentru a masura timpul petrecut in celelalte stari,cu exceptia SERVO_IDLE)
  uint8_t update_interval;    // perioada la care se actualizeaza starea servomotorului
  uint32_t moving_steps;      // numarul de actualizari pentru starea SERVO_OPENING/SERVO_CLOSING
  uint32_t open_steps;        // numarul de actualizari pentru starea SERVO_OPEN
  servo_configurable_param_t conf_parameters; // structura aferenta parametrilor configurabili pentru un servomotor
  uint8_t change_moving_time;        // durata miscarii servomotorului pentru deschidere/inchidere (exprimata in sec)
  uint8_t change_open_time;          // perioada in care servomotorul trebuie sa ramana in pozitia "deschis" (exprimata in sec)
} servo_t;

// variabila globala de tip Servo, utilizata si in alte fisiere
extern servo_t servo;

void servo_init(servo_t* s, volatile uint8_t *ddr_reg, volatile uint16_t *ocr_reg, uint8_t pin, uint8_t update_interval); // functie pentru initializarea servomotorului
void servo_command_open(servo_t* s);      // functie care comanda servomotorul sa se deschida
void servo_update(servo_t* s);            // functie care actualizeaza starea servomotorului la fiecare interval definit
//void servo_calibrate_parameters(servo_t* s, uint8_t* moving_time, uint8_t* open_time); // functie care calibreaza perioadele in care un anumit servomotor ramane deschis, respectiv se afla in miscare

#endif // ACTUATOR_H
