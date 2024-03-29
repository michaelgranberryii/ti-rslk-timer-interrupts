/**
 * @file Bumper_Sensors.c
 * @brief Source code for the Bumper_Sensors driver.
 *
 * This file contains the function definitions for the Bumper_Sensors driver.
 * It is used to demonstrate external I/O-triggered interrupts.
 * The driver interfaces with the following:
 *  - Pololu Left Bumper Switches (https://www.pololu.com/product/3673)
 *  - Pololu Right Bumper Switches (https://www.pololu.com/product/3674)
 *
 * To verify the pinout used for the Pololu Bumper Sensors, refer to the MSP432P401R SimpleLink Microcontroller LaunchPad Development Kit User's Guide
 * Link: https://docs.rs-online.com/3934/A700000006811369.pdf
 *
 * The following pins are used when the Bumper Sensors are connected to the TI MSP432 LaunchPad:
 *  - BUMP_0                    <-->    MSP432 LaunchPad Pin P4.0
 *  - BUMP_1                    <-->    MSP432 LaunchPad Pin P4.2
 *  - BUMP_2                    <-->    MSP432 LaunchPad Pin P4.3
 *  - Right Bumper Sensor GND   <-->    MSP432 LaunchPad GND
 *  - BUMP_3                    <-->    MSP432 LaunchPad Pin P4.5
 *  - BUMP_4                    <-->    MSP432 LaunchPad Pin P4.6
 *  - BUMP_5                    <-->    MSP432 LaunchPad Pin P4.7
 *  - Left Bumper Sensor GND    <-->    MSP432 LaunchPad GND
 *
 * @note The Bumper Switches are configured with negative logic as the default setting.
 * When the switches are active, they connect to GND.
 *
 * @author Aaron Nanas
 *
 */

#include "../inc/Bumper_Sensors.h"

void Bumper_Sensors_Init(void(*task)(uint8_t))
{
    // Store the user-defined task function for use during interrupt handling
    Bumper_Task = task;

    // Configure the following pins as GPIO pins: P4.7 - P4.5, P4.3, P4.2, and P4.0
    P4->SEL0 &= ~0xED;
    P4->SEL1 &= ~0xED;

    // Set the following pins as input: P4.7 - P4.5, P4.3, P4.2, and P4.0
    P4->DIR &= ~0xED;

    // Enable pull-up resistors on the following pins: P4.7 - P4.5, P4.3, P4.2, and P4.0
    P4->REN |= 0xED;

    // Ensure that the pins are pulled up: P4.7 - P4.5, P4.3, P4.2, and P4.0
    P4->OUT |= 0xED;

    // Interrupt Edge Select: High-to-Low Transition
    // Configure the pins to use falling edge event triggers: P4.7 - P4.5, P4.3, P4.2, and P4.0
    P4->IES |= 0xED;

    // Clear any existing interrupt flags
    P4->IFG &= ~0xED;

    // Enable interrupts on the following pins: P4.7 - P4.5, P4.3, P4.2, and P4.0
    P4->IE |= 0xED;

    // Set the priority of the interrupts (IRQ 38) to 0 (section 2.4.3.20)
    NVIC->IP[9] = (NVIC->IP[9] & 0xFF0FFFFF);

    // Enable Interrupt 38 in NVIC (section 2.4.3.2)
    // Bit 6 corresponds to IRQ 38
    NVIC->ISER[1] = 0x00000040;
}

uint8_t Bumper_Read(void)
{
    // Declare a local variable to store the input register value
    // Then, read the input register P4->IN and invert its value using the bitwise NOT operator (~).
    // This is done to account for the negative logic behavior of the bumper switches
    uint32_t bumper_state = ~P4->IN;

    // Use bitwise operations to extract the relevant bits representing the switch states.
    // - ((bumper_state & 0xE0) >> 2): Extract bits 7, 6, and 5, and right-shift them by 2 to align them to bits 5, 4, and 3.
    // - ((bumper_state & 0x0C) >> 1): Extract bits 3 and 2, and right-shift them by 1 to align them to bits 2 and 1.
    // - (bumper_state & 0x01): Extract bit 0.
    // Then, combine the extracted bits using the bitwise OR operator and return the bumper state
    return (((bumper_state & 0xE0) >> 2) | ((bumper_state & 0x0C) >> 1) | (bumper_state & 0x01));
}

/**
 * @brief Interrupt handler for PORT4 (P4) events.
 *
 * This function is an interrupt service routine (ISR) for PORT4 (P4) of the TI MSP432 LaunchPad.
 * It is triggered on a falling edge event on any of the switches connected to P4 (BUMP_0 to BUMP_5).
 * The function clears all interrupt flags for PORT4 and then executes the user-defined task function (Bump_Task)
 * by passing the current state of the switches, which is obtained by calling Bump_Read().
 *
 * @note This function does not handle critical section/race conditions.
 *
 * @return None
 */
void PORT4_IRQHandler(void)
{
    // Clear the interrupt flags for P4.7 - P4.5, P4.3, P4.2, and P4.0
    P4->IFG &= ~0xED;

    uint8_t bumper_sensor_val = Bumper_Read();
    // Execute the user-defined task
    (*Bumper_Task)(bumper_sensor_val);
}
