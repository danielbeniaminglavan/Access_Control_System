#include "comms.h"
#include "NVM.h"
#include <ctype.h>

// variabila globala care retine datele necesare pentru gestionarea modurilor de lucru ale sistemului
system_fsm_t system_fsm;

bool msg_contains(const String& s, const __FlashStringHelper* key) {
  return s.indexOf(key) != -1;
}

void extract_json_value(String& msg, const String& key, String& target) 
{
  target = ""; 
  
  String search_str = "\"" + key + "\":\"";
  
  int p_idx = msg.indexOf(search_str);
  if (p_idx != -1) 
  {
    int start = p_idx + search_str.length();

    int end = msg.indexOf(F("\""), start);
    
    if (end != -1) 
    {
      target = msg.substring(start, end);
    }
  }
}

const __FlashStringHelper* get_sys_mode_name(system_mode_t mode) 
{
  switch (mode) 
  {
    case SYS_ACCESS_MODE: return F("access");
    case SYS_WRITE_MODE:  return F("write");
    default:              return F("unknown");
  }
}

const __FlashStringHelper* get_rec_mode_name(system_rec_mode_t mode) 
{
  switch (mode) 
  {
    case REC_ONLY_WITH_CARD:                    return F("rec_only_with_card");
    case REC_CARD_PLUS_GUESTS:                  return F("rec_card_plus_guests");
    case REC_CARD_PLUS_FAC_ENC:                 return F("rec_card_plus_fac_enc");
    case REC_CARD_PLUS_FAC_ENC_PLUS_GUESTS:     return F("rec_card_f_enc_guests");
    case REC_FAC_ENC:                           return F("rec_fac_enc");
    case REC_FAC_ENC_PLUS_GUESTS:               return F("rec_fac_enc_plus_guests");
    case REC_ONLY_ACC:                          return F("rec_only_acc");
    default:                                    return F("unknown");
  }
}

bool compare_signature(const uint8_t* card_data, const char* hex_signature) 
{
  if (strlen(hex_signature) < 32) return false;

  for (uint8_t i = 0; i < 16; i++) 
  {
    uint8_t high = hex_char_to_nibble(hex_signature[i * 2]);
    uint8_t low  = hex_char_to_nibble(hex_signature[i * 2 + 1]);
    uint8_t signature_byte = (high << 4) | low;

    if (card_data[i] != signature_byte) 
    {
      return false; 
    }
  }
  return true; 
}

// initializarea datelor necesare pentru gestionarea modurilor de lucru ale sistemului din structura system.Fsm
void comms_init() 
{
  bool handshake_success = false;
  bool system_params_b_success = false;
  bool system_params_t_success = false;
  unsigned long start_wait;

  start_wait = millis();
  Serial.println(F("{\"ev\":\"hs_req\"}"));

  while (millis() - start_wait < TIME_FOR_WAIT_INIT_MESSAGE) 
  {
    if (Serial.available()) 
    {
      String response = Serial.readStringUntil('\n');
      response.trim();

      if (response.indexOf(F("\"ev\":\"hs_conf\"")) != -1) 
      {
        handshake_success = true;
        system_fsm.master_defekt = false; 
        break;
      }
    }
  }

  if (handshake_success) 
  {
    start_wait = millis();
    Serial.println(F("{\"ev\":\"req_sysb\"}"));

    while (millis() - start_wait < TIME_FOR_WAIT_INIT_MESSAGE) 
    {
      if (Serial.available()) 
      {
        String response = Serial.readStringUntil('\n');
        response.trim();

        if (response.indexOf(F("\"ev\":\"sys_b\"")) != -1) 
        {
          String val;

          extract_json_value(response, "m", val);
          system_fsm.conf_params.mode = (system_mode_t)val.toInt();

          extract_json_value(response, "r", val);
          system_fsm.conf_params.system_rec_mode = (system_rec_mode_t)val.toInt();

          system_params_b_success = true;
          break;
        }
      }
    }

    start_wait = millis();
    Serial.println(F("{\"ev\":\"req_syst\"}"));

    while (millis() - start_wait < TIME_FOR_WAIT_INIT_MESSAGE) 
    {
      if (Serial.available()) 
      {
        String response = Serial.readStringUntil('\n');
        response.trim();

        if (response.indexOf(F("\"e\":\"sys_t\"")) != -1) 
        {
          String val;

          extract_json_value(response, "vf", val);
          system_fsm.conf_params.time_for_verify_f_enc = (uint16_t)val.toInt();

          extract_json_value(response, "vc", val);
          system_fsm.conf_params.time_for_verify_card = (uint16_t)val.toInt();

          extract_json_value(response, "wc", val);
          system_fsm.conf_params.time_for_wait_card = (uint32_t)val.toInt();

          extract_json_value(response, "cf", val);
          system_fsm.conf_params.time_for_calc_fac_enc = (uint32_t)val.toInt();

          system_params_t_success = true;
          break;
        }
      }
    }
  }

  if (!system_params_b_success || !system_params_t_success) 
  {
    nvm_load_block_system(&system_fsm.conf_params);

    if (!handshake_success) 
    {
      system_fsm.master_defekt = true;
      //Serial.println(F("{\"ev\":\"hs_failed\"}"));
    }

    system_fsm.conf_params.mode = SYS_ACCESS_MODE;
    system_fsm.conf_params.system_rec_mode = REC_ONLY_ACC;
  }

  system_fsm.last_mode = system_fsm.conf_params.mode;
  system_fsm.change_rec_mode_request = system_fsm.conf_params.system_rec_mode;
  system_fsm.change_time_for_wait_card = system_fsm.conf_params.time_for_wait_card;
  system_fsm.change_time_for_verify_card = system_fsm.conf_params.time_for_verify_card;
  system_fsm.change_time_for_verify_f_enc = system_fsm.conf_params.time_for_verify_f_enc;
  system_fsm.change_time_for_calc_fac_enc = system_fsm.conf_params.time_for_calc_fac_enc;
  system_fsm.access_state = ACCESS_WAIT_PERSON;
  system_fsm.write_state = WRITE_WAIT_CMD;
  system_fsm.write_cmd = WRITE_CARD_FULL;
  system_fsm.payload = "";
  system_fsm.timer = 0;
  system_fsm.timer_sync = 0;
  system_fsm.person_detected = false;
  system_fsm.message_available = "";

  /*
  Serial.print(F("{\"w_card\":"));
  Serial.print(system_fsm.conf_params.time_for_wait_card);
  Serial.print(F("{\"calc_enc\":"));
  Serial.print(system_fsm.conf_params.time_for_calc_fac_enc);
  Serial.print(F("{\"ver_card\":"));
  Serial.print(system_fsm.conf_params.time_for_verify_card);
  Serial.print(F("{\"ver_enc\":"));
  Serial.print(system_fsm.conf_params.time_for_verify_f_enc);
  Serial.print(F(",\"sys_mode\":\""));
  Serial.print(get_sys_mode_name(system_fsm.conf_params.mode)); 
  Serial.println(F("}"));
  */
}

