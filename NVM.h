#ifndef NVM_H
#define NVM_H

#include <Arduino.h>
#include <EEPROM.h>

// Includem headerele modulelor pentru a avea acces la definitiile structurilor cu parametri configurabili
#include "comms.h"
#include "buzzer.h"
#include "PN532.h"
#include "actuator.h"
#include "LCD.h"

#define NVM_BASE_ADDR 0
#define NVM_MAGIC_NUMBER 0xBAADF00D // Identificator pentru validare EEPROM

#define nvm_save_block_system(block_system) nvm_save(NVM_BLOCK_SYSTEM, block_system)
#define nvm_save_block_buzzer(block_buzzer) nvm_save(NVM_BLOCK_BUZZER, block_buzzer)
#define nvm_save_block_nfc(block_nfc) nvm_save(NVM_BLOCK_NFC, block_nfc)
#define nvm_save_block_servo(block_servo) nvm_save(NVM_BLOCK_SERVO, block_servo)

#define nvm_save_param_lcd_feedback(param_lcd_feedback) nvm_save(NVM_PARAM_LCD_FEEDBACK, param_lcd_feedback)
#define nvm_save_param_sys_mode(param_sys_mode) nvm_save(NVM_PARAM_SYS_MODE, param_sys_mode)
#define nvm_save_param_rec_mode(param_rec_mode) nvm_save(NVM_PARAM_REC_MODE, param_rec_mode)
#define nvm_save_param_time_for_verify_card(param_time_for_verify_card) nvm_save(NVM_PARAM_TIME_FOR_VERIFY_CARD, param_time_for_verify_card)
#define nvm_save_param_time_for_verify_f_enc(param_time_for_verify_f_enc) nvm_save(NVM_PARAM_TIME_FOR_VERIFY_F_ENC, param_time_for_verify_f_enc)
#define nvm_save_param_time_for_wait_card(param_time_for_wait_card) nvm_save(NVM_PARAM_TIME_FOR_WAIT_CARD, param_time_for_wait_card)
#define nvm_save_param_time_for_calc_fac_enc(param_time_for_calc_fac_enc) nvm_save(NVM_PARAM_TIME_FOR_CALC_FAC_ENC, param_time_for_calc_fac_enc)
#define nvm_save_param_time_for_write_card(param_time_for_write_card) nvm_save(NVM_PARAM_TIME_FOR_WRITE_CARD, param_time_for_write_card)
#define nvm_save_param_data_only_acc(param_data_only_acc) nvm_save(NVM_PARAM_DATA_ONLY_ACC, param_data_only_acc)
#define nvm_save_param_error_time(param_error_time) nvm_save(NVM_PARAM_ERROR_TIME, param_error_time)
#define nvm_save_param_warning_time(param_warning_time) nvm_save(NVM_PARAM_WARNING_TIME, param_warning_time)
#define nvm_save_param_movement_beep_period(param_movement_beep_period) nvm_save(NVM_PARAM_MOVEMENT_BEEP_PERIOD, param_movement_beep_period)
#define nvm_save_param_warning_beep_period(param_warning_beep_period) nvm_save(NVM_PARAM_WARNING_BEEP_PERIOD, param_warning_beep_period)
#define nvm_save_param_servo_moving_time(param_servo_moving_time) nvm_save(NVM_PARAM_SERVO_MOVING_TIME, param_servo_moving_time)
#define nvm_save_param_servo_open_time(param_servo_open_time) nvm_save(NVM_PARAM_SERVO_OPEN_TIME, param_servo_open_time)

#define nvm_load_block_system(block_system) nvm_load(NVM_BLOCK_SYSTEM, block_system)
#define nvm_load_block_buzzer(block_buzzer) nvm_load(NVM_BLOCK_BUZZER, block_buzzer)
#define nvm_load_block_nfc(block_nfc) nvm_load(NVM_BLOCK_NFC, block_nfc)
#define nvm_load_block_servo(block_servo) nvm_load(NVM_BLOCK_SERVO, block_servo)

