// Copyright (c) 2012 Wyss Institute at Harvard University
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// http://www.opensource.org/licenses/mit-license.php

// pulsetrain.h - Library for creating software Pulses for the Arduino Mega
// Set pulse widths and periods in microseconds and also specify the number of periods
// to output the pulses
// The Arduino Mega runs at 16 MHz.  The yields a clock cycle lasting 
// 6.25e-8 seconds or 62.5 nanoseconds

// Table of times for different prescalars
// Prescalar  | Resolution  |   Period    |  Max DC Time (8 bit) (16 bit)
//  8           0.5 us          32768 us                8.38 sec    2147 secs
// 64           4 us            262 ms                  67.1 sec    17302 secs

// we need a 16 bit counter to keep track of numbe rof pulses output in order
// to run stepper motors effectively as they have in excess of 2000 steps/rev
// often when microstepping

#ifndef PulseTrain_h
#define PulseTrain_h

#include <inttypes.h>

// Enums
//////////
// The 16 bit timer defines
typedef enum { PTIMER1, PTIMER3, PTIMER4, PTIMER5, NUMBER_OF_16BIT_TIMERS } timers16bit_t;
// Different states to Pulse in
enum pulse_states { PPULSE_LO, PPULSE_HI, PDC_INIT, PDC_RUNNING, POFF};

// Definitions
//////////////
#define PTRAIN_VERSION          1   // software version of this library 

#ifndef NUMBER_OF_PTRAINS
// REDEFINE this if you need more memory and aren't running as many PulseTrains
#define NUMBER_OF_PTRAINS       32  // default number of ptrains to allocate for
#endif

#ifndef PTRAINS_PER_TIMER
// REDEFINE this if you need want to add more PulseTrains to a single Timer
#define PTRAINS_PER_TIMER       12  // the maximum number of ptrains controlled by one timer 
#endif

#define DEFAULT_PTRAIN_PRESCALE 8   // default prescale value for all Timers

#define SMALL_COUNT             4

#define PTRAIN_REMOVED          251
#define ERROR_PTRAIN_REMOVED    252
#define ERROR_TIMER_RUNNING     253
#define ERROR_TIMER_COUNT       254
#define ERROR_PTRAIN_IDX        255

// MACROS
////////////////
#ifndef CLOCKCYCLESPERMICROSECOND
#define CLOCKCYCLESPERMICROSECOND ( F_CPU / 1000000L ) 
#endif

// converts microseconds to tick (assumes prescale of 8) 
#define US_TO_COUNTS(_us,_scale)        (( CLOCKCYCLESPERMICROSECOND* _us) / _scale)
// converts from counts back to microseconds
#define COUNTS_TO_US(_counts,_scale)    (( (unsigned)_counts * _scale)/ CLOCKCYCLESPERMICROSECOND )

// Custom Structs
/////////////////
typedef struct {
    uint8_t         pin;            // a pin number from 0 to 63
    uint8_t         timer_index;    // 0 to PTRAINS_PER_TIMER - 1, 
                                    // this is the index into a timer16control_t
                                        
    timers16bit_t   timer_number;   // Timer this pin is assigned to 
    uint16_t        pulse_counts;   // number of counts for pulse duration
    uint16_t        period_counts;  // number of counts for pulse off
    uint16_t        period_num_limit;   // number of periods allowed
    uint16_t        prescale;
} ptrain_t;

typedef struct {
    uint8_t     ptrain_idxs[PTRAINS_PER_TIMER];   // a ptrain index from 0 to NUMBER_OF_PTRAINS - 1
    uint8_t     number_of_ptrains;               // number of ptrains attached in use
    uint16_t    number_of_periods;              // number of periods output currently
    uint16_t    period_num_limit;               // number of periods allowed
    uint16_t    pulse_counts;                   // number of counts for pulse duration
    uint16_t    period_counts;                  // number of counts for pulse off
    uint8_t     bit_prescale;                   // the bitwise prescale
    uint8_t     pulsed_state;                   // keeps track if we are in the pulse or dwell
} timer16control_t;

// Function prototypes
///////////////////////
uint8_t pSetupTimers();
uint8_t pNewPTrain();
uint8_t pAttach(uint8_t ptrain_index, int pin, timers16bit_t timer);

uint8_t pSetPulseUS(uint8_t ptrain_index, uint32_t period, 
                        uint32_t pulse_width, uint16_t period_num_limit);
