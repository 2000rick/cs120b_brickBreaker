/*	Author: Qipeng Liang
 *	Lab Section: 021
 *	Assignment: Custom Lab Prpoject  Phase #3
 *	Description: Brick Breaker, Custom Lab Project.
 *	Current code is work in progress for the second phase of the project.
 *	Objectives for the current phase are: integrate shift register with LED matrix, continue on the game mechanics, and start joystick implementation.
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
unsigned short joyLR = 0; //Position of joystick in the left and right axis
unsigned short joyUD = 0; //position of joystick in the up down axis
unsigned char joyMove = 0x00; //Where the joystick is turned, J0 represents left, J1 right, J2 Up, J3 Down
unsigned char brickPattern = 0x01; //Pattern for the bricks, which is the last column for lv1
unsigned char brickEn = 0xE0; //Rows to enable for the bricks, which is all 5 rows in the beginning
unsigned char paddlePattern = 0x80; //Player's paddle pattern
//unsigned char paddleEn = 0xF3; //The rows to enable for the paddle
unsigned char paddleLen = 0xFB ; //enable for left portion of paddle
unsigned char paddleRen = 0xF7; //enable for right portion of paddle
unsigned char ballPos = 0x40; //Ball position, initialized to row 3 column 2
unsigned char ballEn = 0xFB; //Which row to enable for the ball, initialized to row 3

enum LED_States {Start, GameWon, GameOver, Wait, Wait2};
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
			if(loseFlag) { state = GameOver; cnt = 0; }
			break;
		case GameWon:
			cnt++;
			pattern = 0xFF;
			enable = 0xE0;
			if(joyMove & 0x02) { state = Start; cnt = 0; pattern = 0x00; enable = 0xFF; winFlag = 0; }
			break;
		case GameOver:
			cnt++;
			pattern = 0xFF;
			enable = 0xFB;
			if(joyMove & 0x02) { state = Start; cnt = 0; pattern = 0x00; enable = 0xFF; loseFlag = 0; }
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
	DataWrite(enable);
	//PORTD = enable;
	return state;
}

enum Ball_States {Ball_Start, Ball_Right, Ball_Left, Ball_Wait, Ball_dright_up, Ball_dleft_up, Ball_dright_down, Ball_dleft_down};
int BallSMTick(int state) {
	switch(state) {
		case Ball_Start:
			if(joyMove & 0x02) state = Ball_Right;//Push up on joystick to start 
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
		default:
			state = Ball_Start; break;
	}
	if(winFlag) { state = Ball_Start; ballPos = 0x40; ballEn = 0xFB; }
	if(loseFlag) {state = Ball_Start; ballPos = 0x40; ballEn = 0xFB; }
	if(joyMove & 0x01) { state = Ball_Start; ballPos = 0x40; ballEn = 0xFB; }
	return state;
}

enum Brick_States {Brick_start, Brick_update, Brick_wait};
int BrickSMTick(int state) {
	switch(state) {
		case Brick_start:
			brickEn = 0xE0;
			if(joyMove & 0x02) state = Brick_update;
			break;
		case Brick_update:
			if(ballPos == 0x01) {
				brickEn = ( brickEn | (~ballEn) ) ;
				if(brickEn == 0xFF) {
					winFlag = 1;
					state = Brick_start;
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
	
	//PORTB = joyMove;
	return state;	
}

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

void Nokia_init() {
	PORTB &= 0xFE;
	delay_ms(50);
	PORTB |= 0x01;	
}

void Nokia_setup(unsigned char input) {
	PORTB &= 0xFD; //CE active low
	for(unsigned char i = 0; i < 8; ++i) {
		if(input & 0x80) PORTB |= 0x20;
		else PORTB &= 0xDF;
		input = input << 1;
		PORTB |= 0x80;
		delay_ms(1);
		PORTB &= 0x7F;
		delay_ms(1);
	}
	PORTB |= 0x02; //Set CE back to high
}

#include <avr/pgmspace.h>
#include <stdint.h>

const uint8_t CHARSET[][5] PROGMEM = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00 }, // 20 space
	{ 0x00, 0x00, 0x5f, 0x00, 0x00 }, // 21 !
	{ 0x00, 0x07, 0x00, 0x07, 0x00 }, // 22 "
	{ 0x14, 0x7f, 0x14, 0x7f, 0x14 }, // 23 #
	{ 0x24, 0x2a, 0x7f, 0x2a, 0x12 }, // 24 $
	{ 0x23, 0x13, 0x08, 0x64, 0x62 }, // 25 %
	{ 0x36, 0x49, 0x55, 0x22, 0x50 }, // 26 &
	{ 0x00, 0x05, 0x03, 0x00, 0x00 }, // 27 '
	{ 0x00, 0x1c, 0x22, 0x41, 0x00 }, // 28 (
	{ 0x00, 0x41, 0x22, 0x1c, 0x00 }, // 29 )
	{ 0x14, 0x08, 0x3e, 0x08, 0x14 }, // 2a *
	{ 0x08, 0x08, 0x3e, 0x08, 0x08 }, // 2b +
	{ 0x00, 0x50, 0x30, 0x00, 0x00 }, // 2c ,
	{ 0x08, 0x08, 0x08, 0x08, 0x08 }, // 2d -
	{ 0x00, 0x60, 0x60, 0x00, 0x00 }, // 2e .
	{ 0x20, 0x10, 0x08, 0x04, 0x02 }, // 2f /
	{ 0x3e, 0x51, 0x49, 0x45, 0x3e }, // 30 0
	{ 0x00, 0x42, 0x7f, 0x40, 0x00 }, // 31 1
	{ 0x42, 0x61, 0x51, 0x49, 0x46 }, // 32 2
	{ 0x21, 0x41, 0x45, 0x4b, 0x31 }, // 33 3
	{ 0x18, 0x14, 0x12, 0x7f, 0x10 }, // 34 4
	{ 0x27, 0x45, 0x45, 0x45, 0x39 }, // 35 5
	{ 0x3c, 0x4a, 0x49, 0x49, 0x30 }, // 36 6
	{ 0x01, 0x71, 0x09, 0x05, 0x03 }, // 37 7
	{ 0x36, 0x49, 0x49, 0x49, 0x36 }, // 38 8
	{ 0x06, 0x49, 0x49, 0x29, 0x1e }, // 39 9
	{ 0x00, 0x36, 0x36, 0x00, 0x00 }, // 3a :
	{ 0x00, 0x56, 0x36, 0x00, 0x00 }, // 3b ;
	{ 0x08, 0x14, 0x22, 0x41, 0x00 }, // 3c <
	{ 0x14, 0x14, 0x14, 0x14, 0x14 }, // 3d =
	{ 0x00, 0x41, 0x22, 0x14, 0x08 }, // 3e >
	{ 0x02, 0x01, 0x51, 0x09, 0x06 }, // 3f ?
	{ 0x32, 0x49, 0x79, 0x41, 0x3e }, // 40 @
	{ 0x7e, 0x11, 0x11, 0x11, 0x7e }, // 41 A
	{ 0x7f, 0x49, 0x49, 0x49, 0x36 }, // 42 B
	{ 0x3e, 0x41, 0x41, 0x41, 0x22 }, // 43 C
	{ 0x7f, 0x41, 0x41, 0x22, 0x1c }, // 44 D
	{ 0x7f, 0x49, 0x49, 0x49, 0x41 }, // 45 E
	{ 0x7f, 0x09, 0x09, 0x09, 0x01 }, // 46 F
	{ 0x3e, 0x41, 0x49, 0x49, 0x7a }, // 47 G
	{ 0x7f, 0x08, 0x08, 0x08, 0x7f }, // 48 H
	{ 0x00, 0x41, 0x7f, 0x41, 0x00 }, // 49 I
	{ 0x20, 0x40, 0x41, 0x3f, 0x01 }, // 4a J
	{ 0x7f, 0x08, 0x14, 0x22, 0x41 }, // 4b K
	{ 0x7f, 0x40, 0x40, 0x40, 0x40 }, // 4c L
	{ 0x7f, 0x02, 0x0c, 0x02, 0x7f }, // 4d M
	{ 0x7f, 0x04, 0x08, 0x10, 0x7f }, // 4e N
	{ 0x3e, 0x41, 0x41, 0x41, 0x3e }, // 4f O
	{ 0x7f, 0x09, 0x09, 0x09, 0x06 }, // 50 P
	{ 0x3e, 0x41, 0x51, 0x21, 0x5e }, // 51 Q
	{ 0x7f, 0x09, 0x19, 0x29, 0x46 }, // 52 R
	{ 0x46, 0x49, 0x49, 0x49, 0x31 }, // 53 S
	{ 0x01, 0x01, 0x7f, 0x01, 0x01 }, // 54 T
	{ 0x3f, 0x40, 0x40, 0x40, 0x3f }, // 55 U
	{ 0x1f, 0x20, 0x40, 0x20, 0x1f }, // 56 V
	{ 0x3f, 0x40, 0x38, 0x40, 0x3f }, // 57 W
	{ 0x63, 0x14, 0x08, 0x14, 0x63 }, // 58 X
	{ 0x07, 0x08, 0x70, 0x08, 0x07 }, // 59 Y
	{ 0x61, 0x51, 0x49, 0x45, 0x43 }, // 5a Z
	{ 0x00, 0x7f, 0x41, 0x41, 0x00 }, // 5b [
	{ 0x02, 0x04, 0x08, 0x10, 0x20 }, // 5c backslash
	{ 0x00, 0x41, 0x41, 0x7f, 0x00 }, // 5d ]
	{ 0x04, 0x02, 0x01, 0x02, 0x04 }, // 5e ^
	{ 0x40, 0x40, 0x40, 0x40, 0x40 }, // 5f _
	{ 0x00, 0x01, 0x02, 0x04, 0x00 }, // 60 `
	{ 0x20, 0x54, 0x54, 0x54, 0x78 }, // 61 a
	{ 0x7f, 0x48, 0x44, 0x44, 0x38 }, // 62 b
	{ 0x38, 0x44, 0x44, 0x44, 0x20 }, // 63 c
	{ 0x38, 0x44, 0x44, 0x48, 0x7f }, // 64 d
	{ 0x38, 0x54, 0x54, 0x54, 0x18 }, // 65 e
	{ 0x08, 0x7e, 0x09, 0x01, 0x02 }, // 66 f
	{ 0x0c, 0x52, 0x52, 0x52, 0x3e }, // 67 g
	{ 0x7f, 0x08, 0x04, 0x04, 0x78 }, // 68 h
	{ 0x00, 0x44, 0x7d, 0x40, 0x00 }, // 69 i
	{ 0x20, 0x40, 0x44, 0x3d, 0x00 }, // 6a j
	{ 0x7f, 0x10, 0x28, 0x44, 0x00 }, // 6b k
	{ 0x00, 0x41, 0x7f, 0x40, 0x00 }, // 6c l
	{ 0x7c, 0x04, 0x18, 0x04, 0x78 }, // 6d m
	{ 0x7c, 0x08, 0x04, 0x04, 0x78 }, // 6e n
	{ 0x38, 0x44, 0x44, 0x44, 0x38 }, // 6f o
	{ 0x7c, 0x14, 0x14, 0x14, 0x08 }, // 70 p
	{ 0x08, 0x14, 0x14, 0x18, 0x7c }, // 71 q
	{ 0x7c, 0x08, 0x04, 0x04, 0x08 }, // 72 r
	{ 0x48, 0x54, 0x54, 0x54, 0x20 }, // 73 s
	{ 0x04, 0x3f, 0x44, 0x40, 0x20 }, // 74 t
	{ 0x3c, 0x40, 0x40, 0x20, 0x7c }, // 75 u
	{ 0x1c, 0x20, 0x40, 0x20, 0x1c }, // 76 v
	{ 0x3c, 0x40, 0x30, 0x40, 0x3c }, // 77 w
	{ 0x44, 0x28, 0x10, 0x28, 0x44 }, // 78 x
	{ 0x0c, 0x50, 0x50, 0x50, 0x3c }, // 79 y
	{ 0x44, 0x64, 0x54, 0x4c, 0x44 }, // 7a z
	{ 0x00, 0x08, 0x36, 0x41, 0x00 }, // 7b {
	{ 0x00, 0x00, 0x7f, 0x00, 0x00 }, // 7c |
	{ 0x00, 0x41, 0x36, 0x08, 0x00 }, // 7d }
	{ 0x10, 0x08, 0x08, 0x10, 0x08 }, // 7e ~
	{ 0x00, 0x00, 0x00, 0x00, 0x00 } // 7f
};


#define PORT_LCD PORTB
#define DDR_LCD DDRB

#define LCD_SCE PB1
#define LCD_RST PB0
#define LCD_DC PB2
#define LCD_DIN PB5
#define LCD_CLK PB7

#define LCD_CONTRAST 0x40

void nokia_lcd_init(void);

void nokia_lcd_clear(void);

/**
 * Power of display
 * @lcd: lcd nokia struct
 * @on: 1 - on; 0 - off;
 */
