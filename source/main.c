/*	Author: Qipeng Liang
 *	Lab Section: 021
 *	Assignment: Lab #Custom Lab  Exercise #1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
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

enum LED_States {Start};
int LED_Tick(int state) {

	static unsigned char cnt = 0;
	static unsigned char pattern = 0x80;	//pattern  1 =  LED on
	static unsigned char enable = 0xFE; //row enable: if row = 0 -> display

	switch (state) {
		case Start:
			if(cnt == 0) {
				cnt++;
				pattern = 0x83;
				enable = 0xE0;
			}
			else if (cnt == 1) {
				cnt++;
				pattern = 0x46;
			}
			else if (cnt == 2) {
				cnt++;
				pattern = 0x2C;
			}
			else if (cnt == 3) {
				cnt++;
				pattern = 0x18;
			}
			else if (cnt == 4) {
				cnt++;
				pattern = 0x38;
			}
			else if (cnt == 5) {
				cnt++;
				pattern = 0x64;
			}
			else if (cnt == 6) {
				cnt++;
				pattern = 0xC2;
			}
			else if (cnt == 7) {
				cnt++;
				pattern = 0x81;
			}
			else if (cnt == 8) {
				cnt = 0;
				pattern = 0x00;
			}
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
	task1.period = 1000;
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
