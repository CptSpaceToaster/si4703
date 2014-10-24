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
uint16_t si4703_data_registers[16];
uint8_t reg_index;
uint16_t ret;

void si4703_setVolume(uint8_t volume) {
	if (volume>0x0F) {
		volume = 0x0F;
	}
	si4703_pull();
	si4703_data_registers[SYSCONFIG2] = (si4703_data_registers[SYSCONFIG2] & 0xFFF0) | volume; //set volume
	si4703_push();
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
	si4703_pull();
	si4703_data_registers[CHANNEL] &= 0xFE00; //Clear out the channel bits
	si4703_data_registers[CHANNEL] |= newChannel; //Mask in the new channel
	si4703_data_registers[CHANNEL] |= (1<<TUNE); //Set the TUNE bit to start
	si4703_push();
	
	while( !(si4703_data_registers[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //setting channel complete!
	
	si4703_pull();
	si4703_data_registers[CHANNEL] &= ~(1<<TUNE); //Clear the tune bit after a tune has completed
	si4703_push();
	
	while( (si4703_data_registers[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //wait for the radio to clean up the STC bit
}

uint16_t si4703_getChannel() {
	si4703_pull();
	ret = si4703_data_registers[READCHAN] & 0x03FF; //Mask out everything but the lower 10 bits
	
	//freq(MHz) = 0.200(in USA) * Channel + 87.5MHz
	//x = 0.2 * Chan + 87.5
	ret *= 2; //49 * 2 = 98	 (USA)
	
	ret += 875; //98 + 875 = 973
	return ret;
}

uint8_t si4703_seek(enum DIRECTION dir) {
	si4703_pull();
	si4703_data_registers[POWERCFG] &= ~(1<<SKMODE); //enable wrapping of frequencies
	if (dir == UP) {
		si4703_data_registers[POWERCFG] &= ~(_BV(UP));
	} else {
		si4703_data_registers[POWERCFG] |= _BV(UP);
	}
	si4703_data_registers[POWERCFG] |= (_BV(SEEK)); //start seeking
	
	si4703_push();
	while( !(si4703_data_registers[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //seek complete!
	
	ret = si4703_data_registers[STATUSRSSI] & (1<<SFBL); //Store the value of SFBL
	
	si4703_data_registers[POWERCFG] &= ~(_BV(SEEK)); //stop seeking
	si4703_push();
	while( (si4703_data_registers[STATUSRSSI] & _BV(STC)) ) {
		si4703_pull();
	} //wait for the radio to clean up the STC bit
	
	return ret;
}

void si4703_init() {	
	DDRB |= _BV(4);//Reset is an output
	DDRC |= _BV(4);//SDIO is an output
	//SET_RST_RADIO; //SETH(FM_DDRRESET, FM_RESET);
	PORTC &= ~_BV(4); //SETH(FM_DDRSDIO, FM_SDIO);
	//PORTC |= _BV(5);
	PORTB &= ~_BV(4); //lower the reset pin
	_delay_ms(1);
	//PORTC &= _BV(4); //SETL(FM_PORTSDIO, FM_SDIO);
	PORTB |= _BV(4);  //SETL(FM_PORTRESET, FM_RESET);
	_delay_ms(10);
	
	PORTC |= (_BV(5) | _BV(4));
	_delay_ms(10);
}

/* Call init() first */
void si4703_powerOn() {
	si4703_pull();
	si4703_data_registers[OSCCTRL] = 0x8100;
	si4703_push();
	_delay_ms(500); //Wait for oscillator to settle
	si4703_pull();
	si4703_data_registers[POWERCFG] = 0x4001; //Enable the IC
	si4703_data_registers[SYSCONFIG1] |= _BV(RDS); //Enable RDS
	si4703_data_registers[SYSCONFIG2] &= ~(_BV(SPACE1) | _BV(SPACE0)); //FOrce 200 kHz channel spacing (USA)
	si4703_data_registers[SYSCONFIG2] &= ~(0x000F); //Turn down for what?!?
	//si4703_data_registers[SYSCONFIG2] |= 0x0001; //Lowest volume setting
	si4703_push();
	_delay_ms(110); //Waiting the max powerup time for the radio
}


//The device starts reading at reg_index 0x0A, and has 32 bytes
void si4703_pull() {
	//printf("HI\n");
	i2c_start(SI4703_ADDR | I2C_READ);
	//printf("HELLO\n");
	/*for (reg_index=0x00; reg_index<0x10; reg_index++) {
		printf("?\n");
		pancakes[(reg_index+0x0A) % 0x10] = i2c_readAck() << 8;
		printf("!\n");
		pancakes[(reg_index+0x0A) % 0x10] = i2c_readAck();
	}*/
	for(int x = 0x0A ; ; x++) { //Read in these 32 bytes
		if(x == 0x10) x = 0; //Loop back to zero
		si4703_data_registers[x] = i2c_readAck() << 8;
		si4703_data_registers[x] |= i2c_readAck();
		
		if(x == 0x08) break; //We're [almost] done!
	}
	
	si4703_data_registers[0x09] = i2c_readAck() << 8;
	si4703_data_registers[0x09] |= i2c_readNak();

}


//The device starts writing at reg_index 4, and there are only 12 bytes that are controlling
void si4703_push() {
	
	i2c_start_wait(SI4703_ADDR | I2C_WRITE);
	for (reg_index=0x00; reg_index<0x06; reg_index++) {
		i2c_write(si4703_data_registers[reg_index+2] >> 8);
		i2c_write(si4703_data_registers[reg_index+2] & 0x00FF);
	}
	
	i2c_stop();
}

/* shudder... shudder shudder shudder shudder */
/*
void seek_TWI_devices() {
	uint8_t i = 0; 
	printf("Looking for devices...\n");
	for (i=0; i<=0x7F; i++) {
		if (!i2c_start(i<<1)) {
			printf("There is a Device at 0x%02x", i);
			i2c_stop();
			printf("\n");
		} else {
			printf("0x%02x\n", i);
		}
		_delay_ms(2000);
	}
	printf("Searched for devices 0x00 to 0x7F\n");
} 
*/