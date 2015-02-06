#GPRSBee_LP_SMS

This code send SMS text with RTC temperature value using GPRSBee. Low power is implemented every SLPNG seconds.

#Configuration
Send SMS every SLPNG seconds and go to sleep.

  SMS is RTC temperature
  
  Stalker + GPRSBee
  
  To program unconnect pin0 and 1 or take gprsbee out. (sharing ports 0 and 1 from Stalker)
  
  Jumper INT PD2 welded in Stalker to Use Interruption with RTC
  
  Cable CTS and PWRPin from GPRSBee to Stalker In pins
  
  GPRSBEE_PWRPIN (Stalker) <--> pin#9 (GPRSBee)
  
  XBEECTS_PIN    (Stalker) <--> pin#12 (GPRSBee)
  
  Port0 (Stalker) <--> pin#1 (GPRSBee) Tx
  
  Port1 (Stalker) <--> pin#2 (GPRSBee) Rx
  
  BEE_PWRPIN      5    //if PD5-En jumper welded in Stalker to swtich on and off bee power
  
  To read from serial Diag.h enable_diag in 1 and defined otherwise #undef
  
  #define ENABLE_DIAG     1
  
  UartSBee Gnd <--> Gnd Stalker
  
  UartSBee Rx <--> DIAGPORT_TX Stalker
  
  UartSBee TX <--> DIAGPORT_RX Stalker


##Libraries
https://github.com/SodaqMoja/GPRSbee

##Hardware
Stalker V2.3 http://www.seeedstudio.com/wiki/Seeeduino_Stalker_v2.3

GPRSBee http://www.gprsbee.com
