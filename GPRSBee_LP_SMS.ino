#include <avr/sleep.h>
#include <avr/power.h>
#include <Wire.h>
#include <DS3231.h>

#include <GPRSbee.h>
#include "Diag.h"
#include <String.h>

//sleeping time
#define SLPNG           60
#define BEE_PWRPIN      5
#define GPRSBEE_PWRPIN  7
#define XBEECTS_PIN     8
#define DIAGPORT_RX     4
#define DIAGPORT_TX     3

//#########       diag      #############
#ifdef ENABLE_DIAG
#if defined(UBRRH) || defined(UBRR0H)
// There probably is no other Serial port that we can use
// Use a Software Serial instead
#include <SoftwareSerial.h>
SoftwareSerial diagport(DIAGPORT_RX, DIAGPORT_TX);
#else
#define diagport Serial;
#endif
#endif

// Fill in your mobile number here. Start with + and country code
#define TELNO      "+393898896252"

//The following code is taken from sleep.h as Arduino Software v22 (avrgcc), in w32 does not have the latest sleep.h file
#define sleep_bod_disable() \
{ \
  uint8_t tempreg; \
  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t" \
                       "ori %[tempreg], %[bods_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" "\n\t" \
                       "andi %[tempreg], %[not_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" \
                       : [tempreg] "=&d" (tempreg) \
                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR), \
                         [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)), \
                         [not_bodse] "i" (~_BV(BODSE))); \
}


DS3231 RTC; //Create RTC object for DS3231 RTC come Temp Sensor 
char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
//year, month, date, hour, min, sec and week-day(starts from 0 and goes to 6)
//writing any non-existent time-data may interfere with normal operation of the RTC.
//Take care of week-day also.
DateTime dt(2014, 06, 05, 11, 5, 00, 4);

char CH_status_print[][4]=  { "Off","On ","Ok ","Err" };

static uint8_t prevSecond=0; 
static uint8_t prevMinute=0; 
static DateTime interruptTime;

//Interrupt service routine for external interrupt on INT0 pin conntected to DS3231 /INT
void INT0_ISR()
{
    detachInterrupt(0); 
    interruptTime = DateTime(interruptTime.get() + SLPNG);  //decide the time for next interrupt, configure next interrupt   
}

unsigned char read_charge_status(void)
{
  unsigned char CH_Status=0;
  unsigned int ADC6=analogRead(6);
  if(ADC6>900)  
   {CH_Status = 0;}//sleeping 
  else if(ADC6>550)
   {CH_Status = 1;}//charging
  else if(ADC6>350)
   {CH_Status = 2;}//done
  else
   {CH_Status = 3;}//error
  return CH_Status;
}
  


void setup () 
{
     pinMode(BEE_PWRPIN,OUTPUT);    //extern GPRS power 
     digitalWrite(BEE_PWRPIN,HIGH);     
     Serial.begin(9600);      // Serial1 is connected to SIM900 GPRSbee
     gprsbee.init(Serial, XBEECTS_PIN, GPRSBEE_PWRPIN);
     gprsbee.off();
    #ifdef ENABLE_DIAG
     diagport.begin(9600);
     gprsbee.setDiag(diagport);
    #endif
     
    /*Initialize INT0 pin for accepting interrupts */
     PORTD |= 0x04; 
     DDRD &=~ 0x04;
     pinMode(4,INPUT);//extern power
   
     Wire.begin();
     RTC.begin();
     RTC.adjust(dt); //Adjust date-time as defined 'dt' above 
     delay(1000);
     attachInterrupt(0, INT0_ISR, LOW); //Only LOW level interrupt can wake up from PWR_DOWN
     set_sleep_mode(SLEEP_MODE_PWR_DOWN);
 
     //Enable Interrupt 
     DateTime start = RTC.now();
     interruptTime = DateTime(start.get() + SLPNG); //Add SPLNG seconds in seconds to start time
     analogReference(INTERNAL); //Read battery status
    

}

void loop () 
{
    //Battery Charge Status and Voltage reader
    int BatteryValue = analogRead(A7);
    float voltage=BatteryValue * (1.1 / 1024)* (10+2)/2;  //Voltage devider
    unsigned char CHstatus = read_charge_status();//read the charge status
  
    ////////////////////// START : Application or data logging code//////////////////////////////////
    RTC.convertTemperature();          //convert current temperature into registers
    float temp = RTC.getTemperature(); //Read temperature sensor value
    
    // Convert temperature voltage to string for Ubidots
    char buffer5[14]; //make buffer large enough for 7 digits
    String temperatureS = dtostrf(temp, 7,2,buffer5);
    //'7' digits including '-' negative, decimal and white space. '2' decimal places
    temperatureS.trim(); //trim whitespace, important so Ubidots will treat it as a number
    
    DateTime now = RTC.now(); //get the current date-time    
    prevMinute = now.minute();
   
    gprsbee.on();
    bool smsSent = gprsbee.sendSMS(TELNO,"Temp");
    gprsbee.off();      
    gprsbee.off();   
    RTC.clearINTStatus(); //This function call is  a must to bring /INT pin HIGH after an interrupt.
    RTC.enableInterrupts(interruptTime.hour(),interruptTime.minute(),interruptTime.second());    // set the interrupt at (h,m,s)
    attachInterrupt(0, INT0_ISR, LOW);  //Enable INT0 interrupt (as ISR disables interrupt). This strategy is required to handle LEVEL triggered interrupt
    ////////////////////////END : Application code //////////////////////////////// 
       
    //\/\/\/\/\/\/\/\/\/\/\/\/Sleep Mode and Power Down routines\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
            
    //Power Down routines
    cli(); 
    sleep_enable();      // Set sleep enable bit
    sleep_bod_disable(); // Disable brown out detection during sleep. Saves more power
    sei();
        
    delay(50); //This delay is required to allow print to complete
    //Shut down all peripherals like ADC before sleep. Refer Atmega328 manual
    power_all_disable(); //This shuts down ADC, TWI, SPI, Timers and USART
    sleep_cpu();         // Sleep the CPU as per the mode set earlier(power down)  
    sleep_disable();     // Wakes up sleep and clears enable bit. Before this ISR would have executed
    power_all_enable();  //This shuts enables ADC, TWI, SPI, Timers and USART
    delay(10); //This delay is required to allow CPU to stabilize
    
    //\/\/\/\/\/\/\/\/\/\/\/\/Sleep Mode and Power Saver routines\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

} 
