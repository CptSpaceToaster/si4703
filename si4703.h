/*
 * si4703 library
 * 
 * Authored by CptSpaceToaster 10/21/2014
 *
 * References: 
 * Programming Guide: http://www.silabs.com/support%20documents/technicaldocs/AN230.pdf
 * Datasheet: https://www.sparkfun.com/datasheets/BreakoutBoards/Si4702-03-C19-1.pdf
 */

#ifndef SI4703_H
#define SI4703_H

//definitions
#define SI4703_ADDR (0x10<<1) //device address shifted over one

//functions
void si4703_init(void);
void seek_TWI_devices(void);

#endif //SI4703_H
