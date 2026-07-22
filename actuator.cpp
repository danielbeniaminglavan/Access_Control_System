#include "actuator.h"
#include "buzzer.h"
#include <Arduino.h>
#include "NVM.h"
#include "comms.h"

// variabila globala de tip servo_t, care va putea fi folosita si in alte fisiere
servo_t servo;

// functie pentru initializarea servomotorului
void servo_init(servo_t* s, volatile uint8_t *ddr_reg, volatile uint16_t *ocr_reg, uint8_t pin, uint8_t update_interval)
{
  s->ddr_reg = ddr_reg;
  s->ocr_reg = ocr_reg;
  s->pin = pin;
  s->state = SERVO_IDLE;
  s->step_counter = 0;
  s->update_interval = update_interval;

  bool servo_handshake_success = false;

  if (system_fsm.master_defekt == false) 
  {
    unsigned long start_wait = millis();

    Serial.println(F("{\"ev\":\"request_servo_param\"}"));

    while (millis() - start_wait < TIME_FOR_WAIT_INIT_MESSAGE) 
    {
      if (Serial.available()) 
      {
        String response = Serial.readStringUntil('\n');
        response.trim();

        if (response.indexOf(F("\"ev\":\"servo_c\"")) != -1) 
        {
          String val;

          extract_json_value(response, "mov_t", val);
          s->conf_parameters.moving_time = (uint8_t)val.toInt();
          
          extract_json_value(response, "open_t", val);
          s->conf_parameters.open_time = (uint8_t)val.toInt();

          nvm_save_block_servo(&s->conf_parameters);
          
          servo_handshake_success = true;
          Serial.println(F("{\"ev\":\"servo_hs_ok\"}"));
          break;
        }
      }
    }
  }

  if (!servo_handshake_success) 
  {
    nvm_load_block_servo(&s->conf_parameters);
    //Serial.println(F("{\"ev\":\"servo_load_from_nvm\"}"));
  }

  s->change_moving_time = s->conf_parameters.moving_time;
  s->change_open_time = s->conf_parameters.open_time;

  if (s->update_interval > 0) 
  {
    s->moving_steps = ((uint32_t)s->conf_parameters.moving_time * 1000UL) / s->update_interval;
    s->open_steps   = ((uint32_t)s->conf_parameters.open_time   * 1000UL) / s->update_interval;
  }
  
  /*
  Serial.print(F("{\"mov_t\":"));
  Serial.print(s->conf_parameters.moving_time);
  Serial.print(F(",\"open_t\":"));
  Serial.print(s->conf_parameters.open_time);
  Serial.print(F(",\"mov_s\":"));
  Serial.print(s->moving_steps);
  Serial.println(F("}"));
  */

  setbit(*(s->ddr_reg), s->pin);
}

// functie care comanda servomotorul sa se deschida
void servo_command_open(servo_t *s)
{
  // comportament diferit in functie de starea curenta a servomotorului
  switch(s->state)
  {
    // Daca servomotorul este oprit, se trece in starea de deschidere
    case SERVO_IDLE:
      s->state = SERVO_OPENING;
      s->step_counter = 0;
      as_buzzer_command_moving(); // se emite semnal sonor pe perioada miscarii servomotorului
      break;
    
    // daca servomotorul deja se deschide,nu se face nimic
    case SERVO_OPENING:
      break;
    
    // daca servo este deja deschis, resetam contorul pentru a mentine pozitia
    case SERVO_OPEN:
      s->step_counter = 0;
      break;
    
    // Daca servomotorul se inchide, se inverseaza miscarea si se ajusteaza pasii
    case SERVO_CLOSING:
      s->state = SERVO_OPENING;
      s->step_counter = (s->moving_steps - s->step_counter);
      as_buzzer_command_moving();
      break;
  }
}

// functie care actualizeaza starea servomotorului la fiecare "SERVO_UPDATE_INTERVAL_MS" ms
void servo_update(servo_t* s)
{
  switch(s->state)
  {
    case SERVO_IDLE:
      break;

    // servomotorul se deplaseaza catre pozitia "deschis"
    case SERVO_OPENING:
      if(s->step_counter < s->moving_steps)
      {
        *s->ocr_reg = SERVO_CLOSE_OCR_VALUE - ((uint32_t)s->step_counter * SERVO_DELTA_OCR) / s->moving_steps; // calculul pulsului pentru o pozitie intermediara
        s->step_counter++;
      }
      else
      {
        // dupa ce a ajuns in pozitia "deschis", servomotorul trece in starea SERVO_OPEN
        s->state = SERVO_OPEN;
        s->step_counter = 0;
      }
      break;
    
    case SERVO_OPEN:
      // servomotorul ramane deschis pentru perioada definita
      if(s->step_counter < s->open_steps)
      {
        s->step_counter++;
      }
      else
      {
        // dupa perioada definita pentru pozitia "deschis",servomotorul incepe inchiderea 
        s->state = SERVO_CLOSING;
        as_buzzer_command_moving(); // se emite semnal sonor corespunzator miscarii
        
        s->step_counter = 0;
      }
      break;

    case SERVO_CLOSING:
      // servomotorul se deplaseaza catre pozitia "inchis"
      if(s->step_counter < s->moving_steps)
      {
        *s->ocr_reg = SERVO_OPEN_OCR_VALUE + ((uint32_t)s->step_counter * SERVO_DELTA_OCR) / s->moving_steps; // se calculeaza pulsul pentru pozitiile intermediare
        s->step_counter++;
      }
      else
      {
        // dupa ce servomotorul a ajuns in pozitia "inchis", se revine la starea idle
        s->state = SERVO_IDLE;
        s->step_counter = 0;
      }
      break;
  }
}
