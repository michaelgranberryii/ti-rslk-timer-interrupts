/**
 * @file Timers_and_Interrupts_main.c
 * @brief Main source code for the Timers_and_Interrupts program.
 *
 * This file contains the main entry point for the Timers_and_Interrupts program.
 * It uses the SysTick timer to demonstrate periodic interrupts and the GPIO pins used for
 * the Bumper Sensors and the PMOD BTN module to show external I/O-triggered interrupts.
 *
 * @author Aaron Nanas, Abdullah Hendy, Michael Granberry
 *
 */

#include <stdint.h>
#include "msp.h"
#include "../inc/Clock.h"
#include "../inc/CortexM.h"
#include "../inc/GPIO.h"
#include "../inc/Bumper_Sensors.h"
#include "../inc/SysTick_Interrupt.h"
#include "../inc/PMOD_BTN_Interrupt.h"
#include "../inc/EUSCI_A0_UART.h"

// Global variable counter used in PMOD_BTN_Handler to determine the state of the PMOD 8LD module
uint8_t PMOD_BTN_counter = 0x00;

// Global variable counter to keep track of the number of SysTick interrupts
// that have occurred. it increments in SysTick_Handler on each interrupt event.
uint32_t SysTick_counter = 0;

uint32_t SysTick_counter_2s = 0;

// Global variable flag used in SysTick_Handler to initiate SysTick_counter
uint8_t SysTick_enable = 0x00;

// Global variable used to store the current state of the bumper sensors when an interrupt
// occurs (Bumper_Sensors_Handler). It will get updated on each interrupt event.
uint8_t bumper_sensor_value;


// Global variable counter to keep track of the number of SysTick interrupts
// that have occurred. it increments in SysTick_Handler on each interrupt event.
// This counter is intentionally made large and is used to debounce the bump sensor.
uint64_t dbnc_counter = 0;

/**
 * @brief SysTick interrupt handler function.
 *
 * This is the interrupt handler for the SysTick timer. It is automatically called whenever the SysTick timer reaches
 * its specified period (SYSTICK_INT_NUM_CLK_CYCLES) and generates an interrupt. The handler always increments the 'dbnc_counter' variable. The handler also
 * increments the 'SysTick_counter' and 'SysTick_counter_2s' variables when the 'SysTick_enabled' variable is set to 1 and checks if they has reached
 * the toggle rate (SYSTICK_INT_TOGGLE_RATE_MS), (SYSTICK_INT_TOGGLE_RATE_MS) respectively. When the toggle rate is reached for Systick_counter,
 * it toggles the state of LED1 (P1.0). When the toggle rate is reached for SysTick_counter_2s, it toggles the back left red LED (P8.6).
 * Then, it resets the 'SysTick_counter' variable.
 * Otherwise, 'SysTick_counter', 'SysTick_counter_2s' will be 0 and LED1 and the back left LED are also set to 0 when 'SysTick_enabled' is 0.
 *
 * @note Before using this handler, ensure that the SysTick timer has been initialized using the SysTick_Init function.
 *       The timer should be configured to generate periodic interrupts at the desired rate specified by SYSTICK_INT_NUM_CLK_CYCLES.
 *
 * @note The SysTick_Handler function should be defined and implemented separately before being used.
 *
 * @return None
 */

void SysTick_Handler(void)
{
    dbnc_counter ++;
    if (SysTick_enable == 0x01)
    {
        SysTick_counter++;
        if (SysTick_counter >= SYSTICK_INT_TOGGLE_RATE_MS)
        {
            SysTick_counter = 0;
            P1->OUT ^= 0x01;
        }

        SysTick_counter_2s++;
        if (SysTick_counter_2s >= SYSTICK_INT_2S_TOGGLE_RATE_MS)
        {
            SysTick_counter_2s = 0;
            P8->OUT ^= 0x40;
        }
    }
    else
    {
        SysTick_counter = 0;
        SysTick_counter_2s = 0;
        P1->OUT &= ~0x01;
        P8->OUT &= ~0x40;
    }
}

/**
 * @brief Bumper sensor interrupt handler function.
 *
 * This is the interrupt handler for the bumper sensor interrupts. It is called when a falling edge event is detected on
 * any of the bump sensor pins. The function toggles the state of the back right red LED (P8.7).
 * Additionally, it updates the 'bump_sensor_value' variable with the provided 'bump_sensor' parameter, representing the
 * state of the bump sensors at the time of the interrupt. This only happens when the debouncing period has passed.
 *
 * @param bumper_sensor_state An 8-bit unsigned integer representing the bump sensor states at the time of the interrupt.
 *
 * @note Before using this handler, ensure that the bump sensor interrupts have been initialized and configured
 *       appropriately using the Bump_Sensors_Init function.
 *
 * @note The Bump_Sensors_Handler function should be defined and implemented separately before being used.
 *
 * @note Potential bug in this function is if the dbnc counter rolled over and the sensor interrupt was fired when the dbnc counter is between 0-300
 *
 * @return None
 */
