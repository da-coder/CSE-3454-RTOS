// Tasks
// J Losh

#ifndef TASKS_H_
#define TASKS_H_

#include <stdint.h>
#include <stdbool.h>

#define BLUE_LED   PORTF,2  // on/off-board blue LED
#define RED_LED    PORTE,0  // off-board red LED
#define ORANGE_LED PORTA,2  // off-board orange LED
#define YELLOW_LED PORTA,3  // off-board yellow LED
#define GREEN_LED  PORTA,4  // off-board green LED

#define BUTTON_0 PORTB,0    // button 0, no LED,    bottom left
#define BUTTON_1 PORTB,1    // button 1, green LED
#define BUTTON_2 PORTB,2    // button 2, yellow LED
#define BUTTON_3 PORTB,3    // button 3, orange LED
#define BUTTON_4 PORTB,4    // button 4, red LED
#define BUTTON_5 PORTB,5    // button 5, blue LED,  bottom right

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initHw(void);

void idle(void);
void flash4Hz(void);
void oneshot(void);
void partOfLengthyFn(void);
void lengthyFn(void);
void readKeys(void);
void debounce(void);
void uncooperative(void);
void errant(void);
void important(void);

#endif