#define nvm_load_param_lcd_feedback(param_lcd_feedback) nvm_load(NVM_PARAM_LCD_FEEDBACK, param_lcd_feedback)
#define nvm_load_param_sys_mode(param_sys_mode) nvm_load(NVM_PARAM_SYS_MODE, param_sys_mode)
#define nvm_load_param_rec_mode(param_rec_mode) nvm_load(NVM_PARAM_REC_MODE, param_rec_mode)
#define nvm_load_param_time_for_verify_card(param_time_for_verify_card) nvm_load(NVM_PARAM_TIME_FOR_VERIFY_CARD, param_time_for_verify_card)
#define nvm_load_param_time_for_verify_f_enc(param_time_for_verify_f_enc) nvm_load(NVM_PARAM_TIME_FOR_VERIFY_F_ENC, param_time_for_verify_f_enc)
#define nvm_load_param_time_for_wait_card(param_lcd_time_for_wait_card) nvm_load(NVM_PARAM_TIME_FOR_WAIT_CARD, param_time_for_wait_card)
#define nvm_load_param_time_for_calc_fac_enc(param_time_for_calc_fac_enc) nvm_load(NVM_PARAM_TIME_FOR_CALC_FAC_ENC, param_time_for_calc_fac_enc)
#define nvm_load_param_time_for_write_card(param_time_for_write_card) nvm_load(NVM_PARAM_TIME_FOR_WRITE_CARD, param_time_for_write_card)
#define nvm_load_param_data_only_acc(param_data_only_acc) nvm_load(NVM_PARAM_DATA_ONLY_ACC, param_data_only_acc)
#define nvm_load_param_error_time(param_error_time) nvm_load(NVM_PARAM_ERROR_TIME, param_error_time)
#define nvm_load_param_warning_time(param_warning_time) nvm_load(NVM_PARAM_WARNING_TIME, param_warning_time)
#define nvm_load_param_movement_beep_period(param_movement_beep_period) nvm_load(NVM_PARAM_MOVEMENT_BEEP_PERIOD, param_movement_beep_period)
#define nvm_load_param_warning_beep_period(param_warning_beep_period) nvm_load(NVM_PARAM_WARNING_BEEP_PERIOD, param_warning_beep_period)
#define nvm_load_param_servo_moving_time(param_servo_moving_time) nvm_load(NVM_PARAM_SERVO_MOVING_TIME, param_servo_moving_time)
#define nvm_load_param_servo_open_time(param_servo_open_time) nvm_load(NVM_PARAM_SERVO_OPEN_TIME, param_servo_open_time)

// Identificatori pentru acces granular sau pe blocuri
typedef enum {
    // blocuri complete
    NVM_BLOCK_SYSTEM,   // system_configurable_param_t 
    NVM_BLOCK_BUZZER,   // buzzer_configurable_param_t
    NVM_BLOCK_NFC,      // nfc_configurable_param_t
    NVM_BLOCK_SERVO,    // servo_configurable_param_t
    
    // parametri individuali
    NVM_PARAM_LCD_FEEDBACK,            // lcd.feedback_duration
    NVM_PARAM_SYS_MODE,                // system_configurable_param_t.mode
    NVM_PARAM_REC_MODE,                // system_configurable_param_t.system_rec_mode
    NVM_PARAM_TIME_FOR_VERIFY_CARD,    // system_configurable_param_t.time_for_verify_card
    NVM_PARAM_TIME_FOR_VERIFY_F_ENC,   // system_configurable_param_t.time_for_verify_f_enc
    NVM_PARAM_TIME_FOR_WAIT_CARD,      // system_configurable_param_t.time_for_wait_card
    NVM_PARAM_TIME_FOR_CALC_FAC_ENC,   // system_configurable_param_t.time_for_calc_fac_enc
    NVM_PARAM_TIME_FOR_WRITE_CARD,     // nfc.time_for_write_card       
    NVM_PARAM_DATA_ONLY_ACC,           // nfc.data_acc_only
    NVM_PARAM_ERROR_TIME,              // buzzer.error_time
    NVM_PARAM_WARNING_TIME,            // buzzer.warning_time
    NVM_PARAM_MOVEMENT_BEEP_PERIOD,    // buzzer.movement_beep_period
    NVM_PARAM_WARNING_BEEP_PERIOD,     // buzzer.warning_beep_period
    NVM_PARAM_SERVO_MOVING_TIME,       // servo_configurable_param_t.moving_time
    NVM_PARAM_SERVO_OPEN_TIME,         // servo_configurable_param_t.moving_time
    
    // intregul NVM
    NVM_ALL
} nvm_id_t;

// Harta EEPROM
// Structura tipar pentru offset-uri
typedef struct {
    uint32_t magic;
    system_configurable_param_t system;
    buzzer_configurable_param_t buzzer;
    nfc_configurable_param_t nfc;
    servo_configurable_param_t servo;
    uint16_t lcd_feedback_duration;
} nvm_layout_t;

// Functii pentru salvare / incarcare valori din NVM
void nvm_save(nvm_id_t id, const void* data_ptr);
void nvm_load(nvm_id_t id, void* data_ptr);
void nvm_factory_reset_if_needed();

#endif