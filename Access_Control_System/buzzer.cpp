#include <Arduino.h>
#include "buzzer.h"
#include "NVM.h"
#include "comms.h"

// Se declara o variabila de tip buzzer_t, care va fi facuta disponibila si in celelalte fisiere
buzzer_configurable_param_t buzzer_conf_param;
buzzer_t buzzer;

// Functia responsabila de initializarea unui buzzer(initializare countere,stare default,initializare flag, perioada update stare, servomotorul asociat pt sunetul care indica miscare)
void buzzer_init(buzzer_t* b, volatile uint8_t* ddr_reg, volatile uint8_t* port_reg, uint8_t pin, uint8_t update_interval, servo_t* servo)
{
  b->ddr_reg = ddr_reg;
  b->port_reg = port_reg;
  b->pin = pin;
  b->state = BUZZER_IDLE;
  b->step_counter = 0;
  b->old_step_counter =0;
  b->toggle_counter = 0;
  b->is_on = FALSE;
  b->servo = servo;
  b->update_interval = update_interval;

  bool buzzer_handshake_success = false;

  if (system_fsm.master_defekt == false) 
  {
    unsigned long start_wait = millis();

    Serial.println(F("{\"ev\":\"request_buzzer_param\"}"));

    while (millis() - start_wait < TIME_FOR_WAIT_INIT_MESSAGE) 
    {
      if (Serial.available()) 
      {
        String response = Serial.readStringUntil('\n');
        response.trim();

        if (response.indexOf(F("\"ev\":\"bc\"")) != -1) 
        {
          String val;
          
          extract_json_value(response, "err_t", val);
          b->conf_parameters.error_time = (uint16_t)val.toInt();
          
          extract_json_value(response, "wt", val);
          b->conf_parameters.warning_time = (uint16_t)val.toInt();
          
          extract_json_value(response, "mp", val);
          b->conf_parameters.movement_beep_period = (uint16_t)val.toInt();
          
          extract_json_value(response, "wp", val);
          b->conf_parameters.warning_beep_period = (uint16_t)val.toInt();

          nvm_save_block_buzzer(&b->conf_parameters);
          
          buzzer_handshake_success = true;
          Serial.println(F("{\"ev\":\"buzzer_hs_ok\"}"));
          break;
        }
      }
    }
  }

  if (!buzzer_handshake_success) 
  {
    nvm_load_block_buzzer(&b->conf_parameters);
    //Serial.println(F("{\"ev\":\"buzzer_load_from_nvm\"}"));
  }

  b->change_error_time = b->conf_parameters.error_time;
  b->change_warning_time = b->conf_parameters.warning_time;
  b->change_warning_beep_period = b->conf_parameters.warning_beep_period;
  b->change_movement_beep_period = b->conf_parameters.movement_beep_period;
  b->min_moving_steps = (uint8_t)(((uint32_t)2 * BUZZER_MIN_MOVING_CYCLES * b->conf_parameters.movement_beep_period) / b->update_interval);

  if (b->update_interval > 0) 
  {
    b->error_steps = b->conf_parameters.error_time / b->update_interval;

    b->warning_steps = b->conf_parameters.warning_time / b->update_interval;
    b->warning_beep_steps = b->conf_parameters.warning_beep_period / b->update_interval;

    b->movement_beep_steps = b->conf_parameters.movement_beep_period / b->update_interval;
    b->movement_steps = ((uint32_t)b->servo->conf_parameters.moving_time * 1000UL) / b->update_interval;
  }
  
  /*
  Serial.print(F("{\"err_t\":"));
  Serial.print(b->conf_parameters.error_time);
  Serial.print(F(",\"warn_t\":"));
  Serial.print(b->conf_parameters.warning_time);
  Serial.print(F(",\"mov_p\":"));
  Serial.print(b->conf_parameters.movement_beep_period);
  Serial.print(F(",\"warn_p\":"));
  Serial.print(b->conf_parameters.warning_beep_period);
  Serial.println(F("}"));
  */

  setbit(*(b->ddr_reg),b->pin);
  clrbit(*(b->port_reg),b->pin);
}

// Functia care comanda buzzerul sa emita sunet de eroare(se actualizeaza starea,counterele,flagul si se emite semnal HIGH pe pinul corespunzator)
void buzzer_command_error(buzzer_t* b)
{
  if(b->state == BUZZER_MOVING)
  {
    b->old_step_counter = b->step_counter;
  }

  b->state = BUZZER_ERROR;
  b->step_counter = 0;
  b->toggle_counter = 0;
  b->is_on = TRUE;
  setbit(*b->port_reg, b->pin);
}

