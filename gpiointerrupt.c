/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>

/* Driver configuration */
#include "ti_drivers_config.h"

typedef struct task {
    int state;
    unsigned long tick;    //Tick rate = 500ms
    unsigned long tickCTR; //Tick counter
    int (*TickFct)(int);   //Function to call for task's tick
} task;

task tasks[3];

const unsigned char tasksNum = 3; //Number of tasks
const unsigned long periodGCD = 500000; //500ms
const unsigned long periodWAIT = 3500000; //Time to complete WAIT
const unsigned long periodSOS = 13500000; //Time to complete SOS
const unsigned long periodOK = 11500000; //Time to complete OK

int TickFct_WAIT(int state);
enum WAIT_States { WAIT_SMStart, SM_WAIT };

int TickFct_SOS(int state);
enum SOS_States { SOS_SMStart, SM_S1, SM_O1, SM_S2 };

int TickFct_OK(int state);
enum OK_States { OK_SMStart, SM_O2, SM_K };

unsigned char toggle = 0;
unsigned long timeElapsed = 0;

/* Timer Info
 *
 * 500ms  = 500000us
 *
 * 1500ms = 1500000us
 *
 * 3500ms = 3500000us
 *
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    TickFct_WAIT(WAIT_SMStart);

    if(toggle == 0) {
        TickFct_SOS(SOS_SMStart);
        TickFct_SOS(SM_S1);
        TickFct_SOS(SM_O1);
    }
    else if (toggle == 1) {
        TickFct_OK(OK_SMStart);
        TickFct_OK(SM_O2);
        TickFct_OK(SM_K);
    }
}

/*
 *  ======== initTimer ========
 *  Function to initiate Timer
 *
 */
void initTimer(void)
{
    timeElapsed = 0;
    Timer_Handle timer0;
    Timer_Params params;

    Timer_init();
    Timer_Params_init(&params);
    //params.period = 1000000;
    params.period = 500000;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {
        /* Failed to initialize timer */
        while(1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while(1) {}
    }
}

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index) //Top Button
{
    /* Toggle an LED */
    //GPIO_toggle(CONFIG_GPIO_LED_0); //Red LED
}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index) //Bottom Button
{
    /* Sets toggle for message change */
    if (toggle == 0) {
        toggle = 1;
    }
    else {
        toggle = 0;
    }

    /* Toggle an LED */
    //GPIO_toggle(CONFIG_GPIO_LED_1); //Green LED
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    //GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    /*while (1) {
        if (toggle == 0) {
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); //Red LED
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF); //Green LED
        }
        else if (toggle == 1) {
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF); //Red LED
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON); //Green LED
        }
        else {
            break;
        }
    }*/

    initTimer();

    return (NULL);
}

int TickFct_WAIT(int state) {
  switch(state) { // Transitions
     case WAIT_SMStart: // Initial transition
        state = SM_WAIT;
        break;
     case SM_WAIT:
        if (toggle == 0) {
            state = SOS_SMStart;
        }
        else if (toggle == 1) {
            state = OK_SMStart;
        }
        else {
            state = SOS_SMStart;
        }
        break;
     default:
        state = SOS_SMStart;
   } // Transitions

  switch(state) { // State actions
     case SM_WAIT:
        usleep(periodWAIT);
        break;
     default:
        break;
  } // State actions
  return state;
}

int TickFct_SOS(int state) {
  switch(state) { // Transitions
     case SOS_SMStart: // Initial transition
        state = SM_S1;
        break;
     case SM_S1:
        state = SM_O1;
        break;
     case SM_O1:
        state = SM_S2;
        break;
     case SM_S2:
        state = WAIT_SMStart;
        break;
     default:
        state = WAIT_SMStart;
        break;
   } // Transitions

  switch(state) { // State actions
     case SM_S1:
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); //Red LED
         usleep(periodGCD); //Wait 500ms
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         usleep(periodGCD);
         usleep(periodGCD);
        break;
     case SM_O1:
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON); //Green LED
         usleep(periodGCD); //Wait 500ms
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         usleep(periodGCD);
         usleep(periodGCD);
         break;
     case SM_S2:
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); //Red LED
         usleep(periodGCD); //Wait 500ms
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
         break;
     default:
        break;
  } // State actions
  return state;
}

int TickFct_OK(int state) {
  switch(state) { // Transitions
     case OK_SMStart: // Initial transition
         state = SM_O2;
         break;
     case SM_O2:
         state = SM_K;
         break;
     case SM_K:
         state = WAIT_SMStart;
         break;
     default:
         state = WAIT_SMStart;
         break;
   } // Transitions

  switch(state) { // State actions
     case SM_O2:
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON); //Green LED
         usleep(periodGCD); //Wait 500ms
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
         usleep(periodGCD);
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         usleep(periodGCD);
         usleep(periodGCD);
         break;
      case SM_K:
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON); //Green LED
         usleep(periodGCD); //Wait 500ms
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); //Red LED
         usleep(periodGCD); //Wait 500ms
         GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON); //Green LED
         usleep(periodGCD); //Wait 500ms
         usleep(periodGCD);
         usleep(periodGCD);
         GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
         break;
      default:
         break;
  } // State actions
  return state;
}
