#include "PN532.h"
#include <string.h>
#include "NVM.h"
#include "comms.h"

// Instanta Adafruit_PN532 folosind conexiunea SPI
Adafruit_PN532 adafruit_instance(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Se declara o variabila de tip nfc_t,care va fi facuta disponibila si in celelalte fisiere
nfc_t nfc;

// Cheia pentru acces la blocul tinta de pe card
static uint8_t nfc_key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Valoarea scrisa pe cardurile cu care se permite accesul fara pontaj
const char* data_acc_only = "24062002476C6176616E44616E69656C";

// Valoarea scrisa in blocurile care trebuie sterse
const char* data_del = "00000000000000000000000000000000";

// getter care returneaza un pointer la cheie
const uint8_t* nfc_get_key()
{
  return nfc_key;
}

// getter care returneaza un pointer la datele care se scriu pentru control accesului fara pontaj
const char* nfc_get_data()
{
  return data_acc_only;
}

// getter care returneaza un pointer la datele care se scriu in blocurile care se sterg
const char* nfc_get_data_del()
{
  return data_del;
}

// Functie pentru resetarea modulului nfc
void reset_nfc(nfc_t* nfc)
{
  setbit(*(nfc->ddr_reg), nfc->pin);
  clrbit(*(nfc->port_reg), nfc->pin);
  delay(50);
  setbit(*(nfc->port_reg), nfc->pin);
  delay(200);

  nfc->adafruit_instance->begin();
  nfc->adafruit_instance->SAMConfig();
}

// Functie de initializare a modulului nfc
void init_nfc(nfc_t* nfc_module, uint8_t* key, volatile uint8_t* ddr_reg, volatile uint8_t* port_reg, uint8_t pin, uint8_t* data_ver_only_card)
{
    nfc_module->adafruit_instance = &adafruit_instance;
    nfc_module->key_a = key;
    nfc_module->ddr_reg = ddr_reg;
    nfc_module->port_reg = port_reg;
    nfc_module->pin = pin;
    nfc_module->card_already_processed = false;
    nfc_module->data_ver_only_card = data_ver_only_card;

    bool nfc_handshake_success = false;

    if (system_fsm.master_defekt == false) 
    {
      unsigned long start_wait = millis();
      Serial.println(F("{\"ev\":\"req_nfc_p\"}"));

      while (millis() - start_wait < TIME_FOR_WAIT_INIT_MESSAGE) 
      {
        if (Serial.available()) 
        {
          String response = Serial.readStringUntil('\n');
          response.trim();

          if (response.indexOf(F("\"e\":\"nc\"")) != -1) 
          {
            String val;
            extract_json_value(response, "wt", val);
            nfc_module->conf_params.time_for_write_card = (uint32_t)val.toInt();

            extract_json_value(response, "s", val);
            memset(nfc_module->conf_params.data_acc_only, 0, sizeof(nfc_module->conf_params.data_acc_only));
            memcpy(nfc_module->conf_params.data_acc_only, val.c_str(), 32);
            nfc_module->conf_params.data_acc_only[32] = '\0';

            nvm_save_block_nfc(&nfc_module->conf_params);
                    
            nfc_handshake_success = true;
            Serial.println(F("{\"ev\":\"nfc_hd_ok\"}"));
            break;
          }
        }
      }
    }

    if (!nfc_handshake_success) 
    {
      nvm_load_block_nfc(&nfc_module->conf_params);
      //Serial.println(F("{\"ev\":\"nfc_load_from_nvm\"}"));
    }

    nfc_module->change_time_for_write_card = nfc_module->conf_params.time_for_write_card;
    memcpy(nfc_module->change_data_acc_only, nfc_module->conf_params.data_acc_only, 32);
    nfc_module->change_data_acc_only[32] = '\0'; 
    
    /*
    Serial.print(F("{\"wr_t\":"));
    Serial.print(nfc_module->conf_params.time_for_write_card);
    Serial.print(F(",\"sig\":\""));
    Serial.print(nfc_module->conf_params.data_acc_only);
    Serial.println(F("\"}"));
    */

    nfc_module->adafruit_instance->begin();

    uint32_t version = 0;

    for (int i = 0; i < 5; i++) 
    {
        version = nfc_module->adafruit_instance->getFirmwareVersion();
        if (version) break;
        delay(200);                   
    }

    if (!version) 
    {
      lcd_en_print(F("PN532 nu        "), F("functioneaza!   "), FEEDBACK_PRIORITY);

      Serial.println("{\"ev\":\"pn532_isn't_working\"}");
      while(1); 
    }
}


// Functie care converteste UID-ul cardului intr-un string hexazecimal pentru afisare
String uid_to_hex(uint8_t* uid, uint8_t uidLen) 
{
  String s = "";
  for (uint8_t i = 0; i < uidLen; i++) 
  {
    if (uid[i] < 0x10) 
    {
      s += '0'; // se adauga 0 pentru valori mai mici de 0x10
    }  
    s += String(uid[i], HEX);
  }
  s.toUpperCase();  // se realizeaza conversia la majuscule
  return s;
}

// Functie care transforma un cracter in hezacimal in reprezentarea sa binara (jumatate de byte)
uint8_t hex_char_to_nibble(char c) 
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;

  return 0; 
}
  /*
   Functia care pregateste un bloc de 16 octeti pentru scriere pe card
   Daca textul este mai scurt, se completeaza cu 0x00
   Se primesc octetii care trebuie scrisi sub forma unui sir de carctere hexazecimale 
  */
