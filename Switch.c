// Switch.c
// This software to input from switches or buttons
// Runs on TM4C123
// Program written by: Nicholas Vadlamudi and Ishan Kumar
// Date Created: 3/6/17 
// Last Modified: 1/14/21
// Lab number: 10
// Hardware connections
// There are two switches configured on PC4 and PE4 and are implemented to interrupt when pressed

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "main.h"
// Code files contain the actual implemenation for public functions
// this file also contains an private functions and private data


void Button_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x14; //enable Port E/C Clock
	SW1 = 0;
	SW2 = 0;
	
	__asm__{
		NOP
		NOP
	}
		
	GPIO_PORTE_AMSEL_R &= ~0x10; // disable PE4 analog function
	GPIO_PORTE_PCTL_R &= ~0x000F0000; // configure PE4 as GPIO
	GPIO_PORTE_DIR_R = GPIO_PORTE_DIR_R & ~(1 << 4); // set PE4 as input
	GPIO_PORTA_AFSEL_R &= 0x10;
	GPIO_PORTE_DEN_R = GPIO_PORTE_DEN_R | 0x10; // digitally enable those pins

	GPIO_PORTE_IS_R &= ~0x10; //PE4 are edge-sensitive
	GPIO_PORTE_IBE_R &= ~0x10; // PE4 is not on both edges
	GPIO_PORTE_IEV_R |= 0x10; //rising edge trigger
	GPIO_PORTE_ICR_R |= 0x10; // clear flag 4
	GPIO_PORTE_IM_R |= 0x10; // enable PE4 interrupt
	NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFFFF00)|0x00000040; //priority 2
	
	//Repeat for Port C4
	GPIO_PORTC_AMSEL_R &= ~0x10;
	GPIO_PORTC_PCTL_R &= ~0x000F0000;
	GPIO_PORTC_DIR_R &= ~0x10;
	GPIO_PORTC_AFSEL_R &= ~0x10;
	GPIO_PORTC_DEN_R = 0x10;
	GPIO_PORTC_IS_R &= ~0x10;
	GPIO_PORTC_IBE_R &= ~0x10;
	GPIO_PORTC_IEV_R |= 0x10;
	GPIO_PORTC_ICR_R |= 0x10;
	GPIO_PORTC_IM_R |= 0x10;
	NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFF)|0x00400000; //priority 2
	
	NVIC_EN0_R = 0x00000014; // Enable interrupts 2 and 4 
	
}

void GPIOPortC_Handler(void){
	GPIO_PORTC_ICR_R = 0x10; // acknowledge flag
	SW2 = 1; // Signal SW2 for pause and language has occured
}

void GPIOPortE_Handler(void){
	GPIO_PORTE_ICR_R = 0x10; // acknowledge flag
	SW1 = 1; // Signal SW1 for start and shoot has occured
}
