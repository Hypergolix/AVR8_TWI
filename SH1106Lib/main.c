#define F_CPU 16000000UL
#define SCL_CLOCK 400000UL	/*400000UL*/

#define DEVID 0x78
#define RAMCOM 0x40
#define COMMAND 0x00

#define ADXLID 0xA6
#define READ 0x01
#define WRITE 0x00

#define WIDTH 132
#define HEIGHT 64

#define LED_BUILTIN 0

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "font.h"

void error(){
	PORTB = (1<<LED_BUILTIN);
	while(1);
}

void TWIinit(){
	TWSR = 0x00;
	TWBR = ((F_CPU/SCL_CLOCK)-16)/2;
}
// Check status code
void TWIstatus(uint8_t CODE){
	while (!(TWCR & (1<<TWINT)));
	if ((TWSR & 0xF8) != CODE)
	error();
}
// Send data and check status code
void TWIwrite(uint8_t DATA, uint8_t CODE){
	TWDR = DATA;
	TWCR = (1<<TWINT) | (1<<TWEN);
	TWIstatus(CODE);
}

uint8_t TWIread(uint8_t SLA_RW){
	// Start condition
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	TWIstatus(0x10);
	// Send slave address
	TWIwrite(SLA_RW, 0x40);
	TWCR |= (0<<TWEA);
	TWIstatus(0x58);
	return TWDR;
}

void TWIbegin(uint8_t SLA_RW, uint8_t CODE){
	// Start condition
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	TWIstatus(0x08);
	// Send slave address
	TWIwrite(SLA_RW, CODE);
}

void TWIend(void){
	// Stop
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

void OLEDwrite(uint8_t DATA){
	TWIbegin(DEVID, 0x18);
	// RAM or COMMAND
	TWIwrite(COMMAND, 0x28);
	//
	TWIwrite(DATA, 0x28);
	TWIend();
}

void OLEDwriteDual(uint8_t DATA, uint8_t DATA2){
	TWIbegin(DEVID, 0x18);
	// RAM or COMMAND
	TWIwrite(COMMAND, 0x28);
	TWIwrite(DATA, 0x28);
	TWIwrite(DATA2, 0x28);
	TWIend();
}

void OLEDwriteRAM(uint8_t DATA){
	TWIbegin(DEVID, 0x18);
	// RAM or COMMAND
	TWIwrite(RAMCOM, 0x28);
	//
	TWIwrite(DATA, 0x28);
	TWIend();
}

void setPage(uint8_t page) {
	OLEDwrite(0xB0 | page);
}

void clrOLED(uint8_t pixels){
	for(uint8_t i = 0; i < 8;i++){
		setPage(i);
		for(uint8_t j = 0; j < WIDTH; j++){
			OLEDwriteRAM(pixels);
		}
	}
}

void writeSpace(uint8_t spaceSize) {
	for (uint8_t y = 0; y < spaceSize; y++) {
		OLEDwriteRAM(0x00);
	}
}

void setColumn(uint8_t increment) {
	OLEDwrite(0b00010000 | (increment & 0b00001111));
	OLEDwrite(0b00000000 | ((increment & 0b11110000) >> 4));
}

void printLetter(uint16_t startPos, uint8_t page, uint8_t space){
	uint8_t column = 0x00;
	for (uint8_t xPos = 0; xPos < 8; xPos++) {
		column = fontArray[startPos + xPos];
		/*
		if (column != 0) {
			OLEDwriteRAM(column);
		}
		*/
		OLEDwriteRAM(column);
	}
	//writeSpace(space);
}

// Page, column, text
void OLEDstring(uint8_t page, uint8_t column, char text[] ){
	setPage(page);
	setColumn(column + 2);
	for (uint8_t stringPos = 0; stringPos < (strlen(text) -1); stringPos++) {
		uint16_t startPos = text[stringPos];
		if (startPos > 32 && startPos < 91) {
			startPos = (startPos - 33) * 8;
			printLetter(startPos, page, 2);
			} else if (startPos == 32) {
			writeSpace(2);
			} else {
			printLetter(464, page, 2);
		}
	}
}

void ADXLwrite(uint8_t ADDRESS, uint8_t DATA){
	TWIbegin(ADXLID | WRITE, 0x18);
	TWIwrite(ADDRESS, 0x28);
	TWIwrite(DATA, 0x28);
	TWIend();
}

uint8_t ADXLread(uint8_t ADDRESS){
	TWIbegin(ADXLID | WRITE, 0x18);
	TWIwrite(ADDRESS, 0x28);
	uint8_t recievedByte = TWIread(ADXLID | READ);
	
	TWIend();

	/*
	TWIwrite(ADDRESS, 0x18);
	uint8_t readData = TWIread(0x50);
	TWIend();
	return readData;
	*/
	return recievedByte;
}


int main(void)
{
	DDRB = 0xFF;
	PORTB = 0x00;
	
	TWIinit();
	
	OLEDwrite(0xAE);
	OLEDwriteDual(0xD5, 0x80);
	OLEDwriteDual(0xD3, 0x00);
	OLEDwrite(0x40);
	OLEDwriteDual(0xAD, 0x8B);
	OLEDwrite(0x33);
	OLEDwrite(0xC8);
	OLEDwrite(0xA1);
	OLEDwriteDual(0xDA, 0x12);
	OLEDwriteDual(0xD9, 0xF1);
	OLEDwriteDual(0xDB, 0x40);
	OLEDwriteDual(0x81, 0xFF);
	OLEDwrite(0xA4);
	OLEDwrite(0xA6);
	clrOLED(0x00);
	OLEDwrite(0xAF);
	
	OLEDstring(0, 0, "1");

	// ADXL345 SETUP
	ADXLwrite(0x31, 0x00);
	ADXLwrite(0x2C, 0x0A);
	ADXLwrite(0x2D, 0x08);
	// Read ID
	uint8_t ADXL = ADXLread(0x00);
	
	/**/
	while(1){
		uint8_t xRegister = ADXLread(0x32);
		
		char id = xRegister;
		char str[4];
		for(uint8_t i = 0; i < 3; i++){
			str[2 - i] = id % 10;
			str[2 - i] += 0x30;
			id /= 10;
		}
		str[4] = '\0';
		
		//clrOLED(0x00);
		OLEDstring(0, 0, str);
		
		_delay_ms(500);
	}
	
	while(1){
		PORTB |= (1<<LED_BUILTIN);
		clrOLED(0xFF);
		_delay_ms(500);
		PORTB &= ~(1<<LED_BUILTIN);
		clrOLED(0x00);
		_delay_ms(500);
	}

}