void nokia_lcd_power(uint8_t on);

/**
 * Set single pixel
 * @x: horizontal pozition
 * @y: vertical position
 * @value: show/hide pixel
 */
void nokia_lcd_set_pixel(uint8_t x, uint8_t y, uint8_t value);

/**
 * Draw single char with 1-6 scale
 * @code: char code
 * @scale: size of char
 */
void nokia_lcd_write_char(char code, uint8_t scale);

/**
 * Draw string. Example: writeString("abc",3);
 * @str: sending string
 * @scale: size of text
 */
void nokia_lcd_write_string(const char *str, uint8_t scale);

/**
 * Set cursor position
 * @x: horizontal position
 * @y: vertical position
 */
void nokia_lcd_set_cursor(uint8_t x, uint8_t y);

/*
 * Render screen to display
 */
void nokia_lcd_render(void);

static struct {
    /* screen byte massive */
    uint8_t screen[504];

    /* cursor position */
    uint8_t cursor_x;
    uint8_t cursor_y;

} nokia_lcd = {
    .cursor_x = 0,
    .cursor_y = 0
};

static void write(uint8_t bytes, uint8_t is_data)
{
	register uint8_t i;
	/* Enable controller */
	PORT_LCD &= ~(1 << LCD_SCE);

	/* We are sending data */
	if (is_data)
		PORT_LCD |= (1 << LCD_DC);
	/* We are sending commands */
	else
		PORT_LCD &= ~(1 << LCD_DC);

	/* Send bytes */
	for (i = 0; i < 8; i++) {
		/* Set data pin to byte state */
		if ((bytes >> (7-i)) & 0x01)
			PORT_LCD |= (1 << LCD_DIN);
		else
			PORT_LCD &= ~(1 << LCD_DIN);

		/* Blink clock */
		PORT_LCD |= (1 << LCD_CLK);
		PORT_LCD &= ~(1 << LCD_CLK);
	}

	/* Disable controller */
	PORT_LCD |= (1 << LCD_SCE);
}

