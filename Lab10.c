// Lab10.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 1/16/2021 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* 
 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "SSD1306.h"
#include "Print.h"
#include "Random.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "Switch.h"
#include "main.h"
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
#define PB54                  (*((volatile uint32_t *)0x400050C0)) // bits 5-4
#define PF321                 (*((volatile uint32_t *)0x40025038)) // bits 3-1
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040)) 
// TExaSdisplay logic analyzer shows 7 bits 0,PB5,PB4,PF3,PF2,PF1,0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PB54; // sends at 10kHz
}
void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}
// edit this to initialize which pins you use for profiling
void Profile_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x22;      // activate port B,F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 
  GPIO_PORTF_DEN_R |=  0x0E;   // enable digital I/O on PF3,2,1
  GPIO_PORTB_DIR_R |=  0x30;   // output on PB4 PB5
  GPIO_PORTB_DEN_R |=  0x30;   // enable on PB4 PB5  
}


//********************************************************************************

#define MaxEnemies 6
#define MaxMissiles 20

volatile uint8_t SW1,SW2,last = 0;
void Delay100ms(uint32_t count); // time delay in 0.1 seconds
uint32_t random;
uint32_t score = 0;
typedef enum {dead,alive} status_t; // Sets 0 to dead status and 1 to alive status
typedef enum {English,Spanish} language_t; // Language select
typedef enum {GameOver,Score,Restart1,Restart2,Paused} phrase_t;

language_t selLanguage = English; // default selected language is English

const char GameOver_English[] = "Game Over!";
const char Score_English[] = "Your Score:";
const char Restart1_English[] = "Press Fire to";
const char Restart2_English[] = "play again";
const char Paused_English[] = "Game Paused";

const char GameOver_Spanish[] = "Juego Terminado!";
const char Score_Spanish[] = "Su Puntaci\xA2""n";
const char Restart1_Spanish[] = "Presione Disparar";
const char Restart2_Spanish[] = "para reinciar";
const char Paused_Spanish[] = "Jueago Pausado";

const char *phrases [5][2] = {
	{GameOver_English,GameOver_Spanish},
	{Score_English,Score_Spanish},
	{Restart1_English,Restart1_Spanish},
	{Restart2_English,Restart2_Spanish},
	{Paused_English,Paused_Spanish}
};

int8_t lives = 3; // 3 lives for player

struct sprite { // sprite struct
	int32_t x; // x coordinate 0 to 127
	int32_t vx; // x velo pixels/50ms
	double y; // y coordinate 0 to 127
	double vy; // y velo pixels/ 50ms
	const uint8_t *image; //ptr to image
	status_t life; // dead (0) or alive (1)
}; 
typedef struct sprite sprite_t;

// Declaration of Sprites
sprite_t Enemies[MaxEnemies]; // enemy sprites
sprite_t eLasers[MaxEnemies]; // enemy lasers
sprite_t Player; // Player ship sprite
sprite_t Missiles[MaxMissiles]; // missiles for shooting
int NeedToDraw; // Set to 1 when the image needs to be updated on the OLED

// Establish enemies and player
void Init(void){ int i;
	
	for(i=0;i<MaxEnemies;i++){
		Enemies[i].x = 20*i + 12; // set each enemy across
		Enemies[i].y = 10; // along the top of the screen 
		Enemies[i].image = Alien10pointA; // assigns image to sprite
		Enemies[i].life = alive; // makes enemy alive
		Enemies[i].vy = 0.3;
		
		if(i < 3){
			Enemies[i].vx = 1; //right
		}
		if(i >= 3) {
			Enemies[i].vx = -1; // left
		}
	}
	
	for(i=0;i<MaxEnemies;i++){
		eLasers[i].life = dead;
		eLasers[i].vy = 0.6;
		eLasers[i].image = Laser0;
	}
	
	for(i=0;i<MaxMissiles;i++){
		Missiles[i].life = dead;
		Missiles[i].vy = -2;
		Missiles[i].image = Missile0;
	}
	
	//Player.x = 48;
	Player.y = 60;
	Player.image = PlayerShip0;
	Player.life = alive;
	
	NeedToDraw = 1; // mark that this needs to be drawn	
}

