// Tasks
// J Losh

// Sean Slater

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "tasks.h"

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTE,0 // off-board red LED
#define ORANGE_LED PORTA,2 // off-board orange LED
#define YELLOW_LED PORTA,3 // off-board yellow LED
#define GREEN_LED  PORTA,4 // off-board green LED

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw(void)
{
    // enable GPIO ports
   enablePort(PORTA);      // ports A, B, E, and F are utilized
   enablePort(PORTB);
   enablePort(PORTE);
   enablePort(PORTF);

   // set GPIO inputs
   selectPinDigitalInput(BUTTON_0);    // 0-5 on PORTB are set as inputs
   selectPinDigitalInput(BUTTON_1);
   selectPinDigitalInput(BUTTON_2);
   selectPinDigitalInput(BUTTON_3);
   selectPinDigitalInput(BUTTON_4);
   selectPinDigitalInput(BUTTON_5);
   enablePinPullup(BUTTON_0);          // 0-5 on PORTB have pull-up resistors enabled
   enablePinPullup(BUTTON_1);
   enablePinPullup(BUTTON_2);
   enablePinPullup(BUTTON_3);
   enablePinPullup(BUTTON_4);
   enablePinPullup(BUTTON_5);

   // set GPIO outputs
   selectPinPushPullOutput(GREEN_LED); // pin --> LED --> resistor --> GND
   selectPinPushPullOutput(YELLOW_LED);
   selectPinPushPullOutput(ORANGE_LED);
   selectPinPushPullOutput(RED_LED);   // set as 20mA drive strength outputs
   selectPinPushPullOutput(BLUE_LED);

   // Power-up flash
   setPinValue(GREEN_LED, 1);  // blink green LED to signal GPIO init successful
   waitMicrosecond(250000);
   setPinValue(GREEN_LED, 0);
   waitMicrosecond(250000);
}

uint8_t readPbs(void)
{
    return ~(getPortValue(PORTB)) & 0x3F;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            killThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            // original
            *p = 0;

            // if you want to test the ability for requestData to catch calls outside of curren task's memory
            //  comment out original line above
            requestData(0, (p_data*)p);
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}
