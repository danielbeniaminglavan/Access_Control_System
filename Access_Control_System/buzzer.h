#ifndef BUZZER_H
#define BUZZER_H

#include "defs.h"
#include "actuator.h"

#define BUZZER_PIN                5   // buzzerul se va conecta pe pinul 5 al portului C
#define BUZZER_DDR_REGISTER DDRC   // registrul care controleaza directia pinilor aferenti portului la care este conectat buzzerul
#define BUZZER_PORT_REGISTER PORTC   // registrul pentru setarea starii logice a pinilor corspunzator portului la care este conectat buzzerul
#define BUZZER_UPDATE_INTERVAL_MS 10  // perioada la care se actualizeaza starea buzzerului este de 10 ms
#define BUZZER_ERROR_SIGNAL_MS     1000   // durata unui semnal de eroare emis de buzzer este de o secunda
#define BUZZER_BEEP_PERIOD_MS     200 // Thigh = Tlow = 200 ms atunci cand buzzerul emite sunet discontinuu pentru a semnala eroarea
#define BUZZER_WARNING_SIGNAL_MS     400 // durata unui semnal de avertizare emis de buzzer este de 400 ms
#define BUZZER_WARNING_BEEP_PERIOD_MS      100 // Thigh = Tlow = 100 ms atunci cand buzzerul emite sunet pentru atentionare
#define BUZZER_MIN_MOVING_CYCLES      1 // Numarul minim de cicluri on-off ale buzzerului in starea BUZZER_MOVING( util la revenirea din BUZZER_ERROR)      

#define BUZZER_ERROR_STEPS  (BUZZER_ERROR_SIGNAL_MS / BUZZER_UPDATE_INTERVAL_MS)    // numarul de actualizari necesare in starea BUZZER_ERROR
#define BUZZER_MOVEMENT_STEPS ((SERVO_MOVING_TIME_S * 1000UL) / BUZZER_UPDATE_INTERVAL_MS)    // numarul de actualizari necesare in starea BUZZER_MOVING
#define BUZZER_BEEP_STEPS   (BUZZER_BEEP_PERIOD_MS / BUZZER_UPDATE_INTERVAL_MS)               // numarul de actualizari necesare pentru Thigh,respectiv Tlow in starea BUZZER_MOVING
#define BUZZER_WARNING_STEPS (BUZZER_WARNING_SIGNAL_MS / BUZZER_UPDATE_INTERVAL_MS)           // numarul de actualizari necesare in starea BUZZER_WARNING
#define BUZZER_WARNING_BEEP_STEPS (BUZZER_WARNING_BEEP_PERIOD_MS / BUZZER_UPDATE_INTERVAL_MS) // numarul de actualizari necesare pentru Thigh,respectiv Tlow in starea BUZZER_WARNING
#define BUZZER_MIN_MOVING_STEPS  ((2 * BUZZER_MIN_MOVING_CYCLES * BUZZER_BEEP_PERIOD_MS) / BUZZER_UPDATE_INTERVAL_MS) // numarul de actualizari minime la revenirea in BUZZER_MOVING

#define as_buzzer_init() buzzer_init(&buzzer, &BUZZER_DDR_REGISTER, &BUZZER_PORT_REGISTER, BUZZER_PIN, BUZZER_UPDATE_INTERVAL_MS, &servo)
#define as_buzzer_command_error() buzzer_command_error(&buzzer)
#define as_buzzer_command_moving() buzzer_command_moving(&buzzer)
#define as_buzzer_command_warning() buzzer_command_warning(&buzzer)
#define as_buzzer_update() buzzer_update(&buzzer)

// Cele 3 stari in care se poate afla buzzerul
typedef enum
{
  BUZZER_IDLE,    // starea in care buzzerul nu emite sunet 
  BUZZER_ERROR,   // starea in care buzzerul emite sunet continuu timp de BUZZER_ERROR_SIGNAL_S secunde pentru a semnala o eroare
  BUZZER_MOVING,  // starea in care buzzerul emite sunet discontinuu timp de SERVO_MOVING_TIME_S pentru a indica miscarea servomotorului
  BUZZER_WARNING  // starea in care buzzerul emite sunet discontinuu timp de 600 ms pentru a atrage atentia utilizatorului
}buzzer_state_t;