// Drawing out all sprites
void Draw(void){ int i;
	SSD1306_ClearBuffer();
	if(Player.life == alive){
			SSD1306_DrawBMP(Player.x,Player.y,Player.image,0,SSD1306_INVERSE);
	}
	
	for(i=0;i<MaxEnemies;i++){
		if(Enemies[i].life == alive){ // if the enemy is alive
			SSD1306_DrawBMP(Enemies[i].x,Enemies[i].y,Enemies[i].image,0,SSD1306_INVERSE);
		}
		if(eLasers[i].life == alive){
			SSD1306_DrawBMP(eLasers[i].x,eLasers[i].y,eLasers[i].image,0,SSD1306_INVERSE);
		}
	}
	
	
	for(i=0;i<MaxMissiles;i++){
		if(Missiles[i].life == alive){
			SSD1306_DrawBMP(Missiles[i].x,Missiles[i].y,Missiles[i].image,0,SSD1306_INVERSE);
		}
	}
	
	uint32_t x = 0;
	for(i=0;i<lives;i++){ 
		SSD1306_DrawBMP(x,58,heart,0,SSD1306_INVERSE);
		x += 4;
	}
	
	
	SSD1306_DrawUDec2(0,0,score,SSD1306_INVERSE);
	
	
	SSD1306_OutBuffer();
	NeedToDraw = 0;
	
}

void Move(void){ int i,j; uint32_t adcData;
	
	if(Player.life == alive){
		adcData = ADC_In();
		Player.x = ((127-16)*adcData)/4096;
		NeedToDraw = 1;
	}
	
	for(i=0;i<MaxEnemies;i++){
		
		if(Enemies[i].life == alive){
			
			NeedToDraw = 1; // need to draw new info
			if(Enemies[i].y > 62 || Enemies[i].y < 10){ // if the enemy is hitting top or bottom edge
				Enemies[i].life = dead; // kill it
				lives --;
			}
			if(Enemies[i]. x > 114 || Enemies[i].x < 2){ // if the enemy is hitting the left or right edge
				if(Enemies[i].x > 114){ // if it is too far right 
					Enemies[i].x = 112; // reset it near right edge
					Enemies[i].vx = -Enemies[i].vx; // invert its velocity
				} else { // if it is too far left
					Enemies[i].x = 2; // reset at left edge
					Enemies[i].vx = -Enemies[i].vx; // invert velocity
				}
			}
			else{
				Enemies[i].x += Enemies[i].vx; // add velocities
				Enemies[i].y += Enemies[i].vy;			
			}
		}
	}
	
	for(i=0;i<MaxMissiles;i++){ uint32_t x1,x2,y1,y2; int j = 0; 
	
		if(Missiles[i].life == alive){
			NeedToDraw = 1;
			x1 = Missiles[i].x + 1; // Missile Position
			y1 = Missiles[i].y - 4;
			if(Missiles[i].y < 10){ // if it hits the top it's dead now
				Missiles[i].life = dead;
			} else {
				Missiles[i].y += Missiles[i].vy; // make it rise up
			}
			
			for(j=0;j<MaxEnemies;j++){ //for each missile, compare its position to each enemy
				x2 = Enemies[j].x + 6; // Enemy position
				y2 = Enemies[j].y - 4;
				
				if(Enemies[j].life == alive){
				if((((x1-x2)*(x1-x2))+((y1-y2)*(y1-y2))) < 22){ // Square distance collision check
					Enemies[j].life = dead;
					Missiles[i].life = dead;
					score += 10;
					Sound_Explosion();
				}
			}	
		 }			
		}
	}
	
	for(i=0;i<MaxEnemies;i++){ uint32_t x1,x2,y1,y2,j;
		if(eLasers[i].life == alive){
			NeedToDraw = 1;
			x1 = eLasers[i].x;
			y1 = eLasers[i].y;
			if(eLasers[i].y > 62){
				eLasers[i].life = dead;
			} else {
				eLasers[i].y += eLasers[i].vy;
			}
			//Check for missile deflection
			for(j=0;j<MaxMissiles;j++){
				if(Missiles[j].life == alive){
					x2 = Missiles[j].x;
					y2 = Missiles[j].y;
					if(((x1-x2)*(x1-x2))+((y1-y2)*(y1-y2)) < 6){
						Missiles[j].life = dead;
						eLasers[i].life = dead;
					}
				}
			}
			//Check for hitting the player ship
			x2 = Player.x + 8;
			y2 = Player.y + 3;
			if(((x1-x2)*(x1-x2))+((y1-y2)*(y1-y2)) < 40){
				eLasers[i].life = dead;
				lives--;
			}
		}
	}

}
//Output score by converting to string
void SSD1306_DrawUDec2(int16_t x, int16_t y, uint32_t n, uint16_t color){
char buf[4];
if(n>=1000){
buf[0] = '*'; /* illegal */
buf[1] = '*'; /* illegal */
buf[2] = '*'; // illegal
}else{
if(n>=100){
buf[0]= n/100+'0'; /* hundreds digit */
buf[1] = (n%100)/10+'0'; /* tens digit */
buf[2] = '0'; // ones digit is always 0
}else{
buf[0]=n/10+'0'; // tens digit
buf[1] ='0'; /* ones digit */
}
}
buf[3]=0; // null
SSD1306_DrawString(x,y,buf,color);
}