// functie care actualizeaza starea sistemului
void comms_update() 
{
  if (millis() - system_fsm.timer_sync > TIME_FOR_WAIT_MASTER_AV_RESPONSE)
  {
    if (!system_fsm.master_defekt)
    {
      system_fsm.master_defekt = true;
      system_fsm.conf_params.mode = SYS_ACCESS_MODE;
      system_fsm.conf_params.system_rec_mode = REC_ONLY_ACC;

      lcd_en_print(F("Raspberry OFF   "), F("Doar acces      "), FEEDBACK_PRIORITY);
      as_buzzer_command_warning();
    }
  }

  if (Serial.available()) 
  {
    system_fsm.message_available = Serial.readStringUntil('\n');
    system_fsm.message_available.trim();

    if (system_fsm.message_available.indexOf(F("\"ev\":\"hb\"")) != -1)
    {
      system_fsm.timer_sync = millis();
      system_fsm.master_defekt = false;
    }
    if(system_fsm.message_available.indexOf(F("\"ev\":\"signaling\"")) != -1)
    {
      if((system_fsm.conf_params.mode == SYS_ACCESS_MODE && (system_fsm.access_state == ACCESS_VERIFY || system_fsm.access_state == ACCESS_WAIT_CARD)) || (system_fsm.conf_params.mode == SYS_WRITE_MODE && system_fsm.write_state == WRITE_COMPLETE_REGISTRATION))
      {
        if(system_fsm.message_available.indexOf(F("\"type\":\"on\"")) != -1)
        {
          signal_succes();
        }
        else if(system_fsm.message_available.indexOf(F("\"type\":\"off\"")) != -1)
        {
          signal_error();
        }
      }
    }
    else if(system_fsm.message_available.indexOf(F("\"ev\":\"command\"")) != -1 && 
        system_fsm.message_available.indexOf(F("\"door\":\"open\"")) != -1)
    {
      as_servo_command_open();
      lcd_en_print(F("Acces permis    "), F("               "), FEEDBACK_PRIORITY);
      system_fsm.message_available = "";
    } 
    else if(system_fsm.message_available.indexOf(F("\"ev\":\"change_sys_mode\"")) != -1)
    {
      switch (system_fsm.conf_params.mode) 
      {
        case SYS_WRITE_MODE:
          system_fsm.conf_params.mode = SYS_ACCESS_MODE;
          system_fsm.access_state = ACCESS_WAIT_PERSON;
          signal_nothing();
          break;

        case SYS_ACCESS_MODE:
          system_fsm.conf_params.mode = SYS_WRITE_MODE;
          signal_nothing();
          system_fsm.person_detected = false;
          break;
      }

      system_fsm.message_available = "";
    }
    else if(system_fsm.message_available.indexOf(F("\"ev\":\"ch_param\"")) != -1)
    {
      bool parameter_unplausibel = false;
      String val;
      if (system_fsm.message_available.indexOf(F("\"ch\":\"rec_m\"")) != -1)
      {
        if( system_fsm.message_available.indexOf(F("\"new_m\":\"rec_only_with_card\"")) != -1)
        {
          system_fsm.change_rec_mode_request = REC_ONLY_WITH_CARD;
        }
        else if( system_fsm.message_available.indexOf(F("\"new_m\":\"rec_card_plus_guests\"")) != -1)
        {
          system_fsm.change_rec_mode_request = REC_CARD_PLUS_GUESTS;
        }
        else if( system_fsm.message_available.indexOf(F("\"new_m\":\"rec_card_plus_fac_enc\"")) != -1)
        {
          system_fsm.change_rec_mode_request = REC_CARD_PLUS_FAC_ENC;
        }
        else if( system_fsm.message_available.indexOf(F("\"new_m\":\"rec_card_f_enc_guests\"")) != -1)
        {
          system_fsm.change_rec_mode_request = REC_CARD_PLUS_FAC_ENC_PLUS_GUESTS;
        }
        else if( system_fsm.message_available.indexOf(F("\"new_m\":\"rec_fac_enc\"")) != -1)
        {
          system_fsm.change_rec_mode_request = REC_FAC_ENC;
        }
        else if( system_fsm.message_available.indexOf(F("\"new_m\":\"rec_fac_enc_plus_guests\"")) != -1)
        {
          system_fsm.change_rec_mode_request = REC_FAC_ENC_PLUS_GUESTS;
        }
        else if( system_fsm.message_available.indexOf(F("\"new_m\":\"rec_only_acc\"")) != -1)
        {
          system_fsm.change_rec_mode_request = REC_ONLY_ACC;
        }
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"t_ver_enc\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "v_face", val);
        uint16_t val_req = (uint16_t)val.toInt();

        if(val_req > 1999 && val_req < 60001)
        {
          system_fsm.change_time_for_verify_f_enc = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        }  
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"t_ver_card\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "v_card", val);
        uint16_t val_req = (uint16_t)val.toInt();

        if(val_req > 200 && val_req < 1000)
        {
          system_fsm.change_time_for_verify_card = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        } 
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"t_wait_c\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "wait_c", val);
        uint32_t val_req = (uint32_t)val.toInt();

        if(val_req > 1999 && val_req < 120001)
        {
          system_fsm.change_time_for_wait_card = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        } 
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"t_calc_enc\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "calc_f", val);
        uint32_t val_req = (uint32_t)val.toInt();

        if(val_req > 1999 && val_req < 120001)
        {
          system_fsm.change_time_for_calc_fac_enc = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        } 
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"t_wr_c\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "wr_t", val);
        uint32_t val_req = (uint32_t)val.toInt();

        if(val_req > 1999 && val_req < 120001)
        {
          nfc.change_time_for_write_card = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        } 
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"data_acc\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "sig", val);
        size_t len = val.length();

        if (len != 32) 
        {
          parameter_unplausibel = true;
        }
        else
        {
          bool valid_hex = true;

          for (size_t i = 0; i < len; i++)
          {
            char c = val[i];
            if (!isxdigit(c))  
            {
              valid_hex = false;
              break;
            }
          }

          if (!valid_hex)
          {
            parameter_unplausibel = true;
          }
          else
          {
            memset(nfc.change_data_acc_only, 0, sizeof(nfc.change_data_acc_only));
            memcpy(nfc.change_data_acc_only, val.c_str(), 32); 
            nfc.change_data_acc_only[32] = '\0';
          }
        }
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"v_err_t\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "err_t", val);
        uint16_t val_req = (uint16_t)val.toInt();  
        if(val_req > 499 && val_req < 1501)
        {
          buzzer.change_error_time = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        } 
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"v_warn_t\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "warn_t", val);
        uint16_t val_req_warn_tm = (uint16_t)val.toInt();
        extract_json_value(system_fsm.message_available, "warn_p", val);
        uint16_t val_req_warn_p = (uint16_t)val.toInt(); 
        if(((val_req_warn_tm > 199 && val_req_warn_tm < 601) && val_req_warn_tm % 50 == 0) && (val_req_warn_tm % (2*val_req_warn_p) == 0))
        {
          buzzer.change_warning_time = val_req_warn_tm;
          buzzer.change_warning_beep_period = val_req_warn_p;
        }
        else
        {
          parameter_unplausibel = true;
        } 
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"v_mov_t\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "mov_t", val);
        uint8_t val_req_tm = (uint8_t)val.toInt(); 
        extract_json_value(system_fsm.message_available, "mov_p", val);
        uint16_t val_req_p = (uint16_t)val.toInt();

        if ((val_req_tm > 1 && val_req_tm < 7) && (((uint32_t)val_req_tm * 1000U) % ((uint32_t)2 * val_req_p) == 0) && (((uint32_t)val_req_tm * 1000U) / (uint32_t)val_req_p > 3))
        {
          buzzer.change_movement_beep_period = val_req_p;
          servo.change_moving_time = val_req_tm;
        }
        else
        {
          parameter_unplausibel = true;
        }   
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"open_t\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "open_time", val);
        uint8_t val_req = (uint8_t)val.toInt();

        if(val_req > 0 && val_req < 11)
        {
          servo.change_open_time = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        }   
      }
      else if(system_fsm.message_available.indexOf(F("\"ch\":\"feedback_d\"")) != -1)
      {
        extract_json_value(system_fsm.message_available, "feedback_ms", val);
        uint16_t val_req = (uint16_t)val.toInt();

        if(val_req > 999 && val_req < 4001)
        {
          lcd_en.change_feedback_duration = val_req;
        }
        else
        {
          parameter_unplausibel = true;
        }            
      }

      if(parameter_unplausibel)
      {
        Serial.println(F("{\"ev\":\"p_unplausibel\"}"));
      }
    }
  }
  else
  {
    system_fsm.message_available = "";
  }

  if((system_fsm.conf_params.mode == SYS_ACCESS_MODE && system_fsm.access_state == ACCESS_WAIT_PERSON) || (system_fsm.conf_params.mode == SYS_WRITE_MODE && system_fsm.write_state == WRITE_WAIT_CMD))
  {
    bool params_changed = false;

    if( system_fsm.conf_params.system_rec_mode != system_fsm.change_rec_mode_request)
    {
      system_fsm.conf_params.system_rec_mode = system_fsm.change_rec_mode_request;

      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"rec_mode\",\"v\":\""));
      Serial.print((int)system_fsm.conf_params.system_rec_mode);
      Serial.println(F("\"}"));

      switch(system_fsm.conf_params.system_rec_mode)
      {
        case REC_ONLY_WITH_CARD:
          lcd_en_print(F("Mod rec schimbat"), F("Doar card       "), FEEDBACK_PRIORITY);
          break;
        case REC_CARD_PLUS_GUESTS:
          lcd_en_print(F("Mod rec schimbat"), F("Card+Vizitatori "), FEEDBACK_PRIORITY);
          break;
        case REC_CARD_PLUS_FAC_ENC:
          lcd_en_print(F("Mod rec schimbat"), F("Card + Amprenta "), FEEDBACK_PRIORITY);
          break;
        case REC_CARD_PLUS_FAC_ENC_PLUS_GUESTS:
          lcd_en_print(F("Mod rec schimbat"), F("Card + Amp + Viz"), FEEDBACK_PRIORITY);
          break;
        case REC_FAC_ENC:
          lcd_en_print(F("Mod rec schimbat"), F("Amprenta faciala"), FEEDBACK_PRIORITY);
          break;
        case REC_FAC_ENC_PLUS_GUESTS:
          lcd_en_print(F("Mod rec schimbat"), F("Amp + Vizitatori"), FEEDBACK_PRIORITY);
          break;
        case REC_ONLY_ACC:
          lcd_en_print(F("Mod rec schimbat"), F("Doar acces      "), FEEDBACK_PRIORITY);
          break;
      }
      
      as_buzzer_command_warning();
      nvm_save_param_rec_mode(&system_fsm.conf_params.system_rec_mode);
    }
    
    if(system_fsm.change_time_for_verify_f_enc != system_fsm.conf_params.time_for_verify_f_enc)
    {
      system_fsm.conf_params.time_for_verify_f_enc = system_fsm.change_time_for_verify_f_enc;
      nvm_save_param_time_for_verify_f_enc(&system_fsm.conf_params.time_for_verify_f_enc);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"t_ver_enc\",\"v\":"));
      Serial.print(system_fsm.conf_params.time_for_verify_f_enc);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(system_fsm.change_time_for_verify_card != system_fsm.conf_params.time_for_verify_card)
    {
      system_fsm.conf_params.time_for_verify_card = system_fsm.change_time_for_verify_card;
      nvm_save_param_time_for_verify_card(&system_fsm.conf_params.time_for_verify_card);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"t_ver_card\",\"v\":"));
      Serial.print(system_fsm.conf_params.time_for_verify_card);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(system_fsm.change_time_for_wait_card != system_fsm.conf_params.time_for_wait_card)
    {
      system_fsm.conf_params.time_for_wait_card = system_fsm.change_time_for_wait_card;
      nvm_save_param_time_for_wait_card(&system_fsm.conf_params.time_for_wait_card);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"t_wait_c\",\"v\":"));
      Serial.print(system_fsm.conf_params.time_for_wait_card);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(system_fsm.change_time_for_calc_fac_enc != system_fsm.conf_params.time_for_calc_fac_enc)
    {
      system_fsm.conf_params.time_for_calc_fac_enc = system_fsm.change_time_for_calc_fac_enc;
      nvm_save_param_time_for_calc_fac_enc(&system_fsm.conf_params.time_for_calc_fac_enc);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"t_calc_enc\",\"v\":"));
      Serial.print(system_fsm.conf_params.time_for_calc_fac_enc);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(nfc.change_time_for_write_card != nfc.conf_params.time_for_write_card)
    {
      nfc.conf_params.time_for_write_card = nfc.change_time_for_write_card;
      nvm_save_param_time_for_write_card(&nfc.conf_params.time_for_write_card);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"t_wr_card\",\"v\":"));
      Serial.print(nfc.conf_params.time_for_write_card);
      Serial.println(F("}"));
      params_changed = true;
    }

    if (strncmp(nfc.change_data_acc_only, nfc.conf_params.data_acc_only, 32) != 0)
    {
      memcpy(nfc.conf_params.data_acc_only, nfc.change_data_acc_only, 32);
      nfc.conf_params.data_acc_only[32] = '\0';
      nvm_save_param_data_only_acc(&nfc.conf_params.data_acc_only);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"data_acc\",\"v\":\""));
      Serial.print(nfc.conf_params.data_acc_only);
      Serial.println(F("\"}"));
      params_changed = true;
    }

    if(buzzer.change_error_time != buzzer.conf_parameters.error_time)
    {
      buzzer.conf_parameters.error_time = buzzer.change_error_time;
      buzzer.error_steps = buzzer.conf_parameters.error_time / buzzer.update_interval;
      nvm_save_param_error_time(&buzzer.conf_parameters.error_time);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"v_err_t\",\"v\":"));
      Serial.print(buzzer.conf_parameters.error_time);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(buzzer.change_warning_time != buzzer.conf_parameters.warning_time)
    {
      buzzer.conf_parameters.warning_time = buzzer.change_warning_time;
      buzzer.warning_steps = buzzer.conf_parameters.warning_time / buzzer.update_interval;
      nvm_save_param_warning_time(&buzzer.conf_parameters.warning_time);
      params_changed = true;
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"warn_t\",\"v\":"));
      Serial.print(buzzer.conf_parameters.warning_time);
      Serial.println(F("}"));
    }

    if(buzzer.change_warning_beep_period != buzzer.conf_parameters.warning_beep_period)
    {
      buzzer.conf_parameters.warning_beep_period = buzzer.change_warning_beep_period;
      buzzer.warning_beep_steps = buzzer.conf_parameters.warning_beep_period / buzzer.update_interval;
      nvm_save_param_warning_beep_period(&buzzer.conf_parameters.warning_beep_period);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"warn_p\",\"v\":"));
      Serial.print(buzzer.conf_parameters.warning_beep_period);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(servo.change_moving_time != servo.conf_parameters.moving_time)
    {
      servo.conf_parameters.moving_time = servo.change_moving_time;
      servo.moving_steps = ((uint32_t)servo.conf_parameters.moving_time * 1000UL) / servo.update_interval;
      buzzer.movement_beep_steps = buzzer.conf_parameters.movement_beep_period / buzzer.update_interval;
      nvm_save_param_servo_moving_time(&servo.conf_parameters.moving_time);
      buzzer.movement_steps = ((uint32_t)buzzer.servo->conf_parameters.moving_time * 1000UL) / buzzer.update_interval;
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"mov_t\",\"v\":"));
      Serial.print(servo.conf_parameters.moving_time);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(buzzer.change_movement_beep_period != buzzer.conf_parameters.movement_beep_period)
    {
      buzzer.conf_parameters.movement_beep_period = buzzer.change_movement_beep_period;
      buzzer.movement_steps = ((uint32_t)buzzer.servo->conf_parameters.moving_time * 1000UL) / buzzer.update_interval;
      buzzer.movement_beep_steps = buzzer.conf_parameters.movement_beep_period / buzzer.update_interval;
      buzzer.min_moving_steps = (uint8_t)(((uint32_t)2 * BUZZER_MIN_MOVING_CYCLES * buzzer.conf_parameters.movement_beep_period) / buzzer.update_interval);
      nvm_save_param_movement_beep_period(&buzzer.conf_parameters.movement_beep_period);
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"mov_p\",\"v\":"));
      Serial.print(buzzer.conf_parameters.movement_beep_period);
      Serial.println(F("}"));
      params_changed = true;
    }

    if(servo.change_open_time != servo.conf_parameters.open_time)
    {
      servo.conf_parameters.open_time = servo.change_open_time;
      servo.open_steps   = ((uint32_t)servo.conf_parameters.open_time   * 1000UL) / servo.update_interval;
      nvm_save_param_servo_open_time(&servo.conf_parameters.open_time);
      params_changed = true;
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"open_t\",\"v\":"));
      Serial.print(servo.conf_parameters.open_time);
      Serial.println(F("}")); 
    }

    if(lcd_en.change_feedback_duration != lcd_en.feedback_duration)
    {
      lcd_en.feedback_duration = lcd_en.change_feedback_duration;
      nvm_save_param_lcd_feedback(&lcd_en.feedback_duration );
      Serial.print(F("{\"ev\":\"p_ch\",\"key\":\"feedback_d\",\"v\":"));
      Serial.print(lcd_en.feedback_duration);
      Serial.println(F("}"));
      params_changed = true; 
    }

    if(params_changed)
    {
      //Serial.println(F("{\"ev\":\"param_changed\"}"));
      as_buzzer_command_warning();
      lcd_en_print(F("Param actualizat"), F("cu succes!      "), FEEDBACK_PRIORITY);
    }
  }
            
  // se detecteaza schimbarea modului de lucru
  if (system_fsm.conf_params.mode != system_fsm.last_mode) 
  {
    switch (system_fsm.conf_params.mode) 
    {
      case SYS_ACCESS_MODE:
        Serial.println(F("{\"ev\":\"access_mode_on\"}"));
        lcd_en_print(F("Control acces   "), F("activ !         "), FEEDBACK_PRIORITY);
        system_fsm.access_state = ACCESS_WAIT_PERSON;
        system_fsm.timer = millis();
        break;

      case SYS_WRITE_MODE:
        Serial.println(F("{\"ev\":\"write_mode_on\"}"));
        lcd_en_print(F("Mod scriere     "), F("carduri activ !  "), FEEDBACK_PRIORITY);
        break;
    }
    // se actualizeaza ultimul mod de lucru cunoscut al sistemului 
    system_fsm.last_mode = system_fsm.conf_params.mode;
    as_buzzer_command_warning();
    nvm_save_param_sys_mode(&system_fsm.conf_params.mode);
  }

  // FSM-ul care gestioneaza modul de lucru al sistemului
  switch (system_fsm.conf_params.mode) {
    // in cazul in care sistemul se afla in modul de control al accesului,se tine cont de sub-starea activa corespunzatoare acestui mod de lucru
    case SYS_ACCESS_MODE:
      
      switch (system_fsm.access_state) 
      {
        // starea in care se asteapta aparitia unei persoane
        case ACCESS_WAIT_PERSON:
          
          if (en_pir_detect()) 
          {
            if (system_fsm.conf_params.system_rec_mode == REC_FAC_ENC) 
            {
              //lcd_en_print(F("Priviti catre   "), F("camera...       "), INSTRUCTION_PRIORITY);
              
              system_fsm.access_state = ACCESS_VERIFY;
              system_fsm.threshold_for_rec_mess = system_fsm.conf_params.time_for_verify_f_enc;
              //as_buzzer_command_warning();
            } 
            else 
            {
              system_fsm.access_state = ACCESS_WAIT_CARD;
              if(system_fsm.conf_params.system_rec_mode == REC_FAC_ENC)
              {
                system_fsm.time_in_access_wait_card = system_fsm.conf_params.time_for_verify_f_enc;
              }
              else
              {
                system_fsm.time_in_access_wait_card = system_fsm.conf_params.time_for_wait_card;
              }
            }

            system_fsm.timer = millis();
            
            if (system_fsm.conf_params.system_rec_mode == REC_FAC_ENC || system_fsm.conf_params.system_rec_mode == REC_FAC_ENC_PLUS_GUESTS) 
            {
              Serial.println(F("{\"ev\":\"person_detected\"}"));
            }

            if (!system_fsm.person_detected) 
            {
              /*if(system_fsm.system_rec_mode == REC_FAC_ENC || system_fsm.system_rec_mode == REC_FAC_ENC_PLUS_GUESTS)
              {
                Serial.println(F("{\"ev\":\"person_detected\"}"));
              }*/
              system_fsm.person_detected = true;
              //system_fsm.timer = millis();
            }
          } 
          else 
          {
            if (system_fsm.person_detected) 
            {
              //Serial.println(F("{\"ev\":\"no_person\"}"));
              system_fsm.person_detected = false;
            }
            lcd_en_print(F("Univ din Craiova"), F("                "), INSTRUCTION_PRIORITY);
          }
          break;

        // starea in care se asteapta scanarea cardului
        case ACCESS_WAIT_CARD:
        {
          if (millis() - system_fsm.timer > system_fsm.time_in_access_wait_card) 
          {
            system_fsm.access_state = ACCESS_WAIT_PERSON;
            signal_nothing();
            break;
          }

          if (system_fsm.message_available != "") 
          {
            if (system_fsm.message_available.indexOf("\"access\":\"granted\"") != -1) 
            {
              String name, dir;
              extract_json_value(system_fsm.message_available, "user", name);
              extract_json_value(system_fsm.message_available, "dir", dir);
              String first_line;
              String second_line;

              if(name != NULL)
              {
                first_line = "Acces permis pt ";
                second_line = name.substring(0, 10) + " ("+ dir +")";
              }
              else
              {
                first_line = "Acces permis    ";
                second_line = "";
              }
              
              as_servo_command_open();
              lcd_en_print(first_line, second_line, FEEDBACK_PRIORITY);
              system_fsm.access_state = ACCESS_WAIT_PERSON;
              signal_nothing();
            }
          }

          if(system_fsm.conf_params.system_rec_mode == REC_FAC_ENC_PLUS_GUESTS) 
          {
            lcd_en_print(F("Priviti camera /"), F("Scanati cardul  "), INSTRUCTION_PRIORITY);
          } 
          else 
          {
            lcd_en_print(F("Scanati cardul  "), F("                "), INSTRUCTION_PRIORITY);
          }

          bool card_read_success = false;
          if (system_fsm.conf_params.system_rec_mode == REC_ONLY_ACC || system_fsm.conf_params.system_rec_mode == REC_FAC_ENC_PLUS_GUESTS) 
          {
            card_read_success = read_card_only_acc_en();
          } 
          else if (system_fsm.conf_params.system_rec_mode == REC_CARD_PLUS_FAC_ENC || system_fsm.conf_params.system_rec_mode == REC_ONLY_WITH_CARD) 
          {
            card_read_success = read_card_en();
          } 
          else 
          {
            card_read_success = read_card_with_acc_en();
          }

          if (card_read_success) 
          {
            if (system_fsm.conf_params.system_rec_mode == REC_ONLY_ACC || system_fsm.conf_params.system_rec_mode == REC_FAC_ENC_PLUS_GUESTS) 
            {
              if (compare_signature(nfc.last_data_read, nfc.conf_params.data_acc_only)) 
              {
                as_servo_command_open();
                lcd_en_print(F("Acces permis    "), F("                "), FEEDBACK_PRIORITY);
              } 
              else 
              {
                as_buzzer_command_error();
                lcd_en_print(F("Acces respins   "), F("                "), FEEDBACK_PRIORITY);
              }

              system_fsm.access_state = ACCESS_WAIT_PERSON;
              signal_nothing();
              break;
            } 
            else 
            {
              if (system_fsm.conf_params.system_rec_mode == REC_CARD_PLUS_FAC_ENC_PLUS_GUESTS || system_fsm.conf_params.system_rec_mode == REC_CARD_PLUS_FAC_ENC) 
              {
                system_fsm.threshold_for_rec_mess = system_fsm.conf_params.time_for_verify_f_enc;
              }
              else
              {
                system_fsm.threshold_for_rec_mess = system_fsm.conf_params.time_for_verify_card;
              }
              
              system_fsm.timer = millis();
              as_buzzer_command_warning();
              system_fsm.access_state = ACCESS_VERIFY;
              break;
            }
          }
        }
        break;

        // starea in care se asteapta confirmarea accesului dupa verificarea amprentei faciale
        case ACCESS_VERIFY:
        {
          if (millis() - system_fsm.timer > system_fsm.threshold_for_rec_mess) 
          {
            if(system_fsm.conf_params.system_rec_mode == REC_FAC_ENC)
            {
              system_fsm.access_state = ACCESS_WAIT_PERSON;
              signal_nothing();
            }
            else
            {
              system_fsm.access_state = ACCESS_ERROR;
              signal_nothing();
            }
            break;
          }

          if (system_fsm.conf_params.system_rec_mode == REC_FAC_ENC || system_fsm.conf_params.system_rec_mode == REC_CARD_PLUS_FAC_ENC_PLUS_GUESTS || system_fsm.conf_params.system_rec_mode == REC_CARD_PLUS_FAC_ENC) 
            {
              lcd_en_print(F("Priviti catre   "), F("camera...       "), INSTRUCTION_PRIORITY);
            }

          if (system_fsm.message_available != "") 
          {
            if (system_fsm.message_available.indexOf("\"access\":\"granted\"") != -1) 
            {
              String name, dir;
              extract_json_value(system_fsm.message_available, "user", name);
              extract_json_value(system_fsm.message_available, "dir", dir);
              String first_line;
              String second_line;

              if(name != NULL)
              {
                first_line = "Acces permis pt ";
                second_line = name.substring(0, 10) + " ("+ dir +")";
              }
              else
              {
                first_line = "Acces permis    ";
                second_line = "";
              }
              
              as_servo_command_open();
              lcd_en_print(first_line, second_line, FEEDBACK_PRIORITY);
              system_fsm.access_state = ACCESS_WAIT_PERSON;
              signal_nothing();
            } 
            else if (system_fsm.message_available.indexOf("\"access\":\"denied\"") != -1) 
            {
              lcd_en_print(F("Acces respins   "), F("                "), FEEDBACK_PRIORITY);
              as_buzzer_command_error();
              system_fsm.access_state = ACCESS_WAIT_PERSON;
              signal_nothing();
            }
          }
        }
        break;

        // starea in care se ajunge in urma unei erori a sistemului
        case ACCESS_ERROR:
          lcd_en_print(F("Eroare sistem   "), F("                "), FEEDBACK_PRIORITY);
          as_buzzer_command_error();
          system_fsm.access_state = ACCESS_WAIT_PERSON;
          break;
      }
      break;

    // starea in care sistemul asteapta comenzi pentru scriere pe carduri
    case SYS_WRITE_MODE:
      switch(system_fsm.write_state)
      {
        case WRITE_WAIT_CMD:
          lcd_en_print(F("Se asteapta cmd "), F("pt scriere...   "), INSTRUCTION_PRIORITY);
          
          if (system_fsm.message_available == "") break;

          if (system_fsm.message_available.indexOf("\"ev\":\"cap_f_enc\"") != -1)
          {
            lcd_en_print(F("Priviti catre   "), F("camera...       "), INSTRUCTION_PRIORITY);
            
            system_fsm.write_state = WRITE_COMPLETE_REGISTRATION;
            system_fsm.timer = millis();
            as_buzzer_command_warning();
          }
          else if (system_fsm.message_available.indexOf(F("\"ev\":\"cw\"")) != -1) 
          {
            extract_json_value(system_fsm.message_available, "p", system_fsm.payload);

            if (system_fsm.message_available.indexOf(F("\"t\":\"wc\"")) != -1) 
            {
              system_fsm.write_cmd = WRITE_CARD_FULL;
            } 
            else if (system_fsm.message_available.indexOf(F("\"t\":\"wca\"")) != -1) 
            {
              system_fsm.write_cmd = WRITE_CARD_ONLY_ACC;
            } 
            else if (system_fsm.message_available.indexOf(F("\"t\":\"wci\"")) != -1) 
            {
              system_fsm.write_cmd = WRITE_CARD_ONLY_ID;
            } 
            else if (system_fsm.message_available.indexOf(F("\"t\":\"dc\"")) != -1) 
            {
              system_fsm.write_cmd = DEL_CARD_FULL;
            } 
            else if (system_fsm.message_available.indexOf(F("\"t\":\"dca\"")) != -1) 
            {
              system_fsm.write_cmd = DEL_CARD_ONLY_ACC;
            } 
            else if (system_fsm.message_available.indexOf(F("\"t\":\"dci\"")) != -1)
            {
              system_fsm.write_cmd = DEL_CARD_ONLY_ID;
            }

            lcd_en_print(F("Apropiati cardul"), F("                "), INSTRUCTION_PRIORITY);
            
            system_fsm.write_state = WRITE_WAIT_CARD;
            as_buzzer_command_warning();
            system_fsm.timer = millis();
          }
        break;

        case WRITE_COMPLETE_REGISTRATION:

          if (millis() - system_fsm.timer > system_fsm.conf_params.time_for_calc_fac_enc) 
          {
            lcd_en_print(F("Timpul a expirat"), F("                "), FEEDBACK_PRIORITY);

            as_buzzer_command_error();
            system_fsm.write_state = WRITE_WAIT_CMD;
            signal_nothing();
            break;
          }

          if (system_fsm.message_available == "") break;
          else
          {
            if (system_fsm.message_available.indexOf(F("\"ev\":\"cw\"")) != -1) 
            {
              extract_json_value(system_fsm.message_available, "p", system_fsm.payload);

              lcd_en_print(F("Apropiati cardul"), F("                "), INSTRUCTION_PRIORITY);

              as_buzzer_command_warning();
              system_fsm.write_cmd = WRITE_CARD_FULL;
              system_fsm.write_state = WRITE_WAIT_CARD;
              signal_nothing();
              system_fsm.timer = millis();
            } 
            else
            {
              bool feedback_err_received = false;
              if (system_fsm.message_available.indexOf("\"ev\":\"timeout_reached\"") != -1)
              {
                lcd_en_print(F("Timpul a expirat"), F("Reicercati!     "), FEEDBACK_PRIORITY);
                feedback_err_received = true;
              }
              else if (system_fsm.message_available.indexOf("\"ev\":\"mult_fd\"") != -1)
              {
                lcd_en_print(F("Fete multiple   "), F("detectate...    "), FEEDBACK_PRIORITY);
                feedback_err_received = true;
              }
              else if (system_fsm.message_available.indexOf("\"ev\":\"registration_error\"") != -1)
              {
                lcd_en_print(F("Eroare la       "), F("inregistare     "), FEEDBACK_PRIORITY);
                feedback_err_received = true;
              }
              else if (system_fsm.message_available.indexOf("\"ev\":\"insuf_fr\"") != -1)
              {
                lcd_en_print(F("Prea putine     "), F("cadre colectate "), FEEDBACK_PRIORITY);
                feedback_err_received = true;
              }
              else if (system_fsm.message_available.indexOf(F("\"ev\":\"write_abort\"")) != -1) 
              {
                lcd_en_print(F("Inregistrare    "), F("intrerupta...   "), FEEDBACK_PRIORITY);
                feedback_err_received = true;
              }
              else if (system_fsm.message_available.indexOf("\"ev\":\"no_face\"") != -1)
              {
                lcd_en_print(F("Nicio fata...   "), F("Reicercati!     "), FEEDBACK_PRIORITY);
                feedback_err_received = true;
              }
              
              if(feedback_err_received)
              {
                as_buzzer_command_error();
                system_fsm.write_state = WRITE_WAIT_CMD;
                signal_nothing();
              }
            }    
          }
        break;

        case WRITE_WAIT_CARD:

          write_card_return_t result = NO_CARD;

          switch(system_fsm.write_cmd)
          {
            case WRITE_CARD_FULL:
              result = write_card_en(system_fsm.payload.c_str());
            break;

            case WRITE_CARD_ONLY_ACC:
              result = write_card_only_acc_en();
            break;

            case WRITE_CARD_ONLY_ID:
              result = write_card_only_id_en(system_fsm.payload.c_str());
            break;

            case DEL_CARD_FULL:
              result = del_card_en();
            break;

            case DEL_CARD_ONLY_ACC:
              result = del_card_only_acc_en();
            break;

            case DEL_CARD_ONLY_ID:
              result = del_card_only_id_en();
            break;
          }
          
          if ((millis() - system_fsm.timer > nfc.conf_params.time_for_write_card) || result != NO_CARD) 
          {
            if(result == SUCCESS)
            {
              lcd_en_print(F("Scriere incheiata"), F("cu succes !     "), FEEDBACK_PRIORITY);
              as_buzzer_command_warning();
            }
            else if (result != NO_CARD)
            {
              lcd_en_print(F("Eroare la       "), F("scriere...      "), FEEDBACK_PRIORITY);
              as_buzzer_command_error();
            }
            else
            {
              lcd_en_print(F("Niciun card     "), F("detectat...     "), FEEDBACK_PRIORITY);
              as_buzzer_command_error();
              Serial.print(F("{\"ev\":\"wr\",\"stat\":\"timeout\"}"));
            }
            system_fsm.write_state = WRITE_WAIT_CMD;
          } 
        break;

        case WRITE_ERROR:

        break;
      }
    break;  
 }
}
