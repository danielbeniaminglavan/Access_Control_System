#include "timer.h"
#include "comms.h"
#include "PIR.h"
#include "LCD.h"
#include "PN532.h"
#include "button.h"
#include "buzzer.h"
#include "LED.h"
#include "actuator.h"
#include "NVM.h"

void setup() 
{
  Serial.begin(115200);
  nvm_factory_reset_if_needed();
  comms_init();
  as_button_init();
  lcd_en_init();
  en_pir_init();
  as_servo_init();
  as_buzzer_init();
  red_led_init();
  green_led_init();
  init_nfc_en();
  init_timer();
}

void loop() 
{
  comms_update();
  refresh_lcd_en();
}
