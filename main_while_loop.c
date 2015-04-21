#include <avr/io.h>
#include <util/delay.h>

/***********************audio stuff ******************************************************/
#define SEND          0x80
#define PLAY_PAUSE    0x01
#define STOP          0x02
#define NEXT          0x04
#define VOLUME_UP     0x08
#define VOLUME_DOWN   0x08
#define ADDRESS       0x10
void init_audio(void);
void send_audio_data();

uint16_t audio_data;
uint16_t audio_address;
uint16_t volume;

/*******************************************************************************************/
void initialize_routine(void);
void calculate_tempo(void);
void select_song(void);

/***************calculating tempo**********************************/
#define MAX          0x08 
#define FILL_ARRAY   0x80
#define PULSE        0x01
/* for pulse_mask, set bit 0 to indicate a pulse, 
   bit 7 is when we need to get interval and enter it into the array*/
unsigned char pulse_mask; ///bits set for interrupt routine to read
unsigned char intervals[MAX]; ///for calculating average
unsigned char last_pulse_time; ///for calculating interval
unsigned char tempo; ///storing tempo of song
 



/************  states  *****************/
enum state {BEGIN, CALC_INTERVALS, CALC_TEMPO, PLAYING, PAUSED, STOP};
enum state current_state;
unsigned char next_action;
/////////////////actions///////////////
#define RESET        0x80
#define START        0x01  
#define I_MAX        0x02  
#define PLAY         0x04  
#define PAUSE		 0x08
#define STOP         0x10


int main(void)
{
    /* *********************INTERRUPTS***************************************
       The board has a 9.8304 MHz clock.  We want the timer to
       interrupt every 50ms (20 Hz) so we need to count clocks to
       9.8304MHz/20Hz = 491,520.  This is too big for the 16 bit counter
       register so use the prescaler to divide the clock by 256 and then
       count that clock to 1,920.   20 times per second.
    */
    // Reset clears register bits to zero so only set the 1's
    TCCR1B |= (1 << WGM12);     // Set for CTC mode.  OCR1A = modulus
    TIMSK1 |= (1 << OCIE1A);    // Enable CTC interrupt
    sei();                      // Enable global interrupts
    OCR1A = 1920;              // Set the counter modulus
    TCCR1B |= (1 << CS12);      // Set prescaler for divide by 256,
                                // also starts timer
	
	
    initialize_routine();


	
	/* main while loop   */
    while (1) {
	
		if(next_action &= RESET)
		{
			current_state = BEGIN;	
			next_action = 0x00;
			initialize_routine();			
		}
		
		else if(current_state == BEGIN && next_action &= START  )
		{
			current_state = CALC_INTERVALS;	
			next_action = 0x00;
			/*set the FILL_ARRAY bit and let the interrupt routine fill the array  */
			pulse_mask |= FILL_ARRAY;
		}
		
		else if(current_state == CALC_INTERVALS && next_action &= I_MAX)
		{
			current_state = CALC_TEMPO;	
			next_action = 0x00;
			calculate_tempo();
			select_song();
			audio_data = audio_address;
			send_audio_data();		
		}
		
		else if(current_state == CALC_TEMPO && next_action &= PLAY)
		{
			current_state = PLAYING ;	
			next_action = 0x00;
			audio_data = 0xfffe;
			send_audio_data();
		}
		
		else if(current_state == PLAYING && next_action &= PAUSE)
		{
			current_state = PAUSED ;	
			next_action = 0x00;
			audio_data = 0xfffe;
			send_audio_data();
		}
		
		else if(current_state == PAUSED && next_action &= PLAY)
		{
			current_state = PLAYING ;	
			next_action = 0x00;
			audio_data = 0xfffe;
			send_audio_data();
		}
		
		else if(current_state == PLAYING && next_action &= STOP)
		{
			current_state = BEGIN ;	
			next_action = 0x00;
			audio_data = 0xffff;
			send_audio_data();
			initialize_routine();
		}	
			
		else if(current_state == PAUSED && next_action &= STOP)
		{
			current_state = BEGIN ;	
			next_action = 0x00;
			initialize_routine();
		}		
    }

    return 0;   /* never reached */
}

void 
initialize_routine(void)
{
	
	next_action = 0x00;
	pulse_mask = 0x00;
	init_audio();
	state current_state = BEGIN;
}

void calculate_tempo()
{
	uint8_t sum = 0;
	uint8_t i = 0;
	while(i<MAX)
	{
		sum += intervals[i];
	}
	uint16_t average = sum/MAX;
	/*average of how many 1/20 of a second are in an interval. 
	To get the BPM, average*60/20. */
	tempo = average*3;
	
}

void select_song()
{
	if(tempo <= 100)
	{
		audio_address = 0x0000;
	}
	else if(100 < tempo < 130)
	{
		audio_address = 0x0001;
	}
	else if(tempo >= 130)
	{
		audio_address = 0x0010;
	}
}





/*************Timer interrupt routine *****************************/
ISR(TIMER1_COMPA_vect)
{
	static uint8_t i = 0;
	
	cnt++; //update counter

	uint8_t interval; //temp register for holding interval
	/////if PULSE bit is set, reset last_pulse_time
	if(pulse_mask & PULSE == PULSE)
	{
		///////////if FILL_ARRAY bit is set, enter interval in array.
		if(pulse_mask & FILL_ARRAY == FILL_ARRAY)
		{
			if(cnt > last_pulse_time)
			{
				interval = cnt - last_pulse_time;
			}
			else
			{
				interval = 0xff - last_pulse_time + cnt;
			}
			intervals[i] = interval;
			i++;
			if(i == MAX)
			{
				i=0;
				pulse_mask &= ~(FILL_ARRAY);
				next_action = I_MAX;
			}
		}
		last_pulse_time = cnt;
		pulse_mask &= ~(PULSE);
	}  
}


/*********************audio stuff *********************************/

void init_audio()
{
    DDRD |= 1 << PD2;          // Set PORTD bit 2 for output for clock
	DDRD |= 1 << PD3;          // Set PORTD bit 3 for output for data
	
	PORTD |= 1 << PD2;         // Set clock output to high
	PORTD |= 1 << PD3;         // Set data output to high
	

	audio_data = 0;
	audio_address = 0;
	last_pulse_time = 0;
	tempo = 0;
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