uint8_t _pSetPulseUS(uint8_t ptrain_index, uint32_t period, 
                        uint32_t pulse_width,  uint16_t period_num_limit, 
                        uint16_t prescale);
uint8_t pSetPulseOnlyUS(uint8_t ptrain_index, uint32_t pulse_width);
uint8_t pSetPeriodOnlyUS(uint8_t ptrain_index, uint32_t period);
uint8_t pSetPeriodNumberOnly(uint8_t ptrain_index, uint16_t period_num_limit);
uint16_t pGetPulseCounts(uint8_t ptrain_index);
uint16_t pGetPeriodCounts(uint8_t ptrain_index);
uint16_t pGetPeriodNumber(uint8_t ptrain_index);

uint8_t pSetTimerPrescale(timers16bit_t timer, uint8_t prescale);
uint8_t pAddToTimer(timers16bit_t timer, uint8_t ptrain_idx);
uint8_t pReloadToTimer(uint8_t ptrain_idx);
uint8_t pRemoveFromTimer(timers16bit_t timer, uint8_t ptrain_index);
uint8_t pStop(uint8_t ptrain_index);
uint8_t pStartTimer(timers16bit_t timer);
uint8_t pStopTimer(timers16bit_t timer);

// Global Data Structure Allocation
///////////////////////////////////
static ptrain_t ptrains[NUMBER_OF_PTRAINS];// static array of ptrain structures

// array of timer control data structures
static volatile timer16control_t timer_array[NUMBER_OF_16BIT_TIMERS]; 
uint8_t ptrain_count = 0;                // the total number of attached ptrains

// Interrupt Functions
////////////////////////
static void pEnableISR(timers16bit_t timer)
{
    // enable a 16 bit timer for:
    // 1. normal counting
    // 2. clear pending interrupts on that timer
    // 3. enable output compare interrupt
    // 4. clear the timer count register
    // 5. start timer by setting the prescalar
    volatile timer16control_t *timer_control = &timer_array[timer];
    switch(timer) {
        case PTIMER1:
            TCCR1A = 0x00;          // normal counting mode 
            OCR1A = SMALL_COUNT;    // set the output compare to some token count
            TIFR1 |= _BV(OCF1A);    // clear any pending interrupts
            TCCR1B |= timer_control->bit_prescale;
            TCNT1 = 0x0000;         // clear the timer count
            TIMSK1 |=  _BV(OCIE1A); // enable the output compare interrupt
            break;
        case PTIMER3:
            TCCR3A = 0x00;          // normal counting mode 
            OCR3A = SMALL_COUNT;    // set the output compare to some token count
            TIFR3 = _BV(OCF3A);     // clear any pending interrupts;
            TCCR3B &= timer_control->bit_prescale;
            TCNT3 = 0x0000;         // clear the timer count
            TIMSK3 =  _BV(OCIE3A);  // enable the output compare interrupt
            break;
        case PTIMER4:
            TCCR4A = 0x00;          // normal counting mode
            OCR4A = SMALL_COUNT;    // set the output compare to some token count
            TIFR4 = _BV(OCF4A);     // clear any pending interrupts;
            TCCR4B |= timer_control->bit_prescale;
            TCNT4 = 0x0000;         // clear the timer count
            TIMSK4 =  _BV(OCIE4A);  // enable the output compare interrupt
            break;
        case PTIMER5:
            TCCR5A = 0x00;          // normal counting mode
            OCR5A = SMALL_COUNT;    // set the output compare to some token count
            TIFR5 = _BV(OCF5A);     // clear any pending interrupts; 
            TCCR5B |= timer_control->bit_prescale;
            TCNT5 = 0x0000;         // clear the timer count
            TIMSK5 =  _BV(OCIE5A);  // enable the output compare interrupt 
            break;
        default:                    // no default
            break;
    }
}

