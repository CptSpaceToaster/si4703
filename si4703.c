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
uint16_t ret;

void si4703_setVolume(uint8_t volume) {
	printf("2.1\n");
	if (volume>0x0F) {
		volume = 0x0F;
	}
	printf("2.2\n");
	si4703_pull();
	printf("2.3\n");
	pancakes[SYSCONFIG2] = (pancakes[SYSCONFIG2] & 0xFFF0) | volume; //set volume
	si4703_push();
	printf("2.4\n");
}

//TODO: Figure out of this can be replaced with a uint16_t
void si4703_setChannel(int newChannel) {
	//Freq(MHz) = 0.200(in USA) * Channel + 87.5MHz
	//97.3 = 0.2 * Chan + 87.5
	//9.8 / 0.2 = 49
	newChannel *= 10; //973 * 10 = 9730
	newChannel -= 8750; //9730 - 8750 = 980
	
	newChannel /= 20; //980 / 20 = 49
	
	//These steps come from AN230 page 20 rev 0.5
	printf("1.1\n");
	si4703_pull();
	pancakes[CHANNEL] &= 0xFE00; //Clear out the channel bits
	pancakes[CHANNEL] |= newChannel; //Mask in the new channel
	pancakes[CHANNEL] |= (1<<TUNE); //Set the TUNE bit to start
	si4703_push();
	printf("1.2\n");
	
	while( !(pancakes[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //setting channel complete!
	printf("1.3\n");
	pancakes[CHANNEL] &= ~(1<<TUNE); //Clear the tune bit after a tune has completed
	si4703_push();
	printf("1.4\n");
	
	while( (pancakes[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //wait for the radio to clean up the STC bit
	printf("1.5\n");
}

uint8_t si4703_getChannel() {
	si4703_pull();
	ret = pancakes[READCHAN] & 0x03FF; //Mask out everything but the lower 10 bits
	
	//freq(MHz) = 0.200(in USA) * Channel + 87.5MHz
	//x = 0.2 * Chan + 87.5
	ret *= 2; //49 * 2 = 98	 (USA)
	
	ret += 875; //98 + 875 = 973
	return ret;
}

uint8_t si4703_seek(enum DIRECTION dir) {
	si4703_pull();
	pancakes[POWERCFG] &= ~(1<<SKMODE); //disable wrapping of frequencies
	if (dir == DOWN) {
		pancakes[POWERCFG] &= ~(_BV(UP));
	} else {
		pancakes[POWERCFG] |= _BV(UP);
	}
	pancakes[POWERCFG] |= (_BV(SEEK)); //start seeking
	
	si4703_push();
	while( !(pancakes[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //seek complete!
	
	ret = pancakes[STATUSRSSI] & (1<<SFBL); //Store the value of SFBL
	
	pancakes[POWERCFG] &= ~(_BV(SEEK)); //stop seeking
	
	while( (pancakes[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //wait for the radio to clean up the STC bit
	
	return ret;
}

void si4703_init() {
	printf("0.1\n");
	si4703_pull();
	printf("0.2\n");
	pancakes[OSCCTRL] = 0x8100;
	si4703_push();
	printf("0.3\n");
	_delay_ms(500); //Wait for oscillator to settle
	si4703_pull();
	printf("0.4\n");
	pancakes[POWERCFG] = 0x4001; //Enable the IC
	pancakes[SYSCONFIG1] |= _BV(RDS); //Enable RDS
	pancakes[SYSCONFIG2] &= ~(_BV(SPACE1) | _BV(SPACE0)); //FOrce 200 kHz channel spacing (USA)
	pancakes[SYSCONFIG2] &= ~(0x000F); //Turn down for what?!?
	pancakes[SYSCONFIG2] |= 0x0001; //Lowest volume setting
	printf("0.5\n");
	si4703_push();
	_delay_ms(110); //Waiting the max powerup time for the radio
	printf("0.6\n");
}



//The device starts reading at reg_index 0x0A, and has 32 bytes
void si4703_pull() {
	printf("HI\n");
	i2c_start_wait(SI4703_ADDR || I2C_READ);
	printf("HELLO\n");
	for (reg_index=0x00; reg_index<0x10; reg_index++) {
		printf("?\n");
		pancakes[(reg_index+0x0A) % 0x10] = i2c_readAck() << 8;
		printf("!\n");
		pancakes[(reg_index+0x0A) % 0x10] = i2c_readAck();
	}
	printf("GREETINGS\n");
	i2c_stop();
}


//The device starts writing at reg_index 4, and there are only 12 bytes that are controlling
void si4703_push() {
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