/*
 * DomoticsPLC16.c
 *
 * Created: 9/30/2015 10:17:12 PM
 *  Author: Cristian Scarlat
 */ 
#define F_CPU 16000000UL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

// ethernet interf
#include "ip_arp_udp.h"
//#include "ip_arp_udp.c"
#include "enc28j60.h"
//#include "enc28j60.c"
#include "net.h"
#include "timeout.h"
//#include "timeout.c"

#include "hd44780.h"
#include "hd44780_settings.h"
//#include "hd44780.c"

    char StrDisplay[10];
    char strRX[200];
	uint8_t DigIn = 0;//byte in care se citesc intrarile digitale de la MCP23017
	uint8_t ch = 0;// canal ADC
    float tempC;
	float hum;
	float light;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t days;
	uint8_t weekdays = 0;
	uint8_t months;
	uint8_t years;
    uint8_t relays = 0x00;
	uint8_t menu = 0;
    uint8_t submenu = 0;
	//****************************************
	char Val[4];
	uint8_t relay=0;
	uint16_t valON=0;
	uint16_t valOFF =0;
	uint8_t Hon = 0;
	uint8_t Hoff = 0;
	uint8_t Mon = 0;
	uint8_t Moff = 0;
	uint8_t i=0;
	uint8_t j=0;
	uint8_t k=0;
	uint8_t eventsRX=0;//conditii gasite in strRX stringul receptionat de pe UDP
	uint8_t eventsTRUE = 0;// conditii TRUE , indeplinite pentru a comuta releul
	uint8_t OR = 0;
	uint8_t Validare[4];
	uint8_t flags;
	//***************************************
	
        // keep a track of number of overflows
        uint8_t refresh;
	
        uint16_t plen;
        uint8_t payloadlen=0;
	
// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
static uint8_t myip[4] = {192,168,0,10};
static uint16_t myport =1200; // listen port for udp
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX	
#define BUFFER_SIZE 250
static uint8_t buf[BUFFER_SIZE+1];
	

void TWI_Init()
{
	// 100kHz...SCL
	TWBR = 0x48;							//Set bitrate factor to 0
	TWSR &= ~((1<<TWPS1) | (1<<TWPS0));				//Set prescaler to 1
}

void TWI_Start()
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTA);
	while (!(TWCR & (1<<TWINT)));
}

void TWI_Stop()
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	while ((TWCR & (1<<TWSTO)));
}

uint8_t TWI_Read(uint8_t ack)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (((ack ? 1 : 0)<<TWEA));
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}


void TWI_Write(uint8_t byte)
{
	TWDR = byte;
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}
 

void init(void)
{
    DDRB &= ~((1 << 0)|(1 << 1)|(1 << 2)|(1 << 3));
////////////////////////Timer 0 and Interupt //////////////////////////////////////
   // set up timer with prescaler = 1024
    TCCR0 |= (1 << CS02)|(1 << CS00);
  //OCR set for 10ms
    OCR0 = 0x9B;
    // enable compare  interrupt
   TIMSK |= (1 << OCIE0);
    // enable global interrupts
    sei();
  
    // initialize overflow counter variable
    refresh = 0;
///////////////////////////////////////////////////////////////
	lcd_init();
	lcd_clrscr();
	lcd_puts("PLC FOR DOMOTICS");
	lcd_goto(0x40);
	lcd_puts("V1.0_10_2015");
	_delay_ms(1000);
	lcd_clrscr();
	lcd_puts("Author:");
	lcd_goto(0x40);
	lcd_puts("Cristian Scarlat");
	_delay_ms(1000);
        lcd_clrscr();
        _delay_ms(100);
	 TWI_Init();
         //PCF8563 Status and control registers write
	 TWI_Start();
         TWI_Write(0xA2);
	 TWI_Write(0x00);
	 TWI_Write(0x00);
         TWI_Stop();
         TWI_Start();
         TWI_Write(0xA2);
         TWI_Write(0x01);
	 TWI_Write(0x00);
	 TWI_Stop();
	 //MCP23017 init
         TWI_Start();
	 TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	 TWI_Write(0x00); //MCP23017 IODIRA register address
	 TWI_Write(0x00); //MCP23017 PORTA set to output
	 TWI_Stop();
	 TWI_Start();
	 TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	 TWI_Write(0x12); //MCP23017 GPIOA register address
	 TWI_Write(0x00); //MCP23017 reset port GPA
	 TWI_Stop();
	 TWI_Start();
	 TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	 TWI_Write(0x01); //MCP23017 IODIRB register address
	 TWI_Write(0xFF); //MCP23017 PORTB set to input
	 TWI_Stop();
	 //MCP23017 Read PortB*******************************************
	 TWI_Start();
	 TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	 TWI_Write(0x13); //MCP23017 GPIOB register address
	 TWI_Stop();
	 TWI_Start();
	 TWI_Write(0x41); //MCP23017 address A0=0,A1=0,A2=0 , Read mode
	 DigIn = TWI_Read(0);
	 TWI_Stop();
	 //***************************************************************
	 _delay_ms(100);
      //Analog inputs init
		//ADMUX = 1<<REFS0;
		ADMUX &= ~((1<<REFS0)|(1<<REFS1));//AREF,Internal Vreff turned off
		ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
		//while(ADCSRA & (1<<ADSC));
        //*****************************************************************
	i=0;
	for(i=0;i<200;i++){
			strRX[i] = 0;
			}
	i=0;
	eeprom_read_block((char*)strRX,(void*)0,200);
	 /*initialize enc28j60*/
        enc28j60Init(mymac);
        delay_ms(20);

        
        /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);
        delay_ms(20);

        //init the ethernet/ip layer:
        init_ip_arp_udp(mymac,myip);
}