static inline void pHandleInterrupts(   timers16bit_t timer, 
                                        volatile uint16_t *TCNTn, 
                                        volatile uint16_t* OCRnA)
{
    uint8_t out_pin;
    ptrain_t *out_ptrain;
    volatile timer16control_t *timer_control = &timer_array[timer];
    uint8_t num_ptrains = timer_control->number_of_ptrains;
    volatile uint8_t *ptrain_idxs = timer_control->ptrain_idxs;
    switch (timer_control->pulsed_state) {
        case PPULSE_LO:
            *TCNTn = 0x00;                          // clear timer
            for (int i = 0; i < num_ptrains; i++)
            {
                out_pin = ptrains[ptrain_idxs[i]].pin;
                digitalWrite( out_pin, HIGH);
            }
            timer_control->pulsed_state = PPULSE_HI;   // set status to pulsed
            timer_control->number_of_periods += 1;  // increment pulse count
            *OCRnA = timer_control->pulse_counts;
            break;
        case PPULSE_HI:
            for (int i = 0; i < num_ptrains; i++)
            {
                out_ptrain = &ptrains[ptrain_idxs[i]];
                out_pin = out_ptrain->pin;
                digitalWrite( out_pin, LOW);
            }
            timer_control->pulsed_state = PPULSE_LO;
            if (timer_control->number_of_periods >= timer_control->period_num_limit) {
                pStopTimer(timer);
            }
            *OCRnA = timer_control->period_counts;
            break;
        default:
        case PDC_INIT:
            *TCNTn = 0x00;                          // clear timer
            for (int i = 0; i < num_ptrains; i++)
            {
                out_pin = ptrains[ptrain_idxs[i]].pin;
                digitalWrite( out_pin, HIGH);
            }
            timer_control->pulsed_state = PDC_RUNNING;
            *OCRnA = timer_control->pulse_counts;
            break;
        case PDC_RUNNING:
            *TCNTn = 0x00;                          // clear timer
            timer_control->number_of_periods += 1;  // increment pulse count
            if (timer_control->number_of_periods >= timer_control->period_num_limit) {
                for (int i = 0; i < num_ptrains; i++)
                {
                     out_ptrain = &ptrains[ptrain_idxs[i]];
                     out_pin = out_ptrain->pin;
                     digitalWrite( out_pin, LOW);
                }
                pStopTimer(timer);
            }
    } // end switch
}

// Interrupt handlers for Arduino
/////////////////////////////////
SIGNAL (TIMER1_COMPA_vect) 
{ 
  pHandleInterrupts(PTIMER1, &TCNT1, &OCR1A); 
}

SIGNAL (TIMER3_COMPA_vect) 
{ 
  pHandleInterrupts(PTIMER3, &TCNT3, &OCR3A); 
}

SIGNAL (TIMER4_COMPA_vect) 
{
  pHandleInterrupts(PTIMER4, &TCNT4, &OCR4A); 
}

SIGNAL (TIMER5_COMPA_vect) 
{
  pHandleInterrupts(PTIMER5, &TCNT5, &OCR5A); 
}


static boolean pIsTimerActive(timers16bit_t timer)
{
    // returns true if this timer is on
    // mask out the top five bits to get a that prescaler registers
    // in the lower 3 bits.  When those are all zero, the timer is off
    switch (timer){
        case PTIMER1:
            return ((TCCR1B & 0x07) == 0x00) ? false : true;
        case PTIMER3:
            return ((TCCR3B & 0x07) == 0x00) ? false : true;
        case PTIMER4:
            return ((TCCR4B & 0x07) == 0x00) ? false : true;
        case PTIMER5:
            return ((TCCR5B & 0x07) == 0x00) ? false : true;
        default:
            return false;
    }
}

// Control Functions
////////////////////

uint8_t pNewPTrain(void) {
    // returns the ptrain_index
    uint8_t temp = ptrain_count;
    if( ptrain_count < NUMBER_OF_PTRAINS) {
        ptrains[ptrain_count].prescale = DEFAULT_PTRAIN_PRESCALE;
        ptrain_count++;
        return temp;
    }
    else {
        return ERROR_PTRAIN_IDX ;  // too many PTRAINS 
    }
}

void pStartPTrain(uint8_t ptrain_idx) {
        ptrain_t *ptrain_control = &ptrains[ptrain_idx];
        pStartTimer(ptrain_control->timer_number);    
}

boolean pIsValidPTrain(uint8_t test) {
    return (test < ptrain_count ) ? true: false;
}

