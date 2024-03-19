/*
 * i2c_DIO_Firmware_v2.0.c
 *ff
 * Created: 19.03.2024 20:58:35
 * Author : Vanya
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "string.h"
#include "stdbool.h"
#include "uart_hal.h"
#include "gpio_driver.h"
#include "twi_hal1.h"
#include "spi1_hall.h"
#include "stdint.h"

#include <stdio.h>
static FILE mystdout = FDEV_SETUP_STREAM((void *)uart_send_byte, NULL, _FDEV_SETUP_WRITE);

gpio drvEn = {(uint8_t *)&PORTD , PORTD7};
gpio ledRun = {(uint8_t *)&PORTD , PORTD6};
gpio ledFail = {(uint8_t *)&PORTD , PORTD5};
gpio devType = {(uint8_t *)&PORTC , PORTC1};

#define SERIALBAUD			9600U
#define TWIADDR				0x54U
#define TWISPEED			400000U
#define TWIBUFFSIZE			8U
#define TIMERPRESET			(65535-781)				//For 100ms interrupt
uint8_t twiDataBuff[TWIBUFFSIZE];
uint16_t ioTimer = 0;	
uint8_t actionRequest;	



ISR(TIMER1_OVF_vect)
{
	actionRequest = 1;
	TCNT1 = TIMERPRESET;
}



int main(void)
{
    sei();
    wdt_enable(WDTO_2S);
	stdout = &mystdout;
	
	gpio_set_pin_direction(&devType , PORT_DIR_IN);

	gpio_set_pin_direction(&drvEn , PORT_DIR_OUT); gpio_set_pin_level(&drvEn, true);
	gpio_set_pin_direction(&ledRun , PORT_DIR_OUT); gpio_set_pin_level(&ledRun, true);
	gpio_set_pin_direction(&ledFail , PORT_DIR_OUT); gpio_set_pin_level(&ledFail, true);
	_delay_ms(100);
	gpio_set_pin_level(&ledRun, false);
	gpio_set_pin_level(&ledFail, true);
	
	
	twi0_slave_init(TWIADDR, &twiDataBuff, TWISPEED);			//Init TWI slave on port TWI0
	//twi0_init(TWISPEED);
	uart_init(SERIALBAUD, 1);


	TCNT1 = TIMERPRESET;
	TCCR1A	= 0x00;
	TCCR1B = (1<<CS10) | (1<<CS12);
	TIMSK1 = (1<<TOIE0);

	 
	sei();
	//uart_send_string((uint8_t *)"Program Start\n\r");		
    while (1) 
    {
		wdt_reset();
		
		if(actionRequest){
			for (int i = 0; i < TWIBUFFSIZE; ++i) {
				printf("%02X ", twiDataBuff[i]);
				if ((i + 1) % 16 == 0);
			}
			printf("; %hu \n\r", ioTimer);
			//twi0_write(0x26, 0x01, &twiDataBuff, 2);
			
			if(twiDataBuff[0] == 0x55){
				ioTimer = twiDataBuff[1];
				ioTimer = (ioTimer << 8) + twiDataBuff[2];
				memset(twiDataBuff, 0, sizeof(twiDataBuff));
			}
			twiDataBuff[1] = (ioTimer >> 8) & 0x00FF;
			twiDataBuff[2] = ioTimer & 0xff;
			
			
			if(ioTimer != 0){
				ioTimer--;
				gpio_set_pin_level(&drvEn, true);
				gpio_toggle_pin_level(&ledRun);
				gpio_set_pin_level(&ledFail, false);
				}else{
				gpio_set_pin_level(&drvEn, false);
				gpio_set_pin_level(&ledRun, false);
				gpio_set_pin_level(&ledFail, true);
			}
			actionRequest = 0;
		}
		
		
		
		_delay_ms(100);
		
    }
	return 0;
}