static void write_cmd(uint8_t cmd)
{
	write(cmd, 0);
}

static void write_data(uint8_t data)
{
	write(data, 1);
}

void nokia_lcd_init(void)
{
	register unsigned i;
	/* Set pins as output */
	DDR_LCD |= (1 << LCD_SCE);
	DDR_LCD |= (1 << LCD_RST);
	DDR_LCD |= (1 << LCD_DC);
	DDR_LCD |= (1 << LCD_DIN);
	DDR_LCD |= (1 << LCD_CLK);

	/* Reset display */
	PORT_LCD |= (1 << LCD_RST);
	PORT_LCD |= (1 << LCD_SCE);
	delay_ms(10);
	PORT_LCD &= ~(1 << LCD_RST);
	delay_ms(70);
	PORT_LCD |= (1 << LCD_RST);

	/*
	 * Initialize display
	 */
	/* Enable controller */
	PORT_LCD &= ~(1 << LCD_SCE);
	/* -LCD Extended Commands mode- */
	write_cmd(0x21);
	/* LCD bias mode 1:48 */
	write_cmd(0x13);
	/* Set temperature coefficient */
	write_cmd(0x06);
	/* Default VOP (3.06 + 66 * 0.06 = 7V) */
	write_cmd(0xC2);
	/* Standard Commands mode, powered down */
	write_cmd(0x20);
	/* LCD in normal mode */
	write_cmd(0x09);

	/* Clear LCD RAM */
	write_cmd(0x80);
	write_cmd(LCD_CONTRAST);
	for (i = 0; i < 504; i++)
		write_data(0x00);

	/* Activate LCD */
	write_cmd(0x08);
	write_cmd(0x0C);
}

