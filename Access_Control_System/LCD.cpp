#include "LCD.h"
#include "NVM.h"
#include "comms.h"

// Instanta LiquidCrystal 
LiquidCrystal lc_instance(RS, E, D4, D5, D6, D7);

// Se declara o variabila de tip lcd, corespunzatoare lcd-ului situat la intrarea in orgnizatie
lcd lcd_en;

// functia de initializare pentru instanta LiquidCrystal de mai sus, care foloseste lcd_state pentru gestionarea prioritatilor
void lcd_init(lcd* lcd_target, LiquidCrystal* lc_instance) 
{
  bool lcd_handshake_success = false;

  if (system_fsm.master_defekt == false) 
  {
    unsigned long start_wait = millis();

    Serial.println(F("{\"ev\":\"request_lcd_param\"}"));

    while (millis() - start_wait < TIME_FOR_WAIT_INIT_MESSAGE) 
    {
      if (Serial.available()) 
      {
        String response = Serial.readStringUntil('\n');
        response.trim();

        if (response.indexOf(F("\"ev\":\"lcd_config\"")) != -1) 
        {
          String val;

          extract_json_value(response, "feedback_ms", val);
          lcd_target->feedback_duration = (uint16_t)val.toInt();

          nvm_save_param_lcd_feedback(&lcd_target->feedback_duration);
          
          lcd_handshake_success = true;
          Serial.println(F("{\"ev\":\"lcd_hs_ok\"}"));
          break;
        }
      }
    }
  }

  if (!lcd_handshake_success) 
  {
    nvm_load_param_lcd_feedback(&lcd_target->feedback_duration);
    //Serial.println(F("{\"ev\":\"lcd_load_from_nvm\"}"));
  }

  lcd_target->change_feedback_duration = lcd_target->feedback_duration;

  lcd_target->liquid_crystal_instance = lc_instance;
  lcd_target->current_priority = MAX_UNUSED_PRIORITY;
  lcd_target->last_update = 0;
  lcd_target->msg_line_1 = F("");
  lcd_target->msg_line_2 = F("");
  lcd_target->liquid_crystal_instance->begin(16, 2);
  delay(100);
  lcd_target->liquid_crystal_instance->clear();
  delay(1);
  lcd_target->liquid_crystal_instance->setCursor(0, 0);
  
  /*
  Serial.print(F("{\"feedback_ms\":"));
  Serial.print(lcd_target->feedback_duration);
  Serial.println(F("}"));
  */
}

// Functia pentru afisarea de caractere din Flash pe lcd
void lcd_print(lcd* lcd_target, String msg_line_1, String msg_line_2, uint8_t priority) 
{
  if (priority > lcd_target->last_request_priority && (lcd_target->msg_line_1 != lcd_target->last_requested_msg_line_1 || lcd_target->msg_line_2 != lcd_target->last_requested_msg_line_2)) 
  {
    return;
  } 
  lcd_target->last_requested_msg_line_1 = msg_line_1;
  lcd_target->last_requested_msg_line_2 = msg_line_2;
  lcd_target->last_request_priority = priority;
}

void refresh_lcd(lcd* lcd_target)
{
  if( (millis() - lcd_target->last_update < lcd_target->feedback_duration) && (lcd_target->last_request_priority > lcd_target->current_priority))
  {
    return;
  }
  else
  {
    if( lcd_target->msg_line_1 != lcd_target->last_requested_msg_line_1 || lcd_target->msg_line_2 != lcd_target->last_requested_msg_line_2)
    {
      lcd_target->msg_line_1 = lcd_target->last_requested_msg_line_1;
      lcd_target->msg_line_2 = lcd_target->last_requested_msg_line_2;
      
      lcd_target->liquid_crystal_instance->clear();
      lcd_target->liquid_crystal_instance->setCursor(0,0);
      lcd_target->liquid_crystal_instance->print(lcd_target->msg_line_1);
      lcd_target->liquid_crystal_instance->setCursor(0,1);
      lcd_target->liquid_crystal_instance->print(lcd_target->msg_line_2);

      lcd_target->last_update = millis();
      lcd_target->current_priority = lcd_target->last_request_priority;
    }
  }
}

// Functia care asigura oprirea LCD in conditii de siguranta
void lcd_safe_shut_down(lcd* lcd_target)
{
  lcd_target->liquid_crystal_instance->clear();                 // Se sterge tot continutul
  lcd_target->liquid_crystal_instance->setCursor(0, 0);
  lcd_target->liquid_crystal_instance->print("Inchidere...");// Se afiseaza un mesaj scurt sugestiv
  delay(500);                  // Se lasa mesajul sa fie vizibil o perioada scurta
  lcd_target->liquid_crystal_instance->clear();                 // Se curata complet ecranul
  lcd_target->current_priority = MAX_UNUSED_PRIORITY; // Se actualizeaza prioritatea ultimului mesaj afisat la valoarea default
}