uint8_t pAttach(uint8_t ptrain_index, int pin, timers16bit_t timer) {
    if(ptrain_index < NUMBER_OF_PTRAINS ) {
        ptrain_t *ptrain = &ptrains[ptrain_index];
        pinMode( pin, OUTPUT);                      // set ptrain pin to output
        ptrain->pin = pin;
        // initialize the timer if it has not already been initialized 
        ptrain->timer_number = timer;
        ptrain->timer_index = pAddToTimer(timer, ptrain_index);
        return ptrain_index;
    }
    else {
        return ERROR_PTRAIN_IDX ;  // too many PTRAINS 
    }
}

uint8_t _pSetPulseUS(   uint8_t ptrain_index, uint32_t period, 
                        uint32_t pulse_width, uint16_t period_num_limit, 
                        uint16_t prescale) {
    // set period and pulsewidth in microseconds
    if(ptrain_index < NUMBER_OF_PTRAINS ) {
        ptrain_t *ptrain = &ptrains[ptrain_index];
        if ( (pulse_width < period) || (period == 0) ) {
            switch (prescale) {
                case 1024:
                case 256:
                case 64:
                case 8:
                case 1:
                    ptrain->prescale = prescale;
                    break;
                default:
                    ptrain->prescale = DEFAULT_PTRAIN_PRESCALE;
            }
            ptrain->period_counts = US_TO_COUNTS(period,ptrain->prescale);
            ptrain->pulse_counts = US_TO_COUNTS(pulse_width,ptrain->prescale);
            ptrain->period_num_limit = period_num_limit;
            return 0;
        }
        else {
            return ERROR_TIMER_COUNT;
        }

    }
    else {
        return ERROR_PTRAIN_IDX ;  // too many PTRAINS  
    }
}       

uint8_t pSetPulseUS(uint8_t ptrain_index, uint32_t period, 
                    uint32_t pulse_width, uint16_t period_num_limit) {
    // for use when you don't want to set the prescaler or it's already set
    return _pSetPulseUS(ptrain_index, period, pulse_width, 
                                period_num_limit, ptrains[ptrain_index].prescale); 
}

uint8_t pSetPulseOnlyUS(uint8_t ptrain_index, uint32_t pulse_width) {
    ptrain_t *ptrain = &ptrains[ptrain_index];
    ptrain->pulse_counts = US_TO_COUNTS(pulse_width,ptrain->prescale);
    return 0;
}
uint8_t pSetPeriodOnlyUS(uint8_t ptrain_index, uint32_t period) {
    ptrain_t *ptrain = &ptrains[ptrain_index];
    ptrain->period_counts = US_TO_COUNTS(period,ptrain->prescale);
    return 0;
}
uint8_t pSetPeriodNumberOnly(uint8_t ptrain_index, uint16_t period_num_limit) {
    ptrain_t *ptrain = &ptrains[ptrain_index];
    ptrain->period_num_limit = period_num_limit;
    return 0;
    
}

uint16_t pGetPulseCounts(uint8_t ptrain_index) {
    return ptrains[ptrain_index].pulse_counts;
}

uint16_t pGetPeriodCounts(uint8_t ptrain_index) {
    return ptrains[ptrain_index].period_counts;
}

uint16_t pGetPeriodNumber(uint8_t ptrain_index) {
    return ptrains[ptrain_index].period_num_limit;
}

uint8_t pStop(uint8_t ptrain_index) {   
    // 1. remove ptrain from it's timer16control
    ptrain_t *ptrain = &ptrains[ptrain_index];
    ptrain->timer_index = pRemoveFromTimer(ptrain->timer_number, ptrain_index);
    return ptrain->timer_index;
}

uint8_t pSetTimerPrescale(timers16bit_t timer, uint8_t prescale) {
    uint8_t bit_prescale;
    volatile timer16control_t *timer_control = &timer_array[timer];
    switch(prescale) {
        case 1024:
            bit_prescale = 0x05;
            break;
        case 256:
            bit_prescale = 0x04;
            break;
        case 64:
            bit_prescale = 0x03;
            break;
        case 8:
            bit_prescale = 0x02;
            break;
        default:
            bit_prescale = 0x01;
    }
    timer_control->bit_prescale = bit_prescale;
    return 0;
}

uint8_t pAddToTimer(timers16bit_t timer, uint8_t ptrain_idx) {
    // Add a ptrain to a timer
    volatile timer16control_t *timer_control = &timer_array[timer];
    volatile uint8_t *ptrain_idxs = timer_control->ptrain_idxs;
    ptrain_idxs[timer_control->number_of_ptrains] = ptrain_idx; // slide over ptrains
    
    timer_control->number_of_ptrains++;                   //increment ptrain count
    // transfer values
    return pReloadToTimer(ptrain_idx);
}

