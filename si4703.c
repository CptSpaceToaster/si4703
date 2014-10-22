/*
 * si4703.c
 *
 * Created: 10/18/2014 3:30:02 PM
 * Author: CaptainSpaceToaster
 * 
 * Some of the defines came from a Nathan Seidle from Sparkfun
 * I owe him a beer due to the its inclusion (Beerware license).
 * https://www.sparkfun.com/products/10663
 */ 

#include <stdio.h>
#include "si4703.h"
#include "../twi/i2cmaster.h"
#include "util/delay.h"

//data representation of all 32 bytes 
uint16_t pancakes[16];
uint8_t reg_index;

void si4703_init() {
	si_4703_pull();
	pancakes[OSCCTRL] = 0x8100;
	si_4703_push();
	_delay_ms(500); //Wait for oscillator to settle
	si_4703_pull();
	pancakes[POWERCFG] = 0x4001; //Enable the IC
	pancakes[SYSCONFIG1] |= _BV(RDS); //Enable RDS
	pancakes[SYSCONFIG2] &= ~(_BV(SPACE1) | _BV(SPACE0)); //FOrce 200 kHz channel spacing (USA)
	pancakes[SYSCONFIG2] &= ~(0x000F); //Turn down for what?!?
	pancakes[SYSCONFIG2] |= 0x0001; //Lowest volume setting
	si_4703_push();
	_delay_ms(110); //Waiting the max powerup time for the radio
}



//The device starts reading at reg_index 0x0A, and has 32 bytes
void si_4703_pull() {
	i2c_start_wait(SI4703_ADDR || I2C_READ);
	for (reg_index=0x00; reg_index<0x10; reg_index++) {
		pancakes[(reg_index+0x0A) % 0x10] = (i2c_readAck() << 8) | i2c_readAck();
	}
	i2c_stop();
}


//The device starts writing at reg_index 4, and there are only 12 bytes that are controlling
void si_4703_push() {
	i2c_start_wait(SI4703_ADDR || I2C_WRITE);
	for (reg_index=0x00; reg_index<0x06; reg_index++) {
		i2c_write(pancakes[reg_index+2] >> 8);
		i2c_write(pancakes[reg_index+2] & 0x00FF);
	}
	i2c_stop();
}

/* shudder... shudder shudder shudder shudder 
void seek_TWI_devices() {
	uint8_t i = 0; 
	printf("Looking for devices...\n");
	for (i=0; i<=0x7F; i++) {
		if (!i2c_start(i<<1)) {
			printf("There is a Device at 0x%02x", i);
			i2c_stop();
			printf("\n");
		}
		_delay_ms(5);
	}
	printf("Searched for devices 0x00 to 0x7F\n");
}  */