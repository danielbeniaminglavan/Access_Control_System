#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "buzzer.h"
#include "actuator.h"
#include "button.h"

void init_timer()
{
  TCCR1A = 0b00100010; // Timerul 1 in mod Fast PWM, OC1A si OC1B functioneaza in modul neinversat
  TCCR1B = 0b00011010; // prescaler = 8
  ICR1 = 20000-1; // valoarea TOP
  OCR1B = 5000;      //limita pana la care semnalul PWM generat pe OC1B sta in HIGH
  TCNT1 = 0;      //timerul porneste mereu de la 0 la initializare
  setbit(TIFR1,ICF1);  // flagul care indica ciclarea este resetat la initializare

  TIMSK1 |= (1 << ICIE1);  // se activeaza intreruperea corespunzatoare setarii flagului ICF

  sei();  // se activeaza intreruperile globale
}

// Rutina de tratare a intreruperii corespunzatoare setarii ICF
// In cadrul acestei rutine(executate la 10 ms, se updateaza starea servomotorului,a buzzerului si a butonului START/STOP) 
ISR(TIMER1_CAPT_vect)
{
  as_servo_update();
  as_buzzer_update();
  as_ss_btn_update();
}