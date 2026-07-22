#ifndef LED_H
#define LED_H

#include "defs.h"

#define RED_LED_PIN               3   // ledul rosu se va conecta pe pinul 3 al portului C
#define RED_LED_DDR_REGISTER DDRC     // registrul care controleaza directia pinilor aferenti portului la care este conectat ledul rosu
#define RED_LED_PORT_REGISTER PORTC   // registrul pentru setarea starii logice a pinilor corspunzator portului la care este conectat ledul rosu

#define GREEN_LED_PIN             4   // ledul verde se va conecta pe pinul 4 al portului C
#define GREEN_LED_DDR_REGISTER DDRC   // registrul care controleaza directia pinilor aferenti portului la care este conectat ledul verde
#define GREEN_LED_PORT_REGISTER PORTC // registrul pentru setarea starii logice a pinilor corspunzator portului la care este conectat ledul verde

#define red_led_init() led_init(&red_led, &RED_LED_DDR_REGISTER, &RED_LED_PORT_REGISTER, RED_LED_PIN)
#define green_led_init() led_init(&green_led, &GREEN_LED_DDR_REGISTER, &GREEN_LED_PORT_REGISTER, GREEN_LED_PIN)
#define turn_on_red_led() turn_on_led(&red_led)
#define turn_on_green_led() turn_on_led(&green_led)
#define turn_off_red_led() turn_off_led(&red_led)
#define turn_off_green_led() turn_off_led(&green_led)

typedef struct
{
  volatile uint8_t* ddr_reg;    // registrul DDR corespunzator portului la care este conectat ledul
  volatile uint8_t* port_reg;   // portul la care este conectat ledul
  uint8_t pin;                  // pinul la care este conectat ledul
}led_t;

extern led_t red_led; 
extern led_t green_led; 

// Se declara functiile utile pentru lucrul cu un buzzer
void led_init(led_t* led, volatile uint8_t* ddr_reg, volatile uint8_t* port_reg, uint8_t pin);
void turn_on_led(led_t* led);
void turn_off_led(led_t* led);

void signal_succes();
void signal_error();
void signal_nothing();

#endif // LED_H