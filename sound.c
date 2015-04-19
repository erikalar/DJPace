////PD2 and PD3 is going to pins 4 and 5 of the sound breakout.
#include <avr/io.h>
#include <util/delay.h>


#define SEND          0x80
#define PLAY_PAUSE    0x01
#define STOP          0x02
#define NEXT          0x04
#define VOLUME_UP     0x08
#define VOLUME_DOWN   0x08
#define ADDRESS       0x10


void init_audio(void);
void sound_action(void);
void send_audio_data(void);
/* The audio_mask will get set when relevant information is gathered. The main while loop will
   check the msb, and it will be set if there is data that needs to be sent
   to the sound module.
   bit 0 is play/pause, bit 1 is stop, bit 3 is next.
   bit 7 is send.*/
unsigned char audio_mask; // This mask will determine the action required for sound player
uint16_t audio_data;
uint16_t audio_address;



int main(void)
{
    init_audio();	
	/* main while loop   */
    while (1) {
	//assuming some routine sets the mask when action is required
		if(SEND &= audio_mask)
		{
			sound_action();			
		}

    }

    return 0;   /* never reached */
}

void init_audio()
{
    DDRD |= 1 << PD2;          // Set PORTD bit 2 for output for clock
	DDRD |= 1 << PD3;          // Set PORTD bit 3 for output for data
	
	PORTD |= 1 << PD2;         // Set clock output to high
	PORTD |= 1 << PD3;         // Set data output to high
}


/*This function will look at the mask and determine if the 
audio_data needs to be set. Then it will call the send_audio_data
function */
void sound_action()
{
	if(PLAY_PAUSE &= audio_mask)
	{
		audio_data = 0xfffe;
	}
	else if(STOP &= audio_mask)
	{
		audio_data = 0xffff;
	}	
	else if(NEXT &= audio_mask)
	{
		audio_address++;
		audio_data = audio_address;
	}	
	else if(VOLUME_UP &= audio_mask)
	{
		
	}	
	else if(VOLUME_DOWN &= audio_mask)
	{
		
	}
	else if(ADDRESS &= audio_mask)
	{
		audio_data = audio_address;
	}

	/*  reset mask to 0   */
	audio_mask = 0;
	send_audio_data();
}



void send_audio_data()
{
	/*First lower clock for 2ms and send first bit  */
		/*lowering the clock   */
		PORTD &= ~(1 << PD2);
		/*get first bit (MSB) of data  */
		uint16_t msb = audio_data;
		msb = msb >> 15;
		/*set the data line to MSB  */		
		if(msb)
		{
			PORTD |= 1 << PD3;
		}
		else
		{
			PORTD &= ~(1 << PD3);
		}
		/* clock lowered for 2ms  */
		_delay_ms(2);
		/* clock raised for 200us  */
		PORTD |= 1 << PD2;
		_delay_us(200);
		
	/*Next loop for the next 15 bits of data  */
		int8_t count = 14;
		while (count >= 0)
		{
			/* lower clock  */
			PORTD &= ~(1 << PD2);
			/* get the next significant bit of data  */
			uint16_t next_bit = audio_data;
			next_bit = next_bit >> count;
			next_bit &= 0x01;		
			/*set the data line to next bit of data  */		
			if(next_bit)
			{
				PORTD |= 1 << PD3;
			}
			else
			{
				PORTD &= ~(1 << PD3);
			}
			/* clock lowered for 200us  */
			_delay_us(200);
			/* clock raised for 200us  */
			PORTD |= 1 << PD2;
			_delay_us(200);
			count--;			
		}
	/* reset clock and data lines to high  */
	PORTD |= 1 << PD2;         // Set clock output to high
	PORTD |= 1 << PD3;         // Set data output to high
}



