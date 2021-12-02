/*	Author: Qipeng Liang
 *	Lab Section: 021
 *	Assignment: Custom Lab Prpoject  Phase #3
 *	Description: Brick Breaker, Custom Lab Project.
 *		Final Phase of project, main objectives:
 *		*Get Nokia LCD screen to work with hardware and write the software driver functions for it, then get it to display to appropriate informational messages
 *		*Develop the second level of the game
 *		*Solve any known bugs and/or optimize game logic if neccessary
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *
 *	Demo Link: 
 */

#include <avr/io.h>
#include <stdio.h>
#include "bit.h"
#include "io.c"
#include "io.h"
#include "queue.h"
#include "scheduler.h"
#include "stack.h"
#include "timer.h"
#include "Nokia.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

//Begin Shift Register Driver Functions
void ClockPulse() { //generates a pulse for the ShiftReg Clk
	PORTD = PORTD | 0x02; //high
	PORTD = PORTD & 0xFD; //low
}

void SetHigh() { PORTD = PORTD | 0x01; } //bit to shift is 1
void SetLow() { PORTD = PORTD & 0xFE; } //bit to shift is 0

void DataWrite(unsigned char input) {
	PORTD = PORTD & 0xFB; //Disable PD2 output while writing
	for(unsigned char i = 0; i < 8; ++i) {
		if(input & 0x80) SetHigh(); //masking checks if the MSB is 1
		else SetLow();
		input = input << 1; //LeftShift to check the next MST
		ClockPulse();
	}
	PORTD = PORTD | 0x04; //Set PD2 high for output
}
//End ShiftReg Drivers

unsigned char winFlag = 0;
unsigned char loseFlag = 0;
unsigned char score = 0;
unsigned char sFlag = 0; //Signal to update score in LCD
unsigned short joyLR = 0; //Position of joystick in the left and right axis
unsigned short joyUD = 0; //position of joystick in the up down axis
unsigned char joyMove = 0x00; //Where the joystick is turned, J0 represents left, J1 right, J2 Up, J3 Down
unsigned char brickPattern = 0x01; //Pattern for the bricks, which is the last column/row for lv1
unsigned char brickEn = 0xE0; //Rows to enable for the bricks, which is all 5 rows in the beginning
unsigned char paddlePattern = 0x80; //Player's paddle pattern
unsigned char paddleLen = 0xFB ; //enable for left portion of paddle
unsigned char paddleRen = 0xF7; //enable for right portion of paddle
unsigned char ballPos = 0x40; //Ball position, initialized to row 3 column 2
unsigned char ballEn = 0xFB; //Which row to enable for the ball, initialized to row 3
unsigned char brickPattern2 = 0x02; //Additional bricks for lv2
unsigned char brickEn2 = 0xE0; //brick enable for lv2, which is all 5 rows/columns initially
unsigned char lv2 = 0; //Flag that indicates we are in level 2 
unsigned char clearFlag = 0; //signals to the BallSM that one row in lv2 has been cleared