void prepare_block_payload(const char* in, uint8_t out_block[16]) {
  uint8_t len = strlen(in);
  uint8_t bytes_to_write = len / 2;  
  if (bytes_to_write > 16) bytes_to_write = 16;

  for (uint8_t i = 0; i < bytes_to_write; ++i) 
  {
    uint8_t high = hex_char_to_nibble(in[i * 2]);
    uint8_t low  = hex_char_to_nibble(in[i * 2 + 1]);
    out_block[i] = (high << 4) | low;
  }
  // Se completeaza cu 0x00 restul blocului daca este cazul
  for (uint8_t i = bytes_to_write; i < 16; ++i)
  {
    out_block[i] = 0x00;
  }
}

// Functie pentru scrierea cardurilor
write_card_return_t write_card(nfc_t* nfc, const char** payloads, uint8_t* blocks, uint8_t num_blocks) 
{
  uint8_t uid[7];
  uint8_t uid_len;

  if (!nfc->adafruit_instance->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_len, 100)) 
  { 
    return NO_CARD;
  }

  for (uint8_t i = 0; i < num_blocks; i++) 
  {
    uint8_t current_block = blocks[i];
    const char* current_payload = payloads[i];

    if (current_block == 0 || (current_block % 4 == 3)) 
    {
      Serial.print(F("{\"ev\":\"wr\",\"stat\":\"frbbk\"}"));
      Serial.print(current_block);
      Serial.println(F("}"));
      return INVALID_BLOCK;
    }

    // Se realizeaza autentificarea pentru blocul tinta folosind cheia A
    bool authenticated = false;

    for (uint8_t retry = 0; retry < AUTH_RETRIES; retry++) 
    {
      if (nfc->adafruit_instance->mifareclassic_AuthenticateBlock(uid, uid_len, current_block, 0, nfc->key_a)) 
      {
        authenticated = true;
        break; 
      }
      delay(10); 
    }

    if (!authenticated) 
    {
      Serial.print(F("{\"ev\":\"wr\",\"stat\":\"authf\"}"));
      return AUTH_FAILED;
    }
  
    // Se pregatesc datele pentru scriere
    uint8_t data_block[16];
    prepare_block_payload(current_payload, data_block);

    // Se realizeaza scrierea efectiva pe card
    if (!(nfc->adafruit_instance->mifareclassic_WriteDataBlock(current_block, data_block))) 
    {
      Serial.print(F("{\"ev\":\"wr\",\"stat\":\"wrerr\"}"));
      return WRITE_FAILED;
    }
  }

  Serial.print(F("{\"ev\":\"wr\",\"stat\":\"ok\"}"));

  return SUCCESS;
}

// Functie pentru citirea unui bloc de pe card
bool read_card(nfc_t* nfc, uint8_t* blocks, uint8_t num_blocks) 
{
  uint8_t uid[7];
  uint8_t uid_len;

  if (!(nfc->adafruit_instance->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_len, 80))) 
  {
    nfc->card_already_processed = false;
    nfc->last_uid_len = 0;
    return false;
  }

  if (nfc->card_already_processed && (uid_len == nfc->last_uid_len) && (memcmp(uid, nfc->last_uid, uid_len) == 0)) 
  {
    return false;
  }

  String data = "";

  for( uint8_t i = 0; i < num_blocks; i++)
  {
    uint8_t current_block = blocks[i];
    uint8_t data_block[16];

    // Se realizeaza autentificarea pentru blocul tinta folosind cheia A
    bool authenticated = false;

    for (uint8_t retry = 0; retry < AUTH_RETRIES; retry++) 
    {
      if (nfc->adafruit_instance->mifareclassic_AuthenticateBlock(uid, uid_len, current_block, 0, nfc->key_a)) 
      {
        authenticated = true;
        break; 
      }
      delay(5); 
    }

    if (!authenticated) 
    {
      Serial.println(F("{\"ev\":\"read\",\"stat\":\"auth_f\"}"));
      return false;
    }

    // Se realizeaza citirea efectiva a blocului tinta
    if (!(nfc->adafruit_instance->mifareclassic_ReadDataBlock(current_block, data_block))) 
    {
      Serial.println(F("{\"ev\":\"read\",\"stat\":\"read_f\"}"));
      return false;
    }
    
    if (i > 0) data += ",";
    data += "\"";
    data += current_block;
    data += "\":\"";

    for (int j = 0; j < 16; ++j) 
    {
      if (data_block[j] < 0x10) data += '0';
      data += String(data_block[j], HEX);
    }
    data += "\"";

    if(current_block == DATA_ONLY_ACC_BLOCK_GLOBAL)
    {
      memcpy(nfc->last_data_read, data_block, 16);
    }
  }

  data.toUpperCase();
  
  // Se trimit statusul complet cu UID si continut in hexazecimal
  Serial.print(F("{\"ev\":\"read\",\"stat\":\"ok\",\"uid\":\""));
  Serial.print(uid_to_hex(uid, uid_len));
  Serial.print(F("\",\"data\":{"));
  Serial.print(data); 
  Serial.println(F("}}"));

  nfc->card_already_processed = true;
  nfc->last_uid_len = uid_len;
  memcpy(nfc->last_uid, uid, uid_len);

  return true;
}