void Bumper_Sensors_Handler(uint8_t bumper_sensor_state)
{
    if (dbnc_counter >= 300){
        printf("Bumper Sensor State: 0x%02X\n", bumper_sensor_state);
        P8->OUT ^= 0x80;
        dbnc_counter = 0;
    }
}

/**
 * @brief PMOD BTN handler function.
 *
 * This function is the handler for the PMOD BTN interrupts. It is called when a button press event is
 * detected on the PMOD buttons. Depending on the state of the button(s) pressed, this function performs various actions
 * such as updating the counter for the PMOD 8LD module, setting the enable flag for SysTick_Handler, or toggling
 * the state of the back right red LED (P8.7).
 *
 * @param pmod_btn_state An 8-bit unsigned integer representing the state of the PMOD buttons at the time of the interrupt.
 *                       Each bit corresponds to a specific button: Bit 0 for BTN0, Bit 1 for BTN1, Bit 2 for BTN2, and Bit 3 for BTN3.
 *                       A bit value of 1 indicates the button is pressed, and 0 indicates it is released.
 *
 *  button_status      PMOD 8 LED          SysTick Enable
 *  -------------      -----------        -----------------
 *      0x1              Count Up            Unaffected
 *      0x2              Count Down          Unaffected
 *      0x4              Reset (0s)           Disabled
 *      0x8               0xAA                 Toggle
 *
 *
 * @note Before using this handler, ensure that the PMOD button interrupts have been initialized and configured
 *       appropriately using the PMOD_BTN_Interrupt_Init function.
 *
 * @note The PMOD_BTN_Handler function should be defined and implemented separately before being used.
 *
 * @return None
 */
void PMOD_BTN_Handler(uint8_t pmod_btn_state)
{
    switch(pmod_btn_state)
    {
        // PMOD BTN0 is pressed
        case 0x01:
        {
            PMOD_BTN_counter++;
            PMOD_8LD_Output(PMOD_BTN_counter);
            break;
        }

        // PMOD BTN1 is pressed
        case 0x02:
        {
            PMOD_BTN_counter--;
            PMOD_8LD_Output(PMOD_BTN_counter);
            break;
        }

        // PMOD BTN2 is pressed
        case 0x04:
        {
            SysTick_enable = 0;
            PMOD_BTN_counter = 0;
            PMOD_8LD_Output(PMOD_BTN_counter);
            break;
        }

        // PMOD BTN3 is pressed
        case 0x08:
        {
            SysTick_enable ^= 0x01;
            PMOD_BTN_counter = 0xAA;
            PMOD_8LD_Output(PMOD_BTN_counter);
            break;
        }

        default:
        {

        }
    }

    printf("PMOD BTN State: 0x%02X\n", pmod_btn_state);
    printf("PMOD BTN Counter: %d\n", PMOD_BTN_counter);
}

int main()
{
    // Initialize the 48 MHz Clock
    Clock_Init48MHz();

    // Initialize the built-in red LED and the RGB LEDs
    LED1_Init();
    LED2_Init();

    // Initialize the back left and right LEDs (P8.7 and P8.6) as output GPIO pins
    P8_Init();

    // Initialize the SysTick timer which will be used to generate periodic interrupts
    SysTick_Interrupt_Init(SYSTICK_INT_NUM_CLK_CYCLES, SYSTICK_INT_PRIORITY);

    // Initialize the bumper sensors which will be used to generate external I/O-triggered interrupts
    Bumper_Sensors_Init(&Bumper_Sensors_Handler);

    // Initialize the PMOD 8LD module
    PMOD_8LD_Init();

    // Initialize the PMOD BTN module with interrupts enabled
    PMOD_BTN_Interrupt_Init(&PMOD_BTN_Handler);

    // Initialize EUSCI_A0_UART
    EUSCI_A0_UART_Init_Printf();

    // Enable the interrupts used by the SysTick timer and the GPIO pins used by the Bumper Sensors and the PMOD BTN module
    EnableInterrupts();

    while(1)
    {
        // Toggle front yellow LEDs every second in the main thread
        P8->OUT ^= 0x01;
        P8->OUT ^= 0x20;
        Clock_Delay1ms(1000);
    }
}
