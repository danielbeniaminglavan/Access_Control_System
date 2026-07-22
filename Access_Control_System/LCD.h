#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <LiquidCrystal.h> 

// Se definesc prioritati pentru mesaje in asa fel incat un mesaj mai prioritar sa nu poata fi suprascris o anumita perioada de timp
#define FEEDBACK_PRIORITY      0
#define INSTRUCTION_PRIORITY   1

// Durata pe parcursul careia un mesaj prioritar nu poate fi suprascris
#define FEEDBACK_DURATION_MS   2000

// Prioritatea cea mai "slaba",folosita la initializare
#define MAX_UNUSED_PRIORITY    255

// Pinii folositi pentru conectarea LCD-ului
#define RS A0
#define E  12
#define D4 11
#define D5 9
#define D6 8
#define D7 6

#define lcd_en_init() lcd_init(&lcd_en, &lc_instance)
#define lcd_en_print(msg_line_1, msg_line_2, priority) lcd_print(&lcd_en, msg_line_1, msg_line_2, priority)
#define refresh_lcd_en() refresh_lcd(&lcd_en)
#define lcd_en_safe_shut_down() lcd_safe_shut_down(&lcd_en)


// Structura aferenta gestionarii starii unui LCD
typedef struct 
{
  uint16_t feedback_duration;                             // perioada in care nu poate fi suprascris un mesaj prioritar
  uint16_t change_feedback_duration;
  LiquidCrystal* liquid_crystal_instance;                 // instanta de Liquid Crystal necesara pentru comunicarea cu LCD-ul
  uint8_t current_priority;                               // prioritatea ultimului mesaj afisat
  uint8_t last_request_priority;                          // prioritatea ultimului request pentru afisare
  unsigned long last_update;                              // momentul ultimei afisari (in ms)
  String msg_line_1;                  // mesajul care trebuie afisat pe prima linie 
  String msg_line_2;                  // mesajul care trebuie afisat pe a doua linie
  String last_requested_msg_line_1;   // mesajul care trebuie afisat pe prima linie 
  String last_requested_msg_line_2;   // mesajul care trebuie afisat pe a doua linie
} lcd;

extern LiquidCrystal lc_instance;
extern lcd lcd_en;

// Se declara functiile utile pentru lucrul cu un LCD
void lcd_init(lcd* lcd_target, LiquidCrystal* lc_instance);  // functie pentru initializarea unui lcd
void lcd_print(lcd* lcd_target, String msg_line_1, String msg_line_2, uint8_t priority); // functie pentru afisarea de caractere din Flash pe lcd
void refresh_lcd(lcd* lcd_target);            // functie care actualizeaza caracterele afisate pe LCD
//void lcd_calibrate_parameters(lcd* lcd_target, uint16_t* new_feedback_duration); // functie care actualizeaza perioada pe parcursul careia nu se pot afisa mesaje mai putin prioritare
void lcd_safe_shut_down(lcd* lcd_target); // functia care asigura oprirea lcd-ului in conditii de siguranta

#endif // LCD_H
