#ifndef COMMS_H
#define COMMS_H

#include <Arduino.h>
#include "PIR.h"
#include "LCD.h"
#include "PN532.h"
#include "buzzer.h"
#include "LED.h"
#include "actuator.h"
#include "button.h"


/* Tresholduri pentru anumite stari */
#define TIME_IN_ACCESS_VERIFY_CARD_PLUS_ENC_MS 15000 
#define TIME_IN_ACCESS_VERIFY_ONLY_CARD_MS 500 
#define TIME_IN_ACCESS_WAIT_CARD_MS 30000
#define TIME_IN_WRITE_COMPLETE_REGISTRATION_MS 35000
#define TIME_FOR_WAIT_INIT_MESSAGE 1000
#define CHECK_FOR_MASTER_AVAILABILITY 1000
#define TIME_FOR_WAIT_MASTER_AV_RESPONSE 3000

// modurile de operare principale ale sistemului
typedef enum 
{
  SYS_ACCESS_MODE = 0,     // control acces activ
  SYS_WRITE_MODE           // mod de scriere carduri
} system_mode_t;

// mijloace de recunoastere
typedef enum 
{
  REC_ONLY_WITH_CARD = 0,               // Accesul se permite doar angajatilor, in baza cardului
  REC_CARD_PLUS_GUESTS,                 // Accesul se permite angajatilor si vizitatorilor, pe baza cardului
  REC_CARD_PLUS_FAC_ENC,                // Accesul se permite doar angajatilor, in baza cardului si a amprentei faciale
  REC_CARD_PLUS_FAC_ENC_PLUS_GUESTS,    // Accesul se permite angajatilor, in baza cardului si a amprentei faciale si vizitatorilor in baza cardului
  REC_FAC_ENC,                          // Accesul se permite doar angajatilor, in baza amprentei faciale
  REC_FAC_ENC_PLUS_GUESTS,              // Accesul se permite angajatilor in baza amprentei faciale si vizitatorilor in baza cardului
  REC_ONLY_ACC                          // Accesul se permite doar pe baza cardului, fara pontaj
} system_rec_mode_t;

// substari corespunzatoare modului de operare SYS_ACCESS_MODE
typedef enum 
{
  ACCESS_WAIT_PERSON = 0,  // substare in care se asteapta detectarea unei persoane
  ACCESS_WAIT_CARD,    // substare in care se asteapta scanarea unui card
  ACCESS_VERIFY,       // substare in care se asteapta verificarea amprentei faciale
  ACCESS_ERROR         // substarea care marcheaza o eroare
} system_access_state_t;

// substari corespunzatoare modului de operare SYS_WRITE_MODE
typedef enum 
{
  WRITE_WAIT_CMD,                 // substare in care se asteapta comenzi pentru scriere
  WRITE_COMPLETE_REGISTRATION,    // substare in care se asteapta comanda pt scriere dupa capturarea encodingului 
  WRITE_WAIT_CARD,                // substare in care se asteapta apropierea cardului pentru scriere
  WRITE_ERROR                     // substarea care marcheaza o eroare
} system_write_state_t;

// tipuri de comenzi de scriere
typedef enum 
{
  WRITE_CARD_FULL,                 
  WRITE_CARD_ONLY_ACC,    
  WRITE_CARD_ONLY_ID,                
  DEL_CARD_FULL,        
  DEL_CARD_ONLY_ACC,
  DEL_CARD_ONLY_ID            
} system_write_cmd_t;

typedef struct
{
  volatile system_mode_t mode;                  // modul actual de lucru
  system_rec_mode_t system_rec_mode;            // modul in care se face recunoasterea pentru acordarea accesului
  uint16_t time_for_verify_f_enc;               // timpul maxim pentru verificarea amprentei faciale
  uint16_t time_for_verify_card;                // timpul maxim pentru verificarea cardului in modurile de recunoastere CARD sau CARD + VIZITATORI
  uint32_t time_for_wait_card;                  // timpul maxim pentru asteptarea scanarii cardului dupa detectarea prezentei
  uint32_t time_for_calc_fac_enc;               // timpul maxim pentru calcularea amprentei faciale
}system_configurable_param_t;

// structura care retine datele necesare pentru gestionarea modurilor de lucru ale sistemului
typedef struct 
{
  system_configurable_param_t conf_params;      // parametrii configurabili ai sistemului
  system_mode_t last_mode;                      // modul precedent de lucru(util pentru detectarea tranzitiilor si transmiterea mesajelor la aparitia acestora)
  system_access_state_t access_state;           // sub-starea curenta(utila in cazul in care modul de lucru actual este SYS_ACCESS_MODE)
  system_write_state_t write_state;             // sub-starea curenta(utila in cazul in care modul de lucru actual este SYS_WRITE_MODE)
  system_write_cmd_t write_cmd;                 // util pentru a sti care blocuri trebuie sa fie scrise pe card in sub-starea WRITE_WAIT_CARD 
  String payload;                               // util pentru a sti ce trebuie scris pe card
  unsigned long timer;                          // timer(util pentru gestionarea timeout-urilor)
  unsigned long timer_sync;                     // timer(util pentru detectarea erorilor)
  bool person_detected;                         // flag pentru transmiterea mesajelor cu privire la starea senzorului PIR
  String message_available;                     // mesaj disponibil pe seriala
  unsigned long threshold_for_rec_mess;         // threshold pentru primirea unui raspuns
  system_rec_mode_t change_rec_mode_request;    // cerere de schimbare a modului in care se efectueaza recunoasterea
  uint16_t change_time_for_verify_f_enc;        // cerere de schimvare pentru timpul maxim pentru verificarea amprentei faciale
  uint16_t change_time_for_verify_card;         // cerere de schimvare pentru timpul maxim pentru verificarea cardului in modurile de recunoastere CARD sau CARD + VIZITATORI
  uint32_t change_time_for_wait_card;           // cerere de schimvare pentru timpul maxim pentru asteptarea scanarii cardului dupa detectarea prezentei
  uint32_t change_time_for_calc_fac_enc;        // cerere de schimvare pentru timpul maxim pentru calcularea amprentei faciale
  bool master_defekt;                           // starea Raspberry Pi
  uint32_t time_in_access_wait_card;            // timpul petrcut in aceasta stare in care se poate efectua si verificarea daca modul de recunoastere o impune
} system_fsm_t;


// variabila globala care retine datele necesare pentru gestionarea modurilor de lucru ale sistemului se poate folosi si in alte fisiere
extern system_fsm_t system_fsm;

// functii pentru initializarea si oprirea sistemului
void comms_init();
void comms_update();
void extract_json_value(String& msg, const String& key, String& target);
//void comm_system_calibrate_parameters(system_fsm_t* system_fsm, uint16_t* time_for_verify_f_enc, uint16_t* time_for_verify_card, uint32_t* time_for_wait_card, uint32_t* time_for_calc_fac_enc);
//void comms_shutdown();

#endif // COMMS_H
