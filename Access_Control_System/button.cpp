#include "button.h"
#include "LED.h"


// variabila globala pentru butonul START/STOP
button_t start_stop_btn;


// functie pentru initializarea unui buton
void button_init(button_t* btn, volatile uint8_t* ddr_reg, volatile uint8_t* pin_reg, uint8_t pin) {
    btn->ddr_reg = ddr_reg;
    btn->pin_reg = pin_reg;
    btn->pin = pin;
    btn->state_now = 0;
    btn->state_prev = 0;
    // Se configureaza pinul ca intrare
    clrbit(*(btn->ddr_reg), btn->pin);

}

// functie care actualizeaza starea butonului START/STOP
void ss_btn_update(button_t* btn) {
  // se citeste starea actuala a pinului: 1 daca este apasat, 0 daca nu este apasat
  btn->state_now = (*(btn->pin_reg) & (1 << btn->pin)) ? 1 : 0;

  // se detecteaza trecerea de la neapasat la apasat
  if (btn->state_prev == 0 && btn->state_now == 1) {
    // se iau decizii diferite in functie de modul curent de operare al sistemului
    switch (system_fsm.conf_params.mode) 
    {
      case SYS_WRITE_MODE:
        system_fsm.conf_params.mode = SYS_ACCESS_MODE;
        signal_nothing();
        system_fsm.access_state = ACCESS_WAIT_PERSON;
        break;

      case SYS_ACCESS_MODE:
        system_fsm.conf_params.mode = SYS_WRITE_MODE;
        signal_nothing();
        system_fsm.person_detected = false;
        break;
    }
  }
  
  btn->state_prev = btn->state_now;
}