// Structura aferenta parametrilor configurabili pentru un buzzer
typedef struct
{
  uint16_t error_time;           // durata emiterii semanalului de eroare (exprimata in ms)
  uint16_t movement_beep_period; // Thigh = Tlow = 200 ms atunci cand buzzerul emite sunet discontinuu pentru a semnala eroarea
  uint16_t warning_time;         // durata unui semnal de avertizare emis de buzzer este de 400 ms
  uint16_t warning_beep_period;  // Thigh = Tlow = 100 ms atunci cand buzzerul emite sunet pentru atentionare
}buzzer_configurable_param_t;

// Structura aferenta unui buzzer
typedef struct
{
  volatile uint8_t* ddr_reg;     // registrul DDR corespunzator portului la care este conectat buzzerul
  volatile uint8_t* port_reg;    // portul la care este conectat buzzerul
  uint8_t pin;                   // pinul la care este conectat buzzerul
  buzzer_state_t state;          // starea buzzerului
  uint16_t step_counter;         // contor pentru numarul de actualizari(necesar pentru a sti cat se sta in starile BUZZER_ERROR, BUZZER_MOVING si BUZZER_WARNING)
  uint16_t old_step_counter;     // contor pentru numarul de actualizari excecutate in BUZZER_MOVING la trecerea in BUZZER_ERROR sau BUZZER_WARNING
  uint16_t toggle_counter;       // contor pentru numarul de actualizari in starea BUZZER_MOVING, pentru a sti cand se face tpggle de la HIGH la LOW si invers
  uint8_t is_on;                 // flag care tine evidenta functionarii buzzerului in starea BUZZER_MOVING,pentru a putea face toggle corect
  uint8_t update_interval;       // perioada la care se actualizeaza starea buzzerului
  uint16_t error_steps;          // numarul de actualizari necesare in starea BUZZER_ERROR
  uint32_t movement_steps;       // numarul de actualizari necesare in starea BUZZER_MOVING
  uint16_t movement_beep_steps;  // numarul de actualizari necesare pentru Thigh,respectiv Tlow in starea BUZZER_MOVING
  uint16_t warning_steps;        // numarul de actualizari necesare in starea BUZZER_WARNING
  uint16_t warning_beep_steps;   // numarul de actualizari necesare pentru Thigh,respectiv Tlow in starea BUZZER_WARNING
  servo_t* servo;                // servomotorul pentru care se emite sunet de miscare
  buzzer_configurable_param_t conf_parameters; // structura aferenta parametrilor configurabili pentru un buzzer
  uint16_t change_error_time;           // durata emiterii semanalului de eroare (exprimata in ms)
  uint16_t change_movement_beep_period; // Thigh = Tlow = 200 ms atunci cand buzzerul emite sunet discontinuu pentru a semnala eroarea
  uint16_t change_warning_time;         // durata unui semnal de avertizare emis de buzzer este de 400 ms
  uint16_t change_warning_beep_period;  // Thigh = Tlow = 100 ms atunci cand buzzerul emite sunet pentru atentionare
  uint8_t min_moving_steps;
}buzzer_t;

extern buzzer_t buzzer;   // variabila de tip structura "buzzer" se foloseste si in alte fisiere
extern buzzer_configurable_param_t buzzer_conf_param;

// Se declara functiile utile pentru lucrul cu un buzzer
void buzzer_init(buzzer_t* b, volatile uint8_t* ddr_reg, volatile uint8_t* port_reg, uint8_t pin, uint8_t update_interval, servo_t* servo);  // functie pentru initializarea buzzerului
void buzzer_command_error(buzzer_t* b);    // functie care comanda buzzerul sa emita emnal de eroare
void buzzer_command_moving(buzzer_t* b);   // functie care comanda buzzerul sa emita semnal de miscare
void buzzer_command_warning(buzzer_t* b);  // functie care comanda buzzerul sa emita un semnal care sa atentioneze utilizatorul
void buzzer_update(buzzer_t* b);           // functie care actualizeaza starea unui buzzer
//void buzzer_calibrate_parameters(buzzer_t* b, uint16_t* error_time, uint16_t* warning_time, uint16_t* movement_beep_period, uint16_t* warning_beep_period); // functie care calibreaza perioadele in care un anumit buzzer emite sunet


#endif // BUZZER_H