void nokia_lcd_clear(void)
{
	register unsigned i;
	/* Set column and row to 0 */
	write_cmd(0x80);
	write_cmd(0x40);
	/*Cursor too */
	nokia_lcd.cursor_x = 0;
	nokia_lcd.cursor_y = 0;
	/* Clear everything (504 bytes = 84cols * 48 rows / 8 bits) */
	for(i = 0;i < 504; i++)
		nokia_lcd.screen[i] = 0x00;
}

void nokia_lcd_power(uint8_t on)
{
	write_cmd(on ? 0x20 : 0x24);
}

void nokia_lcd_set_pixel(uint8_t x, uint8_t y, uint8_t value)
{
	uint8_t *byte = &nokia_lcd.screen[y/8*84+x];
	if (value)
		*byte |= (1 << (y % 8));
	else
		*byte &= ~(1 << (y %8 ));
}

void nokia_lcd_write_char(char code, uint8_t scale)
{
	register uint8_t x, y;

	for (x = 0; x < 5*scale; x++)
		for (y = 0; y < 7*scale; y++)
			if (pgm_read_byte(&CHARSET[code-32][x/scale]) & (1 << y/scale))
				nokia_lcd_set_pixel(nokia_lcd.cursor_x + x, nokia_lcd.cursor_y + y, 1);
			else
				nokia_lcd_set_pixel(nokia_lcd.cursor_x + x, nokia_lcd.cursor_y + y, 0);

	nokia_lcd.cursor_x += 5*scale + 1;
	if (nokia_lcd.cursor_x >= 84) {
		nokia_lcd.cursor_x = 0;
		nokia_lcd.cursor_y += 7*scale + 1;
	}
	if (nokia_lcd.cursor_y >= 48) {
		nokia_lcd.cursor_x = 0;
		nokia_lcd.cursor_y = 0;
	}
}

void nokia_lcd_write_string(const char *str, uint8_t scale)
{
	while(*str)
		nokia_lcd_write_char(*str++, scale);
}

void nokia_lcd_set_cursor(uint8_t x, uint8_t y)
{
	nokia_lcd.cursor_x = x;
	nokia_lcd.cursor_y = y;
}

void nokia_lcd_render(void)
{
	register unsigned i;
	/* Set column and row to 0 */
	write_cmd(0x80);
	write_cmd(0x40);

	/* Write screen to display */
	for (i = 0; i < 504; i++)
		write_data(nokia_lcd.screen[i]);
}


int main(void) {
	DDRA = 0x00;	PORTA = 0xFF;
	DDRB = 0xFF;	PORTB = 0x03; //0000 0011 PB0 Reset, PB1 ChipEnable, PB2 Data/Command, PB5 Din, PB7 Clk
	DDRC = 0xFF;	PORTC = 0x00;
	DDRD = 0x07;	PORTD = 0xF8; //PD0 is DS, PD1 is ShiftReg ClkIn, PD2 is StorageReg ClkIn

	ADC_init();
	nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_write_string("IT'S WORKING!",1);
    nokia_lcd_set_cursor(0, 10);
    nokia_lcd_write_string("Hello World", 1);
    nokia_lcd_render();
	/*delay_ms(5);
	Nokia_init();
	Nokia_setup(0x21);
	Nokia_setup(0x90);
	Nokia_setup(0x20);
	Nokia_setup(0x0C);
	PORTB |= 0x04; //Swicth from command to data
	Nokia_setup(0x1F);
	Nokia_setup(0x05);
	Nokia_setup(0x07);
	Nokia_setup(0x00);
	Nokia_setup(0x1F);*/
	

	static task task1, task2, task3, task4, task5;
	task *tasks[] = { &task1, &task2, &task3, &task4, &task5 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	const char start = -1;
	task1.state = start;
	task1.period = 1;
	task1.elapsedTime = task1.period;
	task1.TickFct = &LEDSMTick;

	task2.state = start;
	task2.period = 400;
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