void CreateMissile (void){ int i = 0;
	while(Missiles[i].life == alive){ // find next dead missile
		i++;
	}
	Missiles[i].life = alive;
	Missiles[i].x = Player.x + 8;
	Missiles[i].y = Player.y - 4;
	NeedToDraw = 1;
}

void ReturnFire (uint32_t enemy){int i; int count = 0;
	for(i=0;i<MaxEnemies;i++){ // limit returning lasers to two at a time to increase survival chances
		if(eLasers[i].life == alive){
			count++;
		}
	}
	if(eLasers[enemy].life == dead && count < 2){
		eLasers[enemy].life = alive;
		eLasers[enemy].x = Enemies[enemy].x + 6;
		eLasers[enemy].y = Enemies[enemy].y + 4;
		//Sound_Invader1();
	}
}

/*int main(void){uint32_t time=0;
  DisableInterrupts();
  // pick one of the following three lines, all three set to 80 MHz
  //PLL_Init();                   // 1) call to have no TExaS debugging
  TExaS_Init(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  //TExaS_Init(&ScopeTask);       // or 3) call to activate analog scope PD2
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();
	SysTick_Init(4000000); // 20 Hz Interrupt
  Random_Init(42);
  Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(2, 62, SpaceInvadersMarquee, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  EnableInterrupts();
  Delay100ms(20);
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(47, 63, PlayerShip0, 0, SSD1306_WHITE); // player ship bottom
  SSD1306_DrawBMP(53, 55, Bunker0, 0, SSD1306_WHITE);

  SSD1306_DrawBMP(0, 9, Alien10pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(20,9, Alien10pointB, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(40, 9, Alien20pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(60, 9, Alien20pointB, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(80, 9, Alien30pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(50, 19, AlienBossA, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  Delay100ms(30);

  SSD1306_OutClear();  
  SSD1306_SetCursor(1, 1);
  SSD1306_OutString("GAME OVER");
  SSD1306_SetCursor(1, 2);
  SSD1306_OutString("Nice try,");
  SSD1306_SetCursor(1, 3);
  SSD1306_OutString("Earthling!");
  SSD1306_SetCursor(2, 4);
  while(1){
    Delay100ms(10);
    SSD1306_SetCursor(19,0);
    SSD1306_OutUDec2(time);
    time++;
    PF1 ^= 0x02;
  }
} */