void RelayToggle(uint8_t RelayNo)
{
	TWI_Start();
	TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	TWI_Write(0x12); //MCP23017 GPIOA register address
	relays ^= 1<<RelayNo;
	TWI_Write(relays); //MCP23017 set GPA0
	TWI_Stop();
}

void RelayON(uint8_t RelayNo)
{
	TWI_Start();
	TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	TWI_Write(0x12); //MCP23017 GPIOA register address
	relays |= (1<<RelayNo);
	TWI_Write(relays); //MCP23017 set GPA0
	TWI_Stop();
}

void RelayOFF(uint8_t RelayNo)
{
	TWI_Start();
	TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	TWI_Write(0x12); //MCP23017 GPIOA register address
	relays &= ~(1<<RelayNo);
	TWI_Write(relays); //MCP23017 set GPA0
	TWI_Stop();
}

void ReadDigIn(void)
{
	//MCP23017 Read PortB*******************************************
	 TWI_Start();
	 TWI_Write(0x40); //MCP23017 address A0=0,A1=0,A2=0 , Write mode
	 TWI_Write(0x13); //MCP23017 GPIOB register address
	 TWI_Stop();
	 TWI_Start();
	 TWI_Write(0x41); //MCP23017 address A0=0,A1=0,A2=0 , Read mode
	 DigIn = TWI_Read(0);
	 TWI_Stop();
	 //Revers order of bits******************************************
	 DigIn = (DigIn & 0xF0) >> 4 | (DigIn & 0x0F) << 4;
	 DigIn = (DigIn & 0xCC) >> 2 | (DigIn & 0x33) << 2;
	 DigIn = (DigIn & 0xAA) >> 1 | (DigIn & 0x55) << 1;
	 //***************************************************************
         DigIn = ~DigIn;//Operatie de NOT pentru ca iesirea optocuploarelor este cu colector in gol
}

void readtemp(void)
{               //set channel 1
	        ADMUX |= (1<<MUX0);
		ADMUX &= ~((1<<MUX1)|(1<<MUX2)|(1<<MUX3)|(1<<MUX4));
		// start single convertion
		// write ’1? to ADSC
		ADCSRA |= (1<<ADSC);
		// wait for conversion to complete
		// ADSC becomes ’0? again
		// till then, run loop continuously
		while(ADCSRA & (1<<ADSC));
		float Volt = (ADC*5)/1023.0;
		      tempC = (Volt-0.5)*100;
		dtostrf(tempC,5,1,StrDisplay);	
}

void readhum(void)
{        
          //set channel 2
	        ADMUX |= (1<<MUX1);
		ADMUX &= ~((1<<MUX0)|(1<<MUX2)|(1<<MUX3)|(1<<MUX4));
		// start single convertion
		// write ’1? to ADSC
		ADCSRA |= (1<<ADSC);
		// wait for conversion to complete
		// ADSC becomes ’0? again
		// till then, run loop continuously
		while(ADCSRA & (1<<ADSC));
		float Volt = (ADC*5.0)/1023.0;
		float  SensorRH = (Volt- 0.958) / 0.0307;
                 hum = SensorRH / (1.0546 - 0.00216 * tempC);
		dtostrf(hum,5,1,StrDisplay);	
}

uint16_t readlight()
{                //set channel 3
	        ADMUX |= (1<<MUX0)|(1<<MUX1);
		ADMUX &= ~((1<<MUX2)|(1<<MUX3)|(1<<MUX4));
		// start single convertion
		// write ’1? to ADSC
		ADCSRA |= (1<<ADSC);
		// wait for conversion to complete
		// ADSC becomes ’0? again
		// till then, run loop continuously
		while(ADCSRA & (1<<ADSC));
		light = ADC;
		return(light);
}
uint16_t readan (uint8_t ch)
{       ADMUX &= ~((1<<MUX0)|(1<<MUX1)|(1<<MUX2)|(1<<MUX3)|(1<<MUX4));
	switch(ch){
		case 1:
			ADMUX |= (1<<MUX0)|(1<<MUX1)|(1<<MUX2); 
			break;
		case 2:
			ADMUX |= (1<<MUX1)|(1<<MUX2); 
			break;
		case 3:
			ADMUX |= (1<<MUX0)|(1<<MUX2); 
			break;
		case 4:
			ADMUX |= 1<<MUX2; 
			break;
	}
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
        uint16_t Val = ADC;
	return(Val);
}

// Converts a BCD (binary coded decimal) to decimal
uint8_t BcdToDec(uint8_t value)
{
  return ((value / 16) * 10 + value % 16);
}

// Converts a decimal to BCD (binary coded decimal)
uint8_t DecToBcd(uint8_t value){
  return ((value / 10) * 16 + value % 10);
}

void ReadRTC(void)
{
	 //Citire PCF8563 ********************************
	 TWI_Start();
	 TWI_Write(0xA2); //I2C Address pentru scriere
	 TWI_Write(0x02); //pointer la registru Seconds
	 TWI_Stop();
	 TWI_Start();
	 TWI_Write(0xA3); //I2C Address pentru citire *** adresele registrelor sunt incrementate automat (datasheet)
	 seconds = TWI_Read(1);//generate akc bit
	 minutes = TWI_Read(1);
	 hours = TWI_Read(1);
	 days = TWI_Read(1);
	 weekdays = TWI_Read(1);
	 months = TWI_Read(1);
	 years = TWI_Read(0); //do not generate akc bit
	 TWI_Stop();
	 //***********************************************
         seconds &= ~(1<<7);
         minutes &= ~(1<<7);
         hours &= ~((1<<7)|(1<<6));
         days &= ~((1<<7)|(1<<6));
         months &= ~((1<<7)|(1<<6)|(1<<5));
         seconds = BcdToDec(seconds);
         minutes = BcdToDec(minutes);
         hours = BcdToDec(hours);
         days = BcdToDec(days);
         months = BcdToDec(months);
         years = BcdToDec(years);
         
}
void WriteRTC(void)
{
 ////////////////convert Decimal values to BCD for PCF8563///////////////////
uint8_t setS = DecToBcd(seconds);
uint8_t setMin = DecToBcd(minutes);
uint8_t setH = DecToBcd(hours);
uint8_t setD = DecToBcd(days);
uint8_t setMon = DecToBcd(months);
uint8_t setY = DecToBcd(years);
////////////////////////////////////////////////////////////////////////////
	 TWI_Start();
	 TWI_Write(0xA2);//PCF8563 write I2C address
	 TWI_Write(0x02);// registru secunde, apoi valoarea registrilor este incrementata automat
	 TWI_Write(setS);
	 TWI_Write(setMin);
	 TWI_Write(setH);
	 TWI_Write(setD);
	 TWI_Write(weekdays);
	 TWI_Write(setMon);
	 TWI_Write(setY);
	 TWI_Stop();
}

void displayTime(){
                     lcd_clrscr();
            	     //lcd_goto(0);
                     if(hours < 10) lcd_puts("0");//afiseaza un "0" la zecimale daca ora<10
                     lcd_puts(itoa(hours,StrDisplay,10));// converteste integerul in sring si afiseaza pe LCD

                     lcd_puts(":");
                     if(minutes < 10)lcd_puts("0");
                     lcd_puts(itoa(minutes,StrDisplay,10));

                     //lcd_puts(":");
                     //if(seconds < 10)lcd_puts("0");
                     //lcd_puts(itoa(seconds,StrDisplay,10));

                     lcd_puts(" Date:");

                     if(days < 10)lcd_puts("0");
                     lcd_puts(itoa(days,StrDisplay,10));
                     lcd_puts("/");

                   //lcd_puts(itoa(weekdays,StrDisplay,10));
                   //StrDisplay[] = 0;
                   //lcd_puts("/");
                     if(months < 10)lcd_puts("0");	
                     lcd_puts(itoa(months,StrDisplay,10));

                     lcd_puts("/");
                     lcd_puts(itoa(years,StrDisplay,10));
}

void setTime(void){
        
        lcd_command(0x0E);//Cursor on blinking

 while(menu == 1){
        _delay_ms(100);
	displayTime();
	lcd_goto(0x40);//second line
	lcd_puts("IP:");
	lcd_goto(44);
	lcd_puts(itoa(myip[0],StrDisplay,10));
	lcd_puts(".");
	lcd_puts(itoa(myip[1],StrDisplay,10));
	lcd_puts(".");
	lcd_puts(itoa(myip[2],StrDisplay,10));
	lcd_puts(".");
	lcd_puts(itoa(myip[3],StrDisplay,10));
	lcd_goto(0);
	if(!(PINB & (1 << PINB1))){
		//init the ethernet/ip layer:
		init_ip_arp_udp(mymac,myip);
		lcd_command(0x0C);//Cursor off
		menu = 0;
		submenu = 0;
	}
	if(submenu == 0){
                 lcd_goto(0);
		if(!(PINB & (1 << PINB3))){
			_delay_ms(100);
			hours++;
			if(hours>23)hours=0;
		}
		if(!(PINB & (1 << PINB0))){
			_delay_ms(100);
			hours--;
			if(hours == 255)hours=23;
		}
		if(!(PINB & (1 << PINB2))){
                          while(!(PINB & (1 << PINB2)));
                         _delay_ms(100);
                        submenu++;				
                           }
	}
        if(submenu == 1){
                 lcd_goto(3);  
		if(!(PINB & (1 << PINB3))){
			_delay_ms(100);
			minutes++;
			if(minutes > 59)minutes=0;
		}
		if(!(PINB & (1 << PINB0))){
			_delay_ms(100);
			minutes--;
			if(minutes == 255)minutes=59;
		}
		if(!(PINB & (1 << PINB2))){
                          while(!(PINB & (1 << PINB2)));
                         _delay_ms(100);
                        submenu++;				
                           }
	}
        if(submenu == 2){
                 lcd_goto(11);
		if(!(PINB & (1 << PINB3))){
			_delay_ms(100);
			 days++;
			if(days > 31)days=0;				
		}
		if(!(PINB & (1 << PINB0))){
			_delay_ms(100);
			days--;
			if(days == 255)days=31;
		}
		if(!(PINB & (1 << PINB2))){
                          while(!(PINB & (1 << PINB2)));
                         _delay_ms(100);
                        submenu++;
                           }
	}
	if(submenu == 3){
                 lcd_goto(14);
		if(!(PINB & (1 << PINB3))){
			_delay_ms(100);
			 months++;
			if(months > 12)months=0;
		}
		if(!(PINB & (1 << PINB0))){
			_delay_ms(100);
			months--;
			if(months == 255)months=12;
		}
		if(!(PINB & (1 << PINB2))){
                          while(!(PINB & (1 << PINB2)));
                         _delay_ms(100);
                        submenu++;				
                           }
	}
	if(submenu == 4){
                 lcd_goto(17);
		if(!(PINB & (1 << PINB3))){
			_delay_ms(100);
			 years++;
			if(years > 99)years=0;
		}
		if(!(PINB & (1 << PINB0))){
			_delay_ms(100);
			years--;
			if(years == 255)years=99;
		}
		if(!(PINB & (1 << PINB2))){
                          while(!(PINB & (1 << PINB2)));
                         _delay_ms(100);
                        submenu++;				
                           }
	}
	if(submenu == 5){

			  //uint8_t myip[4] = {192,168,0,10};
				lcd_goto(45); 
				lcd_command(0x0E);//Cursor on blinking 		
			  if(!(PINB & (1 << PINB3))){
				  _delay_ms(100);
				  myip[0]++;

			  }
			  if(!(PINB & (1 << PINB0))){
				  _delay_ms(100);
				  myip[0]--;

			  }
			  if(!(PINB & (1 << PINB2))){
				  while(!(PINB & (1 << PINB2)));
				  _delay_ms(100);
				  submenu++;
			  }
	}
	if(submenu == 6){

			//uint8_t myip[4] = {192,168,0,10};
			lcd_goto(0x49);//second line

			if(!(PINB & (1 << PINB3))){
				_delay_ms(100);
				myip[1]++;

			}
			if(!(PINB & (1 << PINB0))){
				_delay_ms(100);
				myip[1]--;

			}
			if(!(PINB & (1 << PINB2))){
				while(!(PINB & (1 << PINB2)));
				_delay_ms(100);
				submenu++;
			}
		}
	if(submenu == 7){

				//uint8_t myip[4] = {192,168,0,10};
				lcd_goto(0x4C);//second line

				if(!(PINB & (1 << PINB3))){
					_delay_ms(100);
					myip[2]++;

				}
				if(!(PINB & (1 << PINB0))){
					_delay_ms(100);
					myip[2]--;

				}
				if(!(PINB & (1 << PINB2))){
					while(!(PINB & (1 << PINB2)));
					_delay_ms(100);
					submenu++;
				}
			}
	if(submenu == 8){

					//uint8_t myip[4] = {192,168,0,10};
					lcd_goto(0x50);//second line

					if(!(PINB & (1 << PINB3))){
						_delay_ms(100);
						myip[3]++;

					}
					if(!(PINB & (1 << PINB0))){
						_delay_ms(100);
						myip[3]--;

					}
					if(!(PINB & (1 << PINB2))){
						while(!(PINB & (1 << PINB2)));
						_delay_ms(100);
						submenu++;
					}
				}
		if(submenu == 9){
						 WriteRTC();
						 lcd_command(0x0C);//Cursor off
						 lcd_clrscr();
						 menu = 0;
						submenu = 0;
		}
  }
}
void EventCheck()// algoritm de interpretare a programului recep?ionat de la interfa?a software de programare
{	
	ReadDigIn();
	
	
	uint8_t len = strlen(strRX);

		for(i=0;i<len;i++)
			{
				if(strRX[i]=='r')
				{
					Validare[0] = 0;
					Validare[1] = 0;
					Validare[2] = 0;
					Validare[3] = 0;
					OR = 0;
					relay=0;
					switch(strRX[i+1]){
						case '0' :
							relay=0;
							break;
						case '1' :
							relay=1;
							break;
						case '2' :
							relay=2;
							break;
						case '3' :
							relay=3;
							break;
						case '4' :
							relay=4;
							break;
						case '5' :
							relay=5;
							break;
						case '6' :
							relay=6;
							break;
						case '7' :
							relay=7;
							break;
						
					}
						
				}
				if(strRX[i]=='d') //daca 'd' 
				{	eventsRX++;	   //incrementeaz? valoarea variabilei în care se re?ine num?rul condi?iilor întâlnite în program	
					if(((DigIn >> atoi((char*)&strRX[i+1])) & 1))//testeaz? intrarea digital? indicat? de caracterul aflat dup? ”d”
						{
							eventsTRUE++;//incrementeaz? valoarea variabilei în care se re?ine num?rul condi?iilor adev?rate

						}

				}
				if(strRX[i]=='a')  //daca 'a'
				{
					eventsRX++;
					for(k=0;k<4;k++)
					{
						Val[k]=0;
					}
					k=0;
					if(strRX[i+1]=='1')ch=1;
					else if(strRX[i+1]=='2')ch=2;
					else if(strRX[i+1]=='3')ch=3;
					else if(strRX[i+1]=='4')ch=4;
						for(j=i+2;strRX[j]!='-';j++)  //porneste dupa i+2, dupa i este canalul analogic
						{
							Val[k] = strRX[j];
							k++;
						}
						valON = atoi(Val);
						
						k=0;
						for(j=j+1;strRX[j]!='-';j++)  //porneste dupa '-', valOFF
						{
							Val[k] = strRX[j];
							k++;
						}
						valOFF = atoi(Val);
						
						if(readan(ch)>=valON&&readan(ch)<=valOFF)eventsTRUE++;
						
							
				}
				if(strRX[i]=='t')  //daca 't'
				{
					eventsRX++;
						for(k=0;k<4;k++)
						{
							Val[k]=0;
						}
						k=0;
						for(j=i+1;strRX[j]!='-';j++)  //porneste dupa i+1, dupa i este ValON temperatura
						{
							Val[k] = strRX[j];
							k++;
						}
						valON = atoi(Val);
						
						k=0;
						for(j=j+1;strRX[j]!='-';j++)  //porneste dupa '-', valOFF
						{
							Val[k] = strRX[j];
							k++;
						}
						valOFF = atoi(Val);
					
						if(tempC>=valON&&tempC<=valOFF)eventsTRUE++;
							
				}
				if(strRX[i]=='l')  //daca 'l'
				{
					eventsRX++;
					for(k=0;k<4;k++)
					{
						Val[k]=0;
					}
					k=0;
					for(j=i+1;strRX[j]!='-';j++)  //porneste dupa i+1, dupa i este ValON temperatura
					{
						Val[k] = strRX[j];
						k++;
					}
					valON = atoi(Val);
					
					k=0;
					for(j=j+1;strRX[j]!='-';j++)  //porneste dupa '-', valOFF
					{
						Val[k] = strRX[j];
						k++;
					}
					valOFF = atoi(Val);
					
					if(light>=valON&&light<=valOFF)eventsTRUE++;
					
				}
				if(strRX[i]=='h')  //daca 'h'
				{
					eventsRX++;
					for(k=0;k<4;k++)
					{
						Val[k]=0;
					}
					k=0;
					for(j=i+1;strRX[j]!='-';j++)  //porneste dupa i+1, dupa i este ValON temperatura
					{
						Val[k] = strRX[j];
						k++;
					}
					valON = atoi(Val);
					
					k=0;
					for(j=j+1;strRX[j]!='-';j++)  //porneste dupa '-', valOFF
					{
						Val[k] = strRX[j];
						k++;
					}
					valOFF = atoi(Val);
					
					if(hum>=valON&&hum<=valOFF)eventsTRUE++;
					
				}
				if(strRX[i]=='c')  //daca 'c'
				{
					eventsRX++;
					for(k=0;k<4;k++)
					{
						Val[k]=0;
					}
					k=0;
					for(j=i+1;strRX[j]!='-';j++)  //porneste dupa i+1, dupa i este ValON 
					{
						Val[k] = strRX[j];
						k++;
					}
					Hon = atoi(Val);
					
					k=0;
					for(j=j+1;strRX[j]!='-';j++)  //porneste dupa '-'
					{
						Val[k] = strRX[j];
						k++;
					}
					Hoff = atoi(Val);
					k=0;
					for(j=j+1;strRX[j]!='-';j++)  
					{
						Val[k] = strRX[j];
						k++;
					}
					Mon = atoi(Val);
					
					k=0;
					for(j=j+1;strRX[j]!='-';j++)  //porneste dupa '-'
					{
						Val[k] = strRX[j];
						k++;
					}
					Moff = atoi(Val);
					
					if(hours == Hon && minutes == Mon)flags |= 1<<2;
					if(hours == Hoff && minutes == Moff)flags &= ~(1<<2);
					if((flags >> 2) & 1) eventsTRUE++;
					//if(!((flags >> 1) & 1))continue;
				}
				if(strRX[i]=='v')	
				{
					switch(strRX[i+1]){
						case '0' :
							OR=0;
							break;
						case '1' :
							OR=1;
							break;
						case '2' :
							OR=2;
							break;
						case '3' :
							OR=3;
							break;
					}
					if(eventsRX == eventsTRUE)
					{
						Validare[OR] = 1;
						
					}
					eventsRX=0;
					eventsTRUE=0;
					
				}
				if(strRX[i]=='e')	
				{
					if(Validare[0] == 1 || Validare[1] == 1 || Validare[2] == 1 || Validare[3] == 1)
					{
						RelayON(relay);
						
					}
					else RelayOFF(relay);
					
				}
				
			}
		
	}