enum LED_States {Start, GameWon, GameOver, Level2, Wait };
int LEDSMTick(int state) {

	static unsigned short cnt = 0;
	static unsigned char pattern = 0x00;//pattern  1 =  LED on (Positive pins)
	static unsigned char enable = 0xFF; //row enable: if row = 0 -> display (Negative pins/ground)

	switch (state) {
		case Start:
			if(cnt == 0) {
				cnt++;
				enable = 0xFF; //enable off to prevent ghosting
			}
			else if(cnt == 1) {
				cnt++;
				pattern = brickPattern;
				enable = brickEn;
			}
			else if (cnt == 2) {
				cnt++;
				enable = 0xFF; //enable is off
			}
			else if (cnt == 3) {
				cnt++;
				pattern = paddlePattern;
				enable = paddleLen;
			}
			else if (cnt == 4) {
				cnt++;
				enable = 0xFF;
			}
			else if (cnt == 5) {
				cnt++;
				enable = paddleRen;
			}
			else if(cnt == 6) {
				cnt++;
				enable = 0xFF;
			}
			else if (cnt == 7) {
				cnt = 0;
				pattern = ballPos;
				enable = ballEn;
			}
			else {
				cnt = 0;
			}
			if(winFlag) { state = GameWon; cnt = 0; }
			if(loseFlag) { state = GameOver; cnt = 0; lv2 = 0; }
			break;
		case GameWon:
			cnt++;
			pattern = 0xFF;
			enable = 0xE0;
			if(joyMove & 0x02) { state = Level2; cnt = 0; pattern = 0x00; enable = 0xFF; winFlag = 0; lv2 = 1; }
			break;
		case GameOver:
			cnt++;
			pattern = 0xFF;
			enable = 0xFB;
			if(joyMove & 0x02) { state = Start; cnt = 0; pattern = 0x00; enable = 0xFF; loseFlag = 0; }
			break;
		case Level2:
			if(cnt == 0) {
				cnt++;
				enable = 0xFF; //enable off to prevent ghosting
			}
			else if(cnt == 1) {
				cnt++;
				pattern = brickPattern;
				enable = brickEn;
			}
			else if (cnt == 2) {
				cnt++;
				enable = 0xFF; //enable is off
			}
			else if (cnt == 3) {
				cnt++;
				pattern = brickPattern2;
				enable = brickEn2;
			}
			else if (cnt == 4) {
				cnt++;
				enable = 0xFF;
			}
			else if (cnt == 5) {
				cnt++;
				pattern = paddlePattern;
				enable = paddleLen;
			}
			else if (cnt == 6) {
				cnt++;
				enable = 0xFF;
			}
			else if (cnt == 7) {
				cnt++;
				enable = paddleRen;
			}
			else if(cnt == 8) {
				cnt++;
				enable = 0xFF;
			}
			else if (cnt == 9) {
				cnt = 0;
				pattern = ballPos;
				enable = ballEn;
			}
			else {
				cnt = 0;
			}
			if(winFlag) { state = GameWon; cnt = 0; }
			if(loseFlag) { state = GameOver; cnt = 0; }
			break;
		default: 
			state = Start; break;
	}
	PORTC = pattern;
	DataWrite(enable);
	return state;
}

enum Ball_States {Ball_Start, Ball_Right, Ball_Left, Ball_dright_up, Ball_dleft_up, Ball_dright_down, Ball_dleft_down,
	   			  			 Ball_up2, Ball_down2, Ball_dright_up2, Ball_dleft_up2, Ball_dright_down2, Ball_dleft_down2 };
