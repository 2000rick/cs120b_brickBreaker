/*	Author: Qipeng Liang
 *	Lab Section: 021
 *	Assignment: Lab #Custom Lab  Exercise #1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
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
unsigned char brickPattern = 0x03; //Pattern for the bricks, which is the last 2 columns for lv1
unsigned char brickEn = 0xE0; //Rows to enable for the bricks, which is all 5 rows
unsigned char paddlePattern = 0x80; //Player's paddle pattern
unsigned char paddleEn = 0xF1; //The rows to enable for the paddle
unsigned char ballPos = 0x40; //Ball position, initialized to row 3 column 2
unsigned char ballEn = 0xFB; //Which row to enable for the ball, initialized to row 3

enum LED_States {Start, Wait, Wait2};
int LED_Tick(int state) {

	static unsigned char cnt = 0;
	static unsigned char pattern = 0x00;//pattern  1 =  LED on (Positive pins)
	static unsigned char enable = 0x00; //row enable: if row = 0 -> display (Negative pins/ground)

	switch (state) {
		case Start:
			if(~PINA & 0x01) state = Wait;
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

int main(void) {
	DDRA = 0x00;	PORTA = 0xFF;
	DDRC = 0xFF;	PORTC = 0x00;
	DDRD = 0xFF;	PORTD = 0x00;
	static task task1;
	task *tasks[] = { &task1 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	const char start = -1;
	task1.state = start;
	task1.period = 1;
	task1.elapsedTime = task1.period;
	task1.TickFct = &LED_Tick;

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
