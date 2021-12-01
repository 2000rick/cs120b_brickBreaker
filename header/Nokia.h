#ifndef NOKIA_H
#define NOKIA_H

#include "charSet.h"

void Nokia_write(unsigned char input) {
	PORTB &= 0xFD; //CE active low
	for(unsigned char i = 0; i < 8; ++i) {
		if(input & 0x80) PORTB |= 0x08;
		else PORTB &= ~(0x08);
		input = input << 1;
		PORTB |= 0x10; //Pulse Clk 1/2
		//delay_ms(1);
		PORTB &= ~(0x10); //Pulse Clk 2/2
	}
	PORTB |= ~(0xFD); //Set CE back to high
}

void Nokia_print(char input) {		//prints 1 char to the LCD
	for(unsigned char i = 0; i < 5; ++i) {
		Nokia_write(charset[input-32][i]);
	}
}

void Nokia_string(char* input) { //prints the Cstring one char at a time
	while(*input) { Nokia_print(*input++); }
}

void Nokia_clear() {//(84*48) pixels over 8 pixels/bits per byte gives us 504//
	for(unsigned i = 0; i < 504; ++i) Nokia_write(0x00);
	PORTB &= ~(0x04); //command mode
	Nokia_write(0x40); //Set Y-cursor to 0 (datasheet 14)
	Nokia_write(0x80); //Set X-cursor to 0
	PORTB |= 0x04;	 //change back to data mode
}

void Nokia_init() {
	PORTB &= 0xFE; //pulse reset to initialize LCD 1/2
	delay_ms(50);
	PORTB |= ~(0xFE); // pulse 2/2
	Nokia_write(0x23); //commands to set up LCD 1/4 (datasheet 22), enabled vertical addressing here
	Nokia_write(0x90);
	Nokia_write(0x20);
	Nokia_write(0x0C); //command mode 4/4
	PORTB |= 0x04; //Swicth from command to data
	Nokia_clear();
	/*Nokia_string("Welcome to Brick Breaker");
	for(unsigned char i = 0; i < 132; ++i) Nokia_write(0x00);
	Nokia_string("Move joystick up to start"); */
	/*Nokia_write(0x1F);
	Nokia_write(0x05);
	Nokia_write(0x07);
	Nokia_write(0x00);
	Nokia_write(0x1F);//PI should be displayed (from datasheet)
	*/
}

#endif