int main(void){
	DisableInterrupts();
  // pick one of the following three lines, all three set to 80 MHz
  //PLL_Init();                   // 1) call to have no TExaS debugging
  TExaS_Init(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  //TExaS_Init(&ScopeTask);       // or 3) call to activate analog scope PD2
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();
	ADC_Init(5);
	Button_Init();
	Sound_Init();
	SysTick_Init(4000000); // 20 Hz Interrupt
  Random_Init(42);
  Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(2, 62, SpaceInvadersMarquee, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  EnableInterrupts();
	while(SW1 == 0){} // while shoot button is not pressed, stay on banner
	
		SW1 = 0;
	
	SSD1306_ClearBuffer();
	SSD1306_OutBuffer();
	SSD1306_SetCursor(0,0);
	SSD1306_OutString("English");
	SSD1306_SetCursor(0,10);
	SSD1306_OutString(" < ");
	SSD1306_SetCursor(0,1);
	SSD1306_OutString("Espa\xA4ol");
	
	uint8_t line = 0;	
	while(SW1 == 0){ // language select
			if(SW2 == 1){ 
				SW2 = 0;
				selLanguage ^= 1; // toggle language
				line ^= 1;
				SSD1306_ClearBuffer();
				SSD1306_OutBuffer();
				SSD1306_SetCursor(0,0);
				SSD1306_OutString("English");
				SSD1306_SetCursor(0,1);
				SSD1306_OutString("Espa\xA4ol");
				SSD1306_SetCursor(10,line);
				SSD1306_OutString(" < ");
			}
			
	}
	
	SW1 = 0;
	
while(1){ // Entire Program
	
	Init(); // Initialize images
	int count = 0;
	
	// Gameplay Loop of one round
	while(lives > 0){ int shot,i; 
		if(NeedToDraw == 1){ // drawing of sprites
			Draw();
			if(shot == 1){ // light blast LED
				shot = 0;
				PB4 ^= 0x10;
			}
		}
		random = Random();
		int Invaderrand = 0;
			 Invaderrand = Random()%3+1
		if(random < MaxEnemies){ // generate enemy laser if random number generated an enemy designation
			if(Enemies[random].life == alive){
			ReturnFire(random);
				if(Invaderrand == 1){Sound_Invader1();}
				   if(Invaderrand == 2){Sound_Invader2();}
				   if(Invaderrand == 3){Sound_Invader3();}
			}
		}
		if(SW1 == 1){ // If the fire button is pressed
			SW1 = 0;
			PB4 ^= 0x10; // Flash LED
			shot = 1;
			CreateMissile();
			Sound_Shoot();
			last = GPIO_PORTC_DATA_R&0x0010;
		}
		if(SW2 == 1){ // If pause button is pressed
			SW2 = 0;
			Sound_HighPitch();
			SSD1306_SetCursor(4,0);
			SSD1306_OutString((char *)phrases[Paused][selLanguage]);
			PB5 ^= 0x20;
			while(SW2 == 0){NVIC_ST_CTRL_R=0;}
			Sound_HighPitch();
			SW2 = 0;
			PB5 ^= 0x20;
			NVIC_ST_CTRL_R = 0x07;
		}
		
		for(i=0;i<MaxEnemies;i++){
			if(Enemies[i].life == dead){
				count++;
				Sound_Killed();
			}
		}
		
		if(count == MaxEnemies){
			Init();
		}
		
		count = 0;
		
	}
		
	// Game Over Screen
		Sound_Explosion();
		NVIC_ST_CTRL_R = 0;
		SSD1306_ClearBuffer();
		SSD1306_OutBuffer();
		SSD1306_OutClear();
		SSD1306_SetCursor(0,0);
		SSD1306_OutString((char *)phrases[GameOver][selLanguage]);
		SSD1306_SetCursor(0,1);
		SSD1306_OutString((char *)phrases[Score][selLanguage]);
	  SSD1306_SetCursor(14,1);
		SSD1306_OutUDec(score);
		SSD1306_SetCursor(0,3);
		SSD1306_OutString((char *)phrases[Restart1][selLanguage]);
	  SSD1306_SetCursor(0,4);
	  SSD1306_OutString((char *)phrases[Restart2][selLanguage]);
	  score = 0;
	
	while(SW1 == 0){ // wait for new press
	}
	
	lives = 3;
	NVIC_ST_CTRL_R = 0x07;
}
}
// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}



void SysTick_Init(uint32_t period){
	NVIC_ST_CTRL_R = 0; // disable systick
	NVIC_ST_RELOAD_R = period - 1; // set time to interrupt
	NVIC_ST_CURRENT_R = 0; // clear register
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF) | 0x80000000; //priority 3
	NVIC_ST_CTRL_R = 0x07; // enable clock with interrupts
}

void SysTick_Handler(void){
	Move();
}
