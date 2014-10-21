/*
 * si4703.c
 *
 * Created: 10/18/2014 3:30:02 PM
 *  Author: CaptainSpaceToaster
 */ 

#include <stdio.h>
#include "si4703.h"
#include "../twi/i2cmaster.h"
#include "util/delay.h"

void si4703_init() {
	i2c_start_wait(SI4703_ADDR || I2C_WRITE);
}


void seek_TWI_devices() {
	uint8_t i = 0; 
	printf("Looking for devices...\n");
	for (i=0; i<0x7F; i++) {
		if (!i2c_start(i<<1)) {
			printf("There is a Device at 0x%02x", i);
			i2c_stop();
			printf("\n");
		}
		_delay_ms(5);
	}
	printf("Searched for devices 0x00 to 0x7F\n");
}