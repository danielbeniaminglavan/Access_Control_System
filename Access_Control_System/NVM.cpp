#include "NVM.h"
#include <string.h>


static uint16_t _nvm_get_metadata(nvm_id_t id, uint16_t* size) 
{
  switch (id) 
  {
    // blocuri complete
    case NVM_BLOCK_SYSTEM:
      *size = sizeof(system_configurable_param_t);
      return offsetof(nvm_layout_t, system);
    case NVM_BLOCK_BUZZER:
      *size = sizeof(buzzer_configurable_param_t);
      return offsetof(nvm_layout_t, buzzer);
    case NVM_BLOCK_NFC:
      *size = sizeof(nfc_configurable_param_t);
      return offsetof(nvm_layout_t, nfc);
    case NVM_BLOCK_SERVO:
      *size = sizeof(servo_configurable_param_t);
      return offsetof(nvm_layout_t, servo);

    // parametri individuali
        
    // LCD
    case NVM_PARAM_LCD_FEEDBACK:
      *size = sizeof(uint16_t);
      return offsetof(nvm_layout_t, lcd_feedback_duration);

    // system
    case NVM_PARAM_SYS_MODE:
      *size = sizeof(system_mode_t);
      return offsetof(nvm_layout_t, system.mode);
    case NVM_PARAM_REC_MODE:
      *size = sizeof(system_rec_mode_t);
      return offsetof(nvm_layout_t, system.system_rec_mode);
    case NVM_PARAM_TIME_FOR_VERIFY_CARD:
      *size = sizeof(uint16_t);
      return offsetof(nvm_layout_t, system.time_for_verify_card);
    case NVM_PARAM_TIME_FOR_VERIFY_F_ENC:
      *size = sizeof(uint16_t);
      return offsetof(nvm_layout_t, system.time_for_verify_f_enc);
    case NVM_PARAM_TIME_FOR_WAIT_CARD:
      *size = sizeof(uint32_t);
      return offsetof(nvm_layout_t, system.time_for_wait_card);
    case NVM_PARAM_TIME_FOR_CALC_FAC_ENC:
      *size = sizeof(uint32_t);
      return offsetof(nvm_layout_t, system.time_for_calc_fac_enc);

    // NFC
    case NVM_PARAM_TIME_FOR_WRITE_CARD:
      *size = sizeof(uint32_t);
      return offsetof(nvm_layout_t, nfc.time_for_write_card);
    case NVM_PARAM_DATA_ONLY_ACC:
      *size = 32; // char data_acc_only[32]
      return offsetof(nvm_layout_t, nfc.data_acc_only);

    // buzzer
    case NVM_PARAM_ERROR_TIME:
      *size = sizeof(uint16_t);
      return offsetof(nvm_layout_t, buzzer.error_time);
    case NVM_PARAM_WARNING_TIME:
      *size = sizeof(uint16_t);
      return offsetof(nvm_layout_t, buzzer.warning_time);
    case NVM_PARAM_MOVEMENT_BEEP_PERIOD:
      *size = sizeof(uint16_t);
      return offsetof(nvm_layout_t, buzzer.movement_beep_period);
    case NVM_PARAM_WARNING_BEEP_PERIOD:
      *size = sizeof(uint16_t);
      return offsetof(nvm_layout_t, buzzer.warning_beep_period);

    // servo
    case NVM_PARAM_SERVO_MOVING_TIME:
      *size = sizeof(uint8_t);
      return offsetof(nvm_layout_t, servo.moving_time);
    case NVM_PARAM_SERVO_OPEN_TIME:
      *size = sizeof(uint8_t);
      return offsetof(nvm_layout_t, servo.open_time);

    case NVM_ALL:
      *size = sizeof(nvm_layout_t);
      return 0;

    default:
      *size = 0;
      return 0;
    }
}

void nvm_save(nvm_id_t id, const void* data_ptr) 
{
  uint16_t size;
  uint16_t offset = _nvm_get_metadata(id, &size);

  if (size == 0 || data_ptr == NULL) return;

  const uint8_t* bytes = (const uint8_t*)data_ptr;

  for (uint16_t i = 0; i < size; i++) 
  {
    EEPROM.update(NVM_BASE_ADDR + offset + i, bytes[i]);
  }
}

void nvm_load(nvm_id_t id, void* data_ptr) 
{
  uint16_t size;
  uint16_t offset = _nvm_get_metadata(id, &size);

  if (size == 0 || data_ptr == NULL) return;

  uint8_t* bytes = (uint8_t*)data_ptr;

  for (uint16_t i = 0; i < size; i++) 
  {
    bytes[i] = EEPROM.read(NVM_BASE_ADDR + offset + i);
  }
}

void nvm_factory_reset_if_needed() 
{
  uint32_t magic;
  EEPROM.get(offsetof(nvm_layout_t, magic), magic);

  if (magic != NVM_MAGIC_NUMBER) 
  {
    nvm_layout_t default_cfg;

    default_cfg.magic = NVM_MAGIC_NUMBER;

    default_cfg.system.mode = SYS_ACCESS_MODE;
    default_cfg.system.system_rec_mode = REC_ONLY_ACC;
    default_cfg.system.time_for_verify_f_enc = (uint16_t)TIME_IN_ACCESS_VERIFY_CARD_PLUS_ENC_MS;
    default_cfg.system.time_for_verify_card = (uint16_t)TIME_IN_ACCESS_VERIFY_ONLY_CARD_MS;
    default_cfg.system.time_for_wait_card = (uint32_t)TIME_IN_ACCESS_WAIT_CARD_MS;
    default_cfg.system.time_for_calc_fac_enc = (uint32_t)TIME_IN_WRITE_COMPLETE_REGISTRATION_MS;

    default_cfg.buzzer.error_time = (uint16_t)BUZZER_ERROR_SIGNAL_MS;
    default_cfg.buzzer.warning_time = (uint16_t)BUZZER_WARNING_SIGNAL_MS;
    default_cfg.buzzer.movement_beep_period = (uint16_t)BUZZER_BEEP_PERIOD_MS;
    default_cfg.buzzer.warning_beep_period = (uint16_t)BUZZER_WARNING_BEEP_PERIOD_MS;

    default_cfg.nfc.time_for_write_card = (uint32_t)TIME_FOR_WRITE_CARD_MS;
    memset(default_cfg.nfc.data_acc_only, 0, sizeof(default_cfg.nfc.data_acc_only));
    strncpy(default_cfg.nfc.data_acc_only, nfc_get_data(), 32);

    default_cfg.servo.moving_time = (uint8_t)SERVO_MOVING_TIME_S;
    default_cfg.servo.open_time = (uint8_t)SERVO_OPEN_TIME_S;

    default_cfg.lcd_feedback_duration = (uint16_t)FEEDBACK_DURATION_MS;

    EEPROM.put(NVM_BASE_ADDR, default_cfg);
        
    Serial.println(F("{\"event\":\"nvm_factory_reset_done\"}"));
  }
}