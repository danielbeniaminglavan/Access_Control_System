#ifndef PN532_H
#define PN532_H

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include "LCD.h"
#include "defs.h"

// Definirea pinilor pentru conexiunea SPI cu modulul PN532
#define PN532_SCK  2
#define PN532_MISO 5
#define PN532_MOSI 3
#define PN532_SS   4

// Definirea sectorului si blocului tinta pentru citirea/scrierea pe card
#define TARGET_SECTOR    1 // Sectorul folosit pentru scriere/citire
#define BLOCK_IN_SECTOR  0 // Blocul din sector folosit pentru scriere/citire
#define BLOCK_GLOBAL     (TARGET_SECTOR * 4 + BLOCK_IN_SECTOR)  // pozitia globala a blocului
#define DATA_ONLY_ACC_TARGET_SECTOR    10  // Sectorul folosit pentru scriere/citire
#define DATA_ONLY_ACC_BLOCK_IN_SECTOR   1  // Blocul din sector folosit pentru scriere/citire
#define DATA_ONLY_ACC_BLOCK_GLOBAL      (DATA_ONLY_ACC_TARGET_SECTOR * 4 + DATA_ONLY_ACC_BLOCK_IN_SECTOR)  // pozitia globala a blocului

#define AUTH_RETRIES 3  // de cate ori se reincearca autentificarea pe sectorul tinta de pe card

// Pinul folosit pentru resetarea modulului NFC
#define PN532_RST_PIN 2
#define PN532_RST_DDR_REGISTER DDRC
#define PN532_RST_PORT_REGISTER PORTC 

/* Timpul maxim in care se asteapta apropierea cardului pentru scriere */
#define TIME_FOR_WRITE_CARD_MS 30000

#define reset_nfc_en() reset_nfc(&nfc)
#define init_nfc_en() init_nfc(&nfc, (uint8_t*)nfc_get_key(), &PN532_RST_DDR_REGISTER, &PN532_RST_PORT_REGISTER, PN532_RST_PIN,nfc_get_data())
/*#define write_card_en(payload) write_card(&nfc, payload, BLOCK_GLOBAL)
#define write_card_only_acc_en(payload) write_card(&nfc, payload, DATA_ONLY_ACC_BLOCK_GLOBAL)*/
#define read_card_en() ({ \
    uint8_t temp_block[] = {BLOCK_GLOBAL}; \
    read_card(&nfc, temp_block, 1); \
})
#define read_card_only_acc_en() ({ \
    uint8_t temp_block[] = {DATA_ONLY_ACC_BLOCK_GLOBAL}; \
    read_card(&nfc, temp_block, 1); \
})
#define read_card_with_acc_en() ({ \
    uint8_t temp_block[] = {BLOCK_GLOBAL, DATA_ONLY_ACC_BLOCK_GLOBAL}; \
    read_card(&nfc, temp_block, 2); \
})

#define write_card_en(payload) ({ \
    const char* payloads[] = { (const char*)payload, nfc.conf_params.data_acc_only }; \
    uint8_t blocks[] = { BLOCK_GLOBAL, DATA_ONLY_ACC_BLOCK_GLOBAL }; \
    write_card(&nfc, payloads, blocks, 2); \
})

#define write_card_only_acc_en() ({ \
    const char* payloads[] = { nfc.conf_params.data_acc_only }; \
    uint8_t blocks[] = { DATA_ONLY_ACC_BLOCK_GLOBAL }; \
    write_card(&nfc, payloads, blocks, 1); \
})

#define write_card_only_id_en(payload) ({ \
    const char* payloads[] = { (const char*)payload }; \
    uint8_t blocks[] = { BLOCK_GLOBAL }; \
    write_card(&nfc, payloads, blocks, 1); \
})

#define del_card_en() ({ \
    const char* payloads[] = { nfc_get_data_del(), nfc_get_data_del() }; \
    uint8_t blocks[] = { BLOCK_GLOBAL, DATA_ONLY_ACC_BLOCK_GLOBAL }; \
    write_card(&nfc, payloads, blocks, 2); \
})

#define del_card_only_acc_en() ({ \
    const char* payloads[] = { nfc_get_data_del() }; \
    uint8_t blocks[] = { DATA_ONLY_ACC_BLOCK_GLOBAL }; \
    write_card(&nfc, payloads, blocks, 1); \
})

#define del_card_only_id_en() ({ \
    const char* payloads[] = { nfc_get_data_del() }; \
    uint8_t blocks[] = { BLOCK_GLOBAL }; \
    write_card(&nfc, payloads, blocks, 1); \
})

typedef enum {
    SUCCESS = 0,
    NO_CARD,
    AUTH_FAILED,
    WRITE_FAILED,
    INVALID_BLOCK  
} write_card_return_t;

typedef struct
{
  uint32_t time_for_write_card;        // timpul maxim in care se asteapta apropierea cardului pentru scriere
  char data_acc_only[33];           // datele scrise in blocul care controleaza doar accesul fara pontaj
}nfc_configurable_param_t;

// Structura pentru gestionarea unui dispozitiv NFC specific
typedef struct  
{
  Adafruit_PN532* adafruit_instance;   // pointer catre obiectul PN532 corespunzator
  uint8_t* key_a;                      // cheia A folosita pentru autentificare pe card
  volatile uint8_t* ddr_reg;           // registrul care controleaza directia pinilor portului la care este conectat pinul RST
  volatile uint8_t* port_reg;          // registrul care controleaza starea logica de iesire a pinilor la care este conectat RST
  uint8_t pin;                         // pinul Arduino la care este conectat RST
  bool card_already_processed;         // flag folosit pentru prevenirea scanarii multiple a unui card
  uint8_t last_uid_len;                // lungimea identificatorului ultimului card citit
  uint8_t last_uid[7];                 // identificatorul ultimului card citit
  uint8_t* data_ver_only_card;         // datele scrise in blocul block_global_ver_only_card
  uint8_t last_data_read[16];          // datele citite ultima data din blocul block_global_ver_only_card
  nfc_configurable_param_t conf_params;// parametrii configurabili pentru un modul NFC
  uint32_t change_time_for_write_card;        // timpul maxim in care se asteapta apropierea cardului pentru scriere
  char change_data_acc_only[33];           // datele scrise in blocul care controleaza doar accesul fara pontaj
}nfc_t;

extern Adafruit_PN532 adafruit_instance;
extern nfc_t nfc; 

const uint8_t* nfc_get_key();
const char* nfc_get_data();
const char* nfc_get_data_del();

// Functie pentru resetarea modulului nfc
void reset_nfc(nfc_t* nfc);

// Functie de initializare a modulului NFC
void init_nfc(nfc_t* nfc, uint8_t* key, volatile uint8_t* ddr_reg, volatile uint8_t* port_reg, uint8_t pin, uint8_t* data_ver_only_card);

// Functie pentru scrierea blocurilor de date pe card
write_card_return_t write_card(nfc_t* nfc, const char** payloads, uint8_t* blocks, uint8_t num_blocks);

// Functie pentru citirea blocurilor de date de pe card
bool read_card(nfc_t* nfc, uint8_t* blocks, uint8_t num_blocks);

uint8_t hex_char_to_nibble(char c);

#endif // PN532_H