int BallSMTick(int state) {
	switch(state) {
		case Ball_Start:
			if(joyMove & 0x02) { //Push up on joystick to start 
				if(lv2) { state = Ball_up2; }
				else	{ state = Ball_Right; }
			}
			break;
		case Ball_Right:
			if(ballPos == 0x01) {
				state = Ball_Left;
				ballPos = 0x02;
			}
			else ballPos = (ballPos >> 1);
			break;
		case Ball_Left:
			if(ballPos == 0x80) {
				state = Ball_Right;
				ballPos = 0x40;
			}
			else ballPos = (ballPos << 1);
			if(ballPos == 0x80 && ballEn == paddleRen) { //ball goes diagonal if hits right side of paddle
				state = Ball_dright_up;
			}
			if(ballPos == 0x80 && (ballEn != paddleLen) && (ballEn != paddleRen)) { loseFlag = 1; } //Losing Condition
			break;
		case Ball_dright_up: //diagonal right going up
			if(ballEn == 0xEF && ballPos == 0x01) { //case where ball hits top-right corner
				state = Ball_dleft_down;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
			else if(ballEn == 0xEF) { //case where ball hits right boundary/wall
				state = Ball_dleft_up;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos >> 1;
			}
			else if(ballPos == 0x01) { //case where ball hits the top
				state = Ball_dright_down;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			else {
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos >> 1;
			}
			break;
		case Ball_dleft_up: //diagonal left going up
			if(ballEn == 0xFE && ballPos == 0x01) { //case where ball hits top-left corner
				state = Ball_dright_down;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			else if(ballEn == 0xFE) { //case where ball hits left boundary/wall
				state = Ball_dright_up;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos >> 1;
			}
			else if(ballPos == 0x01) { //case where ball hits the top, but not corner
				state = Ball_dleft_down;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
			else { //tyical case
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos >> 1;
			}
			break;
		case Ball_dright_down: //diagonal right going down
			if(ballEn == 0xEF && ballPos == 0x80) { //case where ball hits bottom-right corner
				state = Ball_dleft_up;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos >> 1;
			}
			else if(ballEn == 0xEF) { //case where ball hits right boundary/wall
				state = Ball_dleft_down;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
			else if(ballPos == 0x80) { //case where ball reaches the bottom/paddle
				if(ballEn != paddleLen) {
					state = Ball_dright_up;
					ballEn = (ballEn << 1) | 0x01;
					ballPos = ballPos >> 1;
				}
				else {
					state = Ball_Right;
					ballPos = ballPos >> 1;
				}
			}
			else {
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			if(ballPos == 0x80 && (ballEn != paddleLen) && (ballEn != paddleRen)) { loseFlag = 1; }
			break;
		case Ball_dleft_down: //diagonal left going down
			if(ballEn == 0xFE && ballPos == 0x80) { //case where ball hits bottom-left corner
				state = Ball_dright_up;
				//ballEn = (ballEn << 1) | 0x01;  //commented out this line to make the ball be able to hit all bricks
				ballPos = ballPos >> 1;
			}
			else if(ballEn == 0xFE) { //case where ball hits left bounadry/wall
				state = Ball_dright_down;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			else if(ballPos == 0x80) { //case where ball hits bottom/paddle
				if(ballEn != paddleLen) {
					state = Ball_dleft_up;
					ballEn = (ballEn >> 1) | 0x80;
					ballPos = ballPos >> 1;
				}
				else {
					state = Ball_Right;
					ballPos = ballPos >> 1;
				}
			}
			else {
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
		    if(ballPos == 0x80 && (ballEn != paddleLen) && (ballEn != paddleRen)) { loseFlag = 1; }
			break;
		case Ball_up2: //Level 2 cases begin here...
			if(ballPos == 0x02) {
				state = Ball_down2;
				ballPos = 0x04;
			}
			else ballPos = (ballPos >> 1);
			break;
		case Ball_down2:
			if(ballPos == 0x80) {
				state = Ball_up2;
				ballPos = 0x40;
			}
			else ballPos = (ballPos << 1);
			if(ballPos == 0x80 && ballEn == paddleRen) { //ball goes diagonal if hits right side of paddle
				state = Ball_dright_up2;
			}
			if(ballPos == 0x80 && (ballEn != paddleLen) && (ballEn != paddleRen)) { loseFlag = 1; } //Losing Condition
			break;
		case Ball_dright_up2: //diagonal right going up
			if(ballEn == 0xEF && ballPos == 0x02) { //case where ball hits top-right corner
				state = Ball_dleft_down2;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
			else if(ballEn == 0xEF) { //case where ball hits right boundary/wall
				state = Ball_dleft_up2;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos >> 1;
			}
			else if(ballPos == 0x02) { //case where ball hits the top
				state = Ball_dright_down2;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			else {
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos >> 1;
			}
			break;
		case Ball_dleft_up2: //diagonal left going up
			if(ballEn == 0xFE && ballPos == 0x02) { //case where ball hits top-left corner
				state = Ball_dright_down2;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			else if(ballEn == 0xFE) { //case where ball hits left boundary/wall
				state = Ball_dright_up2;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos >> 1;
			}
			else if(ballPos == 0x02) { //case where ball hits the top, but not corner
				state = Ball_dleft_down2;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
			else { //tyical case
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos >> 1;
			}
			break;
		case Ball_dright_down2: //diagonal right going down
			if(ballEn == 0xEF && ballPos == 0x80) { //case where ball hits bottom-right corner
				state = Ball_dleft_up2;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos >> 1;
			}
			else if(ballEn == 0xEF) { //case where ball hits right boundary/wall
				state = Ball_dleft_down2;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
			else if(ballPos == 0x80) { //case where ball reaches the bottom/paddle
				if(ballEn != paddleLen) {
					state = Ball_dright_up2;
					//ballEn = (ballEn << 1) | 0x01; //commented out for lv2 game adjustment
					ballPos = ballPos >> 1;
				}
				else {
					state = Ball_up2;
					ballPos = ballPos >> 1;
				}
			}
			else {
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			if(ballPos == 0x80 && (ballEn != paddleLen) && (ballEn != paddleRen)) { loseFlag = 1; }
			break;
		case Ball_dleft_down2: //diagonal left going down
			if(ballEn == 0xFE && ballPos == 0x80) { //case where ball hits bottom-left corner
				state = Ball_dright_up2;
				//ballEn = (ballEn << 1) | 0x01;  //commented out this line to make the ball be able to hit all bricks
				ballPos = ballPos >> 1;
			}
			else if(ballEn == 0xFE) { //case where ball hits left bounadry/wall
				state = Ball_dright_down2;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			else if(ballPos == 0x80) { //case where ball hits bottom/paddle
				if(ballEn != paddleLen) {
					state = Ball_dleft_up2;
					ballEn = (ballEn >> 1) | 0x80;
					ballPos = ballPos >> 1;
				}
				else {
					state = Ball_up2;
					ballPos = ballPos >> 1;
				}
			}
			else {
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
		    if(ballPos == 0x80 && (ballEn != paddleLen) && (ballEn != paddleRen)) { loseFlag = 1; }
			break;
		default:
			state = Ball_Start; break;
	}
	if(winFlag) { state = Ball_Start; ballPos = 0x40; ballEn = 0xFB; }
	if(loseFlag) {state = Ball_Start; ballPos = 0x40; ballEn = 0xFB; }
	if(joyMove & 0x01) { state = Ball_Start; ballPos = 0x40; ballEn = 0xFB; }
	if(clearFlag && ballPos == 0x80 ) { clearFlag = 0; state = Ball_dright_up; } 
	return state;
}

enum Brick_States {Brick_start, Brick_update, Brick_update2, Brick_wait};
int BrickSMTick(int state) {
	static unsigned char comp; //used to determine if a brick was hit and update score
	switch(state) {
		case Brick_start:
			brickEn = 0xE0;
			brickEn2 = 0xE0;
			if(joyMove & 0x02) {
				if(lv2) { state = Brick_update2; }
				else { state = Brick_update; }
			}
			break;
		case Brick_update:
			if(ballPos == 0x01) {
				comp = brickEn;
				brickEn = ( brickEn | (~ballEn) ) ;
				if(comp != brickEn) { ++score; sFlag = 1; }
				if(brickEn == 0xFF) {
					winFlag = 1;
					state = Brick_start;
				}
			}
			break;
		case Brick_update2: //Lv2 brick update
			if(ballPos == 0x02) {
				comp = brickEn2;
				brickEn2 = (brickEn2 | (~ballEn) );
				if(comp != brickEn2) { ++score; sFlag = 1; }
				if(brickEn2 == 0xFF) {
					clearFlag = 1;
					state = Brick_update;
				}
			}
			break;
		default:
			state = Brick_start; break;
	}
	if(loseFlag) state = Brick_start;
	if(joyMove & 0x01) state = Brick_start;
	return state;
}

enum Paddle_States {Paddle_start, Paddle_move, Paddle_wait1, Paddle_wait2};
int PaddleSMTick(int state) {
	switch(state) {
		case Paddle_start:
			paddleLen = 0xFB;
			paddleRen = 0xF7;
			if(joyMove & 0x02) state = Paddle_move;
			break;
		case Paddle_move:
			if( (joyMove & 0x04) && paddleLen != 0xFE ) {
				paddleLen = (paddleLen >> 1) | 0x80;
				paddleRen = (paddleRen >> 1) | 0x80;
				state = Paddle_wait1;
			}
			else if( (joyMove & 0x08) && paddleRen != 0xEF) {
				paddleLen = (paddleLen << 1) | 0x01;
				paddleRen = (paddleRen << 1) | 0x01;
				state = Paddle_wait2;
			}
			else state = Paddle_move;
			break;
		case Paddle_wait1:
			if(joyMove & 0x04) state = Paddle_wait1;
			else state = Paddle_move;
			break;
		case Paddle_wait2:
			if(joyMove & 0x08) state = Paddle_wait2;
			else state = Paddle_move;
			break;
		default:
			state = Paddle_start; break;
	}
	if(winFlag) state = Paddle_start;
	if(loseFlag) state = Paddle_start;
	if(joyMove & 0x01) state = Paddle_start;
	return state;
}
enum Nokia_States {Nokia_start, Nokia_inGame, Nokia_score, Nokia_win1, Nokia_lose1, Nokia_wait, Nokia_waitScore, Nokia_winWait, Nokia_loseWait};
int NokiaSMTick(int state) {
	switch(state) {
		case Nokia_start:
			score = 0;
			Nokia_clear();
			Nokia_string("Welcome to Brick Breaker");
			for(unsigned char i = 0; i < 132; ++i) Nokia_write(0x00);
			Nokia_string("Move joystick up to start");
			state = Nokia_wait;
			break;
		case Nokia_inGame:
			Nokia_clear();
			Nokia_string("Game in progress ...");
			for(unsigned char i = 0; i < 152; ++i) Nokia_write(0x00);
			Nokia_string("score: ");
			Nokia_print(score + '0');
			state = Nokia_waitScore;
			break;
		case Nokia_score:
			PORTB &= ~(0x04); //command
			Nokia_write(0x43);//set Y pos
			Nokia_write(0x9F); //set X pos
			PORTB |= 0x04; //data
			Nokia_write(0x00); Nokia_write(0x00); Nokia_write(0x00); Nokia_write(0x00); //clear score on LCD
			Nokia_print(score + '0'); //display new score on LCD
			state = Nokia_waitScore;
			if(score == 5) state = Nokia_win1;
			break;
		case Nokia_win1:
			Nokia_clear();
			Nokia_string("Nice! You Won!");
			for(unsigned char i = 0; i < 99; ++i) Nokia_write(0x00);
			Nokia_string("Current score: ");
			Nokia_print(score + '0');
			state = Nokia_winWait;
			break;
		case Nokia_lose1:
			Nokia_clear();
			Nokia_string("GG. Game Over!");
			for(unsigned char i = 0; i < 99; ++i) Nokia_write(0x00);
			Nokia_string("Final score: ");
			Nokia_print(score + '0');
			state = Nokia_loseWait;
			break;
		case Nokia_wait:
			if(joyMove & 0x02) state = Nokia_inGame;
			break;
		case Nokia_waitScore:
			if(sFlag) { state = Nokia_score; sFlag = 0; }
			break;
		case Nokia_winWait:
			if(!winFlag) state = Nokia_start;
			break;
		case Nokia_loseWait:
			if(!loseFlag) state = Nokia_start;
			break;
		default:
			state = Nokia_start; break;
	}
	if(joyMove & 0x01) { state = Nokia_start; }
	if(loseFlag && state != Nokia_loseWait) { state = Nokia_lose1; }
	return state;
}

//Joystick Driver Function/Machine, only needs 1 state
//Sole purpose is to write to shared variable joyMove which is used by other tasks
enum Joystick_States {Joystick_start};
int JoystickSMTick(int state) {
	joyLR = ADC; //Read ADC value from PA0
	if(joyLR > 700) {
		joyMove = 0x02;
	}
	else if (joyLR < 300) {
		joyMove = 0x01;
	}

	ADMUX |= 0x01; //Set ADMUX so that we read the ADC value from PA1
	delay_ms(1); //wait for 1ms so that the ADC value has time to update
	joyUD = ADC; //Read ADC value from PA1
	if(joyUD > 700) {
		joyMove |= 0x04;
	}
	else if(joyUD < 300) {
		joyMove |= 0x08;
	}

	if(joyLR < 700 && joyLR > 300 && joyUD < 700 && joyUD > 300) {
		joyMove = 0; //If joystick is in the center or deadzone area
	}
	ADMUX &= 0xFE; //Next ADC value read will be from PA0
	
	return state;	
}

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

int main(void) {
	DDRA = 0x00;	PORTA = 0xFF;
	DDRB = 0xFF;	PORTB = 0x03; //0000 0011 PB0 Reset, PB1 ChipEnable, PB2 Data/Command, PB3 Din, PB4 Clk
	DDRC = 0xFF;	PORTC = 0x00;
	DDRD = 0x07;	PORTD = 0xF8; //PD0 is DS, PD1 is ShiftReg ClkIn, PD2 is StorageReg ClkIn

	ADC_init();
	Nokia_init();

	static task task1, task2, task3, task4, task5, task6;
	task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	const char start = -1;
	task1.state = start;
	task1.period = 1;
	task1.elapsedTime = task1.period;
	task1.TickFct = &LEDSMTick;

	task2.state = start;
	task2.period = 300;
	task2.elapsedTime = task2.period;
	task2.TickFct = &BallSMTick;

	task3.state = start;
	task3.period = 200;
	task3.elapsedTime = task3.period;
	task3.TickFct = &BrickSMTick;

	task4.state = start;
	task4.period = 50;
	task4.elapsedTime = task4.period;
	task4.TickFct = &PaddleSMTick;

	task5.state = start;
	task5.period = 20;
	task5.elapsedTime = task5.period;
	task5.TickFct = &JoystickSMTick;

	task6.state = start;
	task6.period = 150;
	task6.elapsedTime = task6.period;
	task6.TickFct = &NokiaSMTick;

	unsigned long GCD = tasks[0]->period;
	for(int i = 1; i < numTasks; i++) {
		GCD = findGCD(GCD, tasks[i]->period);
	}
	TimerSet(GCD);
	TimerOn();
	
	unsigned short i;
    while (1) {
		for(i = 0; i < numTasks; i++) {
			if(tasks[i]->elapsedTime >= tasks[i]->period) {
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += GCD;
		}
		while(!TimerFlag) {}
		TimerFlag = 0;
    }
    return 1;
}
