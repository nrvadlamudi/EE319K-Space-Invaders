// Main.h
// This software for Lab10 main functions
// Runs on TM4C123
// Program written by: Nicholas Vadlamudi and Ishan Kumar
// Date Created: 3/6/17 
// Last Modified: 1/14/21
// Lab number: 10
// Main Functions
// TO STUDENTS "REMOVE THIS LINE AND SPECIFY YOUR HARDWARE********
#ifndef _Main_h
#define _Main_h
#include <stdint.h>

extern volatile uint8_t SW1, SW2;
// Header files contain the prototypes for public functions 

void SysTick_Init(uint32_t period); // Systick initialization

void Init(void); // Enemy position and speed initialization

void Move(void); // Enemy movement function

void Draw(void); // Draw enemies out function
// this file explains what the module does
void CreateMissile(void); // Draw missiles when fired

#endif
