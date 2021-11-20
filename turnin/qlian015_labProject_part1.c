/*	Author: Qipeng Liang
 *	Lab Section: 021
 *	Assignment: Lab #Custom Lab  Exercise #1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *
 *	Demo Link: https://youtu.be/Uth3r56E3nM
 */
#include <avr/io.h>
#include <unistd.h>
#include <stdio.h>
#include "bit.h"
#include "io.c"
#include "io.h"
#include "keypad.h"
#include "queue.h"
#include "scheduler.h"
#include "stack.h"
#include "timer.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

unsigned char winFlag = 0;
unsigned char score = 0;
unsigned char brickPattern = 0x01; //Pattern for the bricks, which is the last column for lv1
unsigned char brickEn = 0xE0; //Rows to enable for the bricks, which is all 5 rows in the beginning
unsigned char paddlePattern = 0x80; //Player's paddle pattern
unsigned char paddleEn = 0xF1; //The rows to enable for the paddle
unsigned char ballPos = 0x40; //Ball position, initialized to row 3 column 2
unsigned char ballEn = 0xFB; //Which row to enable for the ball, initialized to row 3

enum LED_States {Start, Wait, Wait2};
int LEDSMTick(int state) {

	static unsigned char cnt = 0;
	static unsigned char pattern = 0x00;//pattern  1 =  LED on (Positive pins)
	static unsigned char enable = 0x00; //row enable: if row = 0 -> display (Negative pins/ground)

	switch (state) {
		case Start:
			//if(~PINA & 0x01) state = Wait;
			if(cnt == 0) {
				cnt++;
				pattern = brickPattern;
				enable = brickEn;
				//pattern = 0x03;
				//enable = 0xE0;
			}
			else if (cnt == 1) {
				cnt++;
				pattern = paddlePattern;
				enable = paddleEn;
				//pattern = 0x80;
				//enable = 0xF1;
			}
			else if (cnt == 2) {
				cnt = 0;
				pattern = ballPos;
				enable = ballEn;
				//pattern = 0x40;
				//enable = 0xFB;
			}
			else {
				cnt = 0;
			}
			break;
		case Wait:
			if(PINA == 0xFF) state = Wait2;
			break;
		case Wait2:
			if(~PINA & 0x01) state = Start;
			break;
		default: 
			state = Start; break;
	}
	PORTC = pattern;
	PORTD = enable;
	return state;
}

unsigned char testCounter = 0;
enum Ball_States {Ball_Start, Ball_Right, Ball_Left, Ball_Wait, Ball_dright_up, Ball_dleft_up, Ball_dright_down, Ball_dleft_down};
int BallSMTick(int state) {
	switch(state) {
		case Ball_Start:
			state = Ball_Right;
			break;
		case Ball_Right:
			if(ballPos == 0x01) {
				state = Ball_Left;
				ballPos = 0x02;
			}
			else ballPos = (ballPos >> 1);
			break;
		case Ball_Left:
			if(ballPos == 0x40) {
				state = Ball_Right;
				ballPos = 0x20;
			}
			else ballPos = (ballPos << 1);
			if(testCounter == 1 && ballPos == 0x40){
				state = Ball_dright_up;
			}
			break;
		case Ball_dright_up: //diagonal right going up
			if(ballEn == 0xEF) {
				state = Ball_dleft_up;
				ballEn = (ballEn >> 1) | 0x80;
			}
			else ballEn = (ballEn << 1) | 0x01;
			ballPos = ballPos >> 1;
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
			if(ballEn == 0xEF) {
				state = Ball_dleft_down;
				ballEn = (ballEn >> 1) | 0x80;
			}
			else {
				ballEn = (ballEn << 1) | 0x01;
			}
			ballPos = ballPos << 1;
			break;
		case Ball_dleft_down: //diagonal left going down
			if(ballEn == 0xFE) {
				state = Ball_dright_down;
				ballEn = (ballEn << 1) | 0x01;
				ballPos = ballPos << 1;
			}
			else if(ballPos == 0x80) {
				state = Ball_dleft_up;
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos >> 1;
			}
			else {
				ballEn = (ballEn >> 1) | 0x80;
				ballPos = ballPos << 1;
			}
			break;
		default:
			state = Ball_Start; break;
	}

	return state;
}

enum Brick_States {Brick_start, Brick_update, Brick_wait};
int BrickSMTick(int state) {
	switch(state) {
		case Brick_start:
			state = Brick_update;
			break;
		case Brick_update:
			if(ballPos == 0x01) {
				brickEn = ( brickEn | (~ballEn) ) ;
				testCounter++;
			}
			break;
		default:
			state = Brick_start; break;
	}
	return state;
}

enum Paddle_States {Paddle_start, Paddle_update, Paddle_wait};
int PaddleSMTick(int state) {
	switch(state) {
		case Paddle_start:
			state = Paddle_update;
			break;
		case Paddle_update:
			if( (~PINA & 0x01) && paddleEn != 0xF8 ) {
				paddleEn = (paddleEn >> 1) | 0x80;
				state = Paddle_wait;
			}
			else if (~PINA & 0x01) {
				paddleEn = (paddleEn << 1) | 0x01;
				state = Paddle_wait;
			}
			break;
		case Paddle_wait:
			if(~PINA & 0x01) state = Paddle_wait;
			else state = Paddle_update;
			break;
		default:
			state = Paddle_start; break;
	}
		
	return state;
}

int main(void) {
	DDRA = 0x00;	PORTA = 0xFF;
	DDRC = 0xFF;	PORTC = 0x00;
	DDRD = 0xFF;	PORTD = 0x00;
	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	const char start = -1;
	task1.state = start;
	task1.period = 1;
	task1.elapsedTime = task1.period;
	task1.TickFct = &LEDSMTick;

	task2.state = start;
	task2.period = 500;
	task2.elapsedTime = task2.period;
	task2.TickFct = &BallSMTick;

	task3.state = start;
	task3.period = 400;
	task3.elapsedTime = task3.period;
	task3.TickFct = &BrickSMTick;

	task4.state = start;
	task4.period = 100;
	task4.elapsedTime = task4.period;
	task4.TickFct = &PaddleSMTick;

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
