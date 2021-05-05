// ADC.c
// Runs on TM4C123
// Provide functions that initialize ADC0
// Last Modified: 1/16/2021
// Student names: change this to your names or look very silly
// Last modification date: change this to the last modification date or look very silly

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

// ADC initialization function 
// Input: none
// Output: none
// measures from PD2, analog channel 5
void ADC_Init(uint32_t sac){ 

// from lab 8
uint32_t delay;
	
 SYSCTL_RCGCGPIO_R |= 0x08; // Enable D clock
 while((SYSCTL_PRGPIO_R&0x08) == 0){} // Busy-wait
	
	GPIO_PORTD_DIR_R &= ~(1 << 2); // set PD2 as an input
	GPIO_PORTD_AFSEL_R |= 0x04; // enable alternate function
	GPIO_PORTD_DEN_R &= ~(1 << 2); // disable I/O PD2
	GPIO_PORTD_AMSEL_R |= 0x04; // enable analog
	SYSCTL_RCGCADC_R |= 0x01; // Activate ADC0
	 
	delay = SYSCTL_RCGCADC_R; // Extra time to stabilize
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	 delay = SYSCTL_RCGCADC_R; // Extra time to stabilize
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	 delay = SYSCTL_RCGCADC_R; // Extra time to stabilize
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	
	 ADC0_PC_R = 0x01; //configure for 125K
	 ADC0_SSPRI_R = 0x0123; // Sequence 3 is highest priority
	 ADC0_ACTSS_R &= ~0x0008; // disable sample sequencer 3
	 ADC0_EMUX_R &= ~0xF000; // Seq 3 is software trigger
	 ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+5; // Ain 5
	 ADC0_SSCTL3_R = 0x0006; // no TS0 D0, yes IE0 END0
	 ADC0_IM_R &= ~0x0008; // diable SS3 interrupts
	 ADC0_ACTSS_R |= 0x0008; // enable sample sequencer 3
	 ADC0_SAC_R = sac;
}

//------------ADC_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
// measures from PD2, analog channel 5
uint32_t ADC_In(void){  

  // from lab 8
    uint32_t data;
	
	ADC0_PSSI_R = 0x0008; // Start the ADC
	while((ADC0_RIS_R&0x08) == 0){}; // Busy-wait
		data = ADC0_SSFIFO3_R&0xFFF; // read data
		ADC0_ISC_R = 0x0008; // clear flag
		
		return data;
}


