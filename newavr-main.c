#define F_CPU 3333333
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>




volatile uint8_t count = 0;
volatile uint8_t timer_state = 0;       // 0 if timer is off, 1 otherwise.
volatile uint8_t timer_done = 0;


//-------- Initialization -----------

/**
 * Handles Initialization of LEDs
 */
void init_leds() {
    PORTD.DIRSET |= PIN5_bm | PIN7_bm; // R0 R1
    PORTA.DIRSET |= PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm; // Y2 Y3 G4 G5
}

void init_buttons() {
    // initializing the increment button
    PORTC.DIRCLR = PIN0_bm;
    PORTC.PIN0CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
    
    
    // up5 button initialization
    PORTC.DIRCLR = PIN1_bm;
    PORTC.PIN1CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
    
    
    // Start/Cancel button initialization
    PORTD.DIRCLR = PIN6_bm;
    PORTD.PIN6CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
}

/**
 * sets up the timer 
 */
void init_timer() {
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    
    // normal mode 
    TCA0.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_NORMAL_gc;
    
    
    TCA0.SINGLE.EVCTRL &= ~(TCA_SINGLE_CNTEI_bm);
    
    // period val 52083 1000ms
    TCA0.SINGLE.PER = 52083;
    
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
}


void init_speaker() {
    PORTD.DIRSET = PIN1_bm;
    PORTD.PIN1CTRL |= PORT_PULLUPEN_bm;
   
}

///-- Interrupt Handlers 

ISR(PORTC_PORT_vect) {
    // increment button caused interrupt 
    if (PORTC.INTFLAGS & PIN0_bm) {
        if(timer_state == 0) {
            if ((count + 1) <= 63) {
                count++;
            }
        }
        
        // clear interrupt 
        PORTC.INTFLAGS &= PIN0_bm;
    }
    
    // up5 button caused interrupt 
    if (PORTC.INTFLAGS & PIN1_bm) {
        if(timer_state == 0) {
            if ((count + 5) <= 63) {
                count += 5;
            } else {
                count = 63;
            }
        }
        
        // clear interrupt 
        PORTC.INTFLAGS &= PIN1_bm;
    }
    
}

ISR(PORTD_PORT_vect) {
    if (PORTD.INTFLAGS & PIN6_bm) {
        timer_state = !timer_state;
        if (!timer_state) {
            count = 0;
        }
       
        // clear interrupt
        PORTD.INTFLAGS &= PIN6_bm;
    }
}


ISR(TCA0_OVF_vect) {
    if (TCA0.SINGLE.INTFLAGS & TCA_SINGLE_OVF_bm) {
        if (count > 0 && timer_state) {
            count--;
            if(count == 0){
                timer_done = 1;
                timer_state = 0;
            }
        }
        
        // clear interrupt 
        TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
    }

   
}

int main(void) {
    init_leds();
    init_buttons();
    init_timer();
    init_speaker();
    

    sei();
    uint8_t temp[6] = {PIN7_bm, PIN5_bm, PIN7_bm, PIN6_bm, PIN5_bm, PIN4_bm};

    int i;
    while (1) { 
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            _delay_ms(10);  
        }
        for (i = 0; i < 6; i++) {
            if (i == 0 || i == 1) {
                if(count & (1 << i)) {
                    PORTD.OUT |= temp[i];
                } else {
                    PORTD.OUT &= ~temp[i];
                }
            } else {
                if(count & (1 << i)) {
                    PORTA.OUT |= temp[i];
                } else {
                    PORTA.OUT &= ~temp[i];
                }
            }
        }
        
        if (timer_done) {   // alerting user timer is done
            for (int i = 0; i < 250; i++) {
                PORTD.OUTTGL = PIN1_bm;
                _delay_ms(2);
                PORTD.OUTTGL = PIN1_bm;
                _delay_ms(2);
            }
            
            timer_done = 0;
        }
       
    }
}