// Functia care comanda buzzerul sa emita sunet de miscare(se actualizeaza starea,counterele,flagul si se emite semnal HIGH pe pinul corespunzator)
void buzzer_command_moving(buzzer_t* b)
{
  // daca ne aflam deja in starea BUZZER_MOVING,inseamna ca servomotorul a primit o alta comanda de miscare in timp ce se afla deja in miscare
  // si durata pentru executarea "noii miscari" nu va mai fi cea default
  if(b->state == BUZZER_MOVING)
  {
    b->step_counter = b->movement_steps - b->step_counter;
  }
  else
  {
    b->step_counter = 0;
    b->state = BUZZER_MOVING;
    b->toggle_counter = 0;
    b->is_on = TRUE;
    setbit(*b->port_reg, b->pin);
  }
}

// Functia care comanda buzzerul sa emita sunet pentru avertizarea utilizatorului (se actualizeaza starea,counterele,flagul si se emite semnal HIGH pe pinul corespunzator)
void buzzer_command_warning(buzzer_t* b)
{
  // Comanda va fi executata doar daca starea buzzerului este diferita de BUZZER_ERROR sau BUZZER_MOVING

  if(b->state != BUZZER_ERROR && b->state != BUZZER_MOVING)
  {
    /*if(b->state == BUZZER_MOVING)
    {
      b->old_step_counter = b->step_counter;
    }
    */
    b->state = BUZZER_WARNING;
    b->step_counter = 0;
    b->toggle_counter = 0;
    b->is_on = TRUE;
    setbit(*b->port_reg, b->pin);
  }
}

// Functia care actualizeaza starea buzzerului la un interval fix de timp(in acest caz 10 ms)
void buzzer_update(buzzer_t *b)
{
  // actualizarea se face in functie de starea buzzerului la momentul respectiv 
  switch(b->state)
  {
    // nu se face nimic daca starea este BUZZER_IDLE
    case BUZZER_IDLE:
         break;

    // daca starea este BUZZER_WARNING,se actualizeaza counterul pana se ajunge la b->warning_steps, apoi se trece in starea BUZZER_IDLE
    // in tot acest timp se face toggle pe pinul corespunzator buzzerului pentru a se emite sunetul discontinuu
    case BUZZER_WARNING:
         if(b->toggle_counter >= b->warning_beep_steps)
         {
           b->toggle_counter = 0;
           b->is_on = !b->is_on;
           if(b->is_on)
           {
             setbit(*b->port_reg, b->pin);
           }
           else
           {
            clrbit(*b->port_reg, b->pin);
           }
         }
         else
         {
           b->toggle_counter++;
         }

         if(b->step_counter < b->warning_steps)
         {
           b->step_counter++;
         }
         else
         {
          b->is_on = FALSE;
          clrbit(*b->port_reg, b->pin);
          b->state = BUZZER_IDLE;
          b->step_counter = 0;
          b->old_step_counter = 0;
          b->toggle_counter = 0;
         }
         break;
    
    // daca starea este BUZZER_ERROR,se actualizeaza counterul pana se ajunge la b->error_steps,apoi se trece in starea BUZZER_IDLE
    case BUZZER_ERROR:
         if(b->step_counter < b->error_steps)
         {
          b->step_counter++;
         }
         else
         {
          if(b->old_step_counter!= 0 && (b->movement_steps - (b->error_steps + b->old_step_counter)) >= b->min_moving_steps)
          {
            b->state = BUZZER_MOVING;
            b->step_counter = b->error_steps + b->old_step_counter;
          }
          else
          {
            b->state = BUZZER_IDLE;
            b->step_counter = 0;
            b->old_step_counter = 0;
          }
          b->is_on = FALSE;
          clrbit(*b->port_reg, b->pin);
          b->toggle_counter = 0;
         }
         break;
    // daca starea este BUZZER_MOVING,se actualizeaza counterul pana se ajunge la b->movement_steps, apoi se trece in starea BUZZER_IDLE
    // in tot acest timp se face toggle pe pinul corespunzator buzzerului pentru a se emite sunetul discontinuu
    case BUZZER_MOVING:
         if(b->toggle_counter >= b->movement_beep_steps)
         {
          b->toggle_counter = 0;
          b->is_on = !b->is_on;
          if(b->is_on)
          {
            setbit(*b->port_reg, b->pin);
          }
          else
          {
            clrbit(*b->port_reg, b->pin);
          }
         }
         else
         {
          b->toggle_counter++;
         }

         if(b->step_counter < b->movement_steps)
         {
           b->step_counter++;
         }
         else
         {
          b->is_on = FALSE;
          clrbit(*b->port_reg, b->pin);
          b->state = BUZZER_IDLE;
          b->step_counter = 0;
          b->old_step_counter = 0;
         }
         break;
  }
}