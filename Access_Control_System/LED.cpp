#include "LED.h"

led_t red_led;
led_t green_led;

void led_init(led_t* led, volatile uint8_t* ddr_reg, volatile uint8_t* port_reg, uint8_t pin)
{
  led->ddr_reg = ddr_reg;
  led->port_reg = port_reg;
  led->pin = pin;

  setbit(*(led->ddr_reg), led->pin);
  clrbit(*(led->port_reg), led->pin);
}

void turn_on_led(led_t* led)
{
  setbit(*(led->port_reg), led->pin);
}

void turn_off_led(led_t* led)
{
  clrbit(*(led->port_reg), led->pin);
}

void signal_succes()
{
  turn_off_red_led();
  turn_on_green_led();
}

void signal_error()
{
  turn_off_green_led();
  turn_on_red_led();
}

void signal_nothing()
{
  turn_off_green_led();
  turn_off_red_led();
}