// Tasks
// J Losh

#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"

#include "tasks.h"

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
    return getPortValue(PORTB) & 0x3F;
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
        yield();            // allow other programs to run... cooperate!
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);         // sleep for 125ms
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);     // wait using semaphore for required num of flashes
        setPinValue(YELLOW_LED, 1);
        sleep(1000);        // sleep for 1s, can run other tasks if needed until then
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{

    waitMicrosecond(990);   // represent some lengthy operation
    yield();                // give another process a chance to run
}

void lengthyFn(void)
{
    uint16_t i;
    while(true)
    {
        lock(resource);     // lock mutex / semaphore
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);   // unlock mutex / semaphore
    }
}

void readKeys(void)         // different buttons do different tasks
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);  // wait using semaphore for key to be released
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);   // signal via semaphore that key is pressed
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq); // signal via semaphore that a flash is required
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

void debounce(void)         // debounce function using semaphores
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);   // wait for a key to signaled as pressed
        count = 10;
        while (count != 0)  // need 10*10ms of button to be pressed before we consider it debounced
        {
            sleep(10);      // sleep for 10ms
            if (readPbs() == 0)
                count--;    // decrement count, no buttons are pressed
            else
                count = 10; // reset countdown
        }
        post(keyReleased);  // signal that they key is released
    }
}

void uncooperative(void)    // function that refuses to cooperate, only understands violence.
{
    while(true)
    {
        while (readPbs() == 8)
        {                   // oops! looks like yield isn't in the loop
        }
        yield();            // will never execute, serially
    }
}

void errant(void)           // program that attempts to access memory outside where it should...
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {                   // 32 is PB5, as 2^5 = 32; I would switch this to a #define...
            *p = 0;         // attempts to write to address 0x20000000 when
        }
        yield();            // if button is held, yield is NEVER called
    }
}

void important(void)        // this function must be run on time!
{
    while(true)             // uncooperative?
    {
        lock(resource);     // locks via mutex
        setPinValue(BLUE_LED, 1);
        sleep(1000);        // sleep for 1s, then should be executed within ~1ms
        setPinValue(BLUE_LED, 0);
        unlock(resource);   // unlocks via mutex only after 1s
    }
}