int main()
{    
	init();
	while(1)
	{
		if(menu == 1) setTime();
	
		if(menu == 0)
			{

					if(refresh>=50 && !((flags >> 0) & 1)){
						 refresh = 0;
						ReadRTC();
						displayTime(); 
						lcd_goto(0x40);
						lcd_puts("Temp:");
						readtemp();
						//dtostrf(readan(2),5,1,StrDisplay);
						lcd_puts(StrDisplay);
						lcd_puts("C ");
						lcd_puts("Hum:");
						readhum();
						lcd_puts(StrDisplay);
						lcd_puts("% "); 
						readlight();                           
						 } 
					if(refresh>=30 && (flags >> 0) & 1){
						refresh = 0;
						lcd_clrscr();
						float Volt = (readan(1)*5.0)/1023.0;
						dtostrf(Volt,3,1,StrDisplay);
						lcd_puts("A1:");
						lcd_puts(StrDisplay);
						lcd_puts("V");
						Volt = (readan(2)*5.0)/1023.0;
						dtostrf(Volt,3,1,StrDisplay);
						lcd_puts(" A2:");
						lcd_puts(StrDisplay);
						lcd_puts("V");
						Volt = (readan(3)*5.0)/1023.0;
						dtostrf(Volt,3,1,StrDisplay);
						lcd_puts(" A3:");
						lcd_puts(StrDisplay);
						lcd_puts("V");
						Volt = (readan(4)*5.0)/1023.0;
						dtostrf(Volt,3,1,StrDisplay);
						lcd_goto(0x40);
						lcd_puts("A4:");
						lcd_puts(StrDisplay);
						lcd_puts("V");
						int light = itoa(readlight(),StrDisplay,10);
						lcd_puts("  Lumina: ");
						lcd_puts(light);
						}
					
			if(!(PINB & (1 << PINB2)))
				{
                                 _delay_ms(100);
                                menu = 1;				
				}
			if(!(PINB & (1 << PINB1)))
				{
                                 _delay_ms(100);
                                menu = 0;				
				}
			if(!(PINB & (1 << PINB3)))
				{
					flags |= 1<<0;
					_delay_ms(100);
				}
			if(!(PINB & (1 << PINB1)))
			{
				flags &= ~(1<<0);
				_delay_ms(100);
			}
					   
		EventCheck();
	}
	   // get the next new packet:
                plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

                /*plen will ne unequal to zero if there is a valid 
                 * packet (without crc error) */
                if(plen==0)
			{
				continue;
			}
                // arp is broadcast if unknown but a host may also
                // verify the mac address by sending it to 
                // a unicast address.
                if(eth_type_is_arp_and_my_ip(buf,plen))
			{
				make_arp_answer_from_request(buf,plen);
				continue;
			}
                // check if ip packets (icmp or udp) are for us:
                if(eth_type_is_ip_and_my_ip(buf,plen)==0)
			{
				continue;
			}
                

                        
                if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V)
			{
				// a ping packet, let's send pong
				make_echo_reply_from_request(buf,plen);
				continue;
			}
                if (buf[IP_PROTO_P]==IP_PROTO_UDP_V)
			{
				// payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
				// you must sent a string starting with v
				// e.g udpcom version 10.0.0.24
				for(int i=0;i<200;i++)
					{
						strRX[i] = 0;
					}
	//Decodificare mesaj rx UDP and write to EEPROM*****************************************************
				strcpy(strRX,(char*)&buf[UDP_DATA_P]);// copie in str mesajul string primit pe UDP
				if(strncmp("conf",strRX,4)==0)// daca primul din mesaj este 'conf', atunci copiaza stringul in eeprom si in functia Eventcheck incepe decodificarea
					{
						strcpy(strRX,(char*)&strRX[4]);//'conf' este eliminat
						eeprom_update_block(&strRX,(void*)0,200);
					}
				if(strncmp("cmd",strRX,3)==0)
					{
						strcpy(strRX,(char*)&strRX[3]);//'cmd' este eliminat
							switch(strRX[0]){	
								case '0' :
									RelayToggle(0);
									break;	
								case '1' :
									RelayToggle(1);
									break;
								case '2' :
									RelayToggle(2);
									break;
								case '3' :
									RelayToggle(3);
									break;
								case '4' :
									RelayToggle(4);
									break;
								case '5' :
									RelayToggle(5);
									break;
								case '6' :
									RelayToggle(6);
									break;
								case '7' :
									RelayToggle(7);
									break;
								}
							
						for(i=0;i<200;i++)
							{
								strRX[i] = 0;
							}
						i=0;
						eeprom_read_block((char*)strRX,(void*)0,200);			
					}
			}	
	//******************************************************************************		
			
                        	make_udp_reply_from_request(buf,strRX,strnlen(strRX,200),myport);
				
	}
}

ISR(TIMER0_COMP_vect)
{
	TCNT0 = 0;
	refresh++;

}