uint8_t pReloadToTimer(uint8_t ptrain_idx) {
    // Add a ptrain to a timer
    
    ptrain_t *ptrain_control = &ptrains[ptrain_idx];
    volatile timer16control_t *timer_control = &timer_array[ptrain_control->timer_number];
    
    // transfer values
    timer_control->pulse_counts = ptrain_control->pulse_counts;
    timer_control->period_counts = ptrain_control->period_counts;
    timer_control->period_num_limit = ptrain_control->period_num_limit;
    pSetTimerPrescale(ptrain_control->timer_number, ptrain_control->prescale);
    
    return timer_control->number_of_ptrains;
}

uint8_t pRemoveFromTimer(timers16bit_t timer, uint8_t ptrain_idx) {
    // Remove a ptrain from a timer
    volatile timer16control_t *timer_control = &timer_array[timer];
    volatile uint8_t *ptrain_idxs = timer_control->ptrain_idxs;
    uint8_t timer_index = ptrains[ptrain_idx].timer_index;
    
    if (timer_control->number_of_ptrains > 0) { 
        for (int i = timer_index+1; i < timer_control->number_of_ptrains; i++)
        {
            ptrain_idxs[i-1] = ptrain_idxs[i]; // slide over ptrains
        }
        timer_control->number_of_ptrains--;  // decrement ptrain count
        return PTRAIN_REMOVED;
    }
    else {
        return ERROR_PTRAIN_REMOVED;
    }
}

void pClearTimerOfPTrains(timers16bit_t timer) {
    volatile timer16control_t *timer_control = &timer_array[timer];
    timer_control->number_of_ptrains = 0;
}

bool pIsPTrainActive(uint8_t ptrain_idx) {
    ptrain_t *ptrain_control = &ptrains[ptrain_idx];
    volatile timer16control_t *timer_control = &timer_array[ptrain_control->timer_number];
    if ((ptrain_idx == timer_control->ptrain_idxs[ptrain_control->timer_index]) && 
        (pIsTimerActive(ptrain_control->timer_number)) ) {
            return true;
        }
    else {
        return false;
    }
}

uint8_t pStartTimer(timers16bit_t timer) {
    // Start a Timer and reset its count
    volatile timer16control_t *timer_control = &timer_array[timer];
    
    // set up parameters
    if ((pIsTimerActive(timer) == false) && 
            (timer_control->number_of_ptrains > 0)) {
        // initialize timer state
        timer_control->number_of_periods = 0;       // clear the period counter
        if (timer_control->period_counts == 0) {    
            timer_control->pulsed_state = PDC_INIT; // DC opertion
        }
        else {
            timer_control->pulsed_state = PPULSE_LO; // variable
        }
        pEnableISR(timer);
        return 0;
    }
    else {
        return ERROR_TIMER_RUNNING;
    }
}

uint8_t pStopTimer(timers16bit_t timer) {
    // Timers are stopped by setting the prescaler bits to zero
    // and clearing the interrupt mask
    // This does not remove ptrains from the Timer so you can just restart
    switch (timer) {
        case PTIMER1:
            TCCR1B &= 0xF8;         // clear the prescale to stop timer
            TIMSK1 &=  ~_BV(OCIE1A);// disable the output compare interrupt
            break;
        case PTIMER3:
            TCCR3B &= 0xF8;         // clear the prescale to stop timer
            TIMSK3 &=  ~_BV(OCIE3A);// disable the output compare interrupt
            break;
        case PTIMER4:
            TCCR4B &= 0xF8;         // clear the prescale to stop timer
            TIMSK4 &=  ~_BV(OCIE4A); // disable the output compare interrupt
            break;
        case PTIMER5:
            TCCR5B &= 0xF8;         // clear the prescale to stop timer
            TIMSK5 &=  ~_BV(OCIE5A);// disable the output compare interrupt
            break;
        default:
            break;
    }
    return 0;
}

uint8_t pSetupTimers() {
    // ensure timer state is where we want it
    for (uint8_t i = 0; i < NUMBER_OF_16BIT_TIMERS; i++) {
        pStopTimer((timers16bit_t) i);
        timer_array[i].pulsed_state = POFF;
    }
    return 0;
}

#endif
