#include <Enrf24.h>    //NRF24L01 Radio
#include <string.h>    //Strcmp and other string functions
#include <SPI.h>       //Hardware SPI
#include <Wire.h>      //Hardware I2C
#include <BMP085_t.h>  //Pressure and Temperature Sensor
#include <BH1750.h>    //Light sensor
#include <dht11.h>     //Humidity sensor
#include <RTCplus.h>   //Real time clock


#define SENSOR_COUNT 4

const byte IRQ_PIN = P2_2; //NRF24L01+ IRQ Pin
const byte CE_PIN  = P2_0; //NRF24L01+ CE Pin
const byte CS_PIN  = P2_1; //NRF24L01+ CS Pin
const byte HUM_PIN = P1_4; //DHT11 Data Pin
const byte LED1    = P1_0; //RED LED Pin

Enrf24 radio(CE_PIN, CS_PIN, IRQ_PIN);  // P2.0=CE, P2.1=CSN, P2.2=IRQ
BMP085<0> PSensor;
BH1750 lightMeter;
dht11 humSensor;
RealTimeClock rtc;

const uint8_t rxaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x04 };
const uint8_t txaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00 };
int TIME_OUT = 15; //Timeout before enabling RX radio
unsigned int now, start;
int sensorData[SENSOR_COUNT]; //Temp, Pres, Lux, Hum


const char *str_on = "ON";
const char *str_off = "OFF";

const char *STR_RQST_DATA = "DATA";
const char *STR_SET_TIMEOUT = "TIMEOUT";


//Prototypes
void dump_radio_status_to_serialport(uint8_t);
void wakeUp();
void dco1MHz();
void dco16MHz();
byte validateTimeOut();
unsigned int timeMinutes();
void radioInit();
void sensorsInit();
void enableRx();

void setup() {
  
  pinMode(PUSH2, INPUT_PULLUP);
  delay(100);
  if(!digitalRead(PUSH2)){
    delay(25);
    if(!digitalRead(PUSH2)){
      TIME_OUT = 1;
    }
  }

  Serial.begin(9600);
  radioInit();
  
  //dump_radio_status_to_serialport(radio.radioState());
  
  pinMode(P1_0, OUTPUT);
  digitalWrite(P1_0, LOW);
  
  enableRx();  // Start listening
  rtc.begin();
  now = timeMinutes();
}

void loop() {
  char inbuf[33];
  
  dco16MHz();
  
  radioInit();
  enableRx(); //Start listening
   
  while (!radio.available(true))
    ;
  if (radio.read(inbuf)) { //If some data was received
    if (!strcmp(inbuf, STR_RQST_DATA)){ //If header is a DATA Request
       sensorsInit();
       digitalWrite(P1_0, LOW);
            
       humSensor.read(HUM_PIN);
       
       int hum = humSensor.humidity;
       int temp = humSensor.temperature*10;
       temp += PSensor.temperature;
       temp /= 2;
       //int hum = humSensor.temperature;
       
       uint16_t lux = lightMeter.readLightLevel();
       
       radioInit(); //Init radio as Tx
       radio.setTXaddress((void*)txaddr);

       //Temp, Pres, Lux, Hum       
       //sensorData[0] = PSensor.temperature;       //Celsius
       sensorData[0] = temp;       //Celsius
       sensorData[1] = (PSensor.pressure+50)/100; //hPa
       sensorData[2] = lux;                       //Luxes
       sensorData[3] = hum;                       //%HR

       
       /*
       Radio data will be sent as:
       MY_ID;TEMP;PRESS;LUX;HUM;HASH
       
       MY_ID <= UNIQUE NODE ID
       TEMP  <= TEMPERATURE*10 (FIXED POINT)
       PRESS <= ATM. PRESSURE (hPa)
       LUX   <= LIGHT METER MEASUREMENT
       HUM   <= RELATIVE HUMIDITY
       HASH  <= SUM OF PREVIOUS DATA MOD 256
       */
       
       long sum = rxaddr[4];
       int i;
       radio.print(rxaddr[4]); //Send my Address
       radio.print(";");
       for (i = 0; i < SENSOR_COUNT; i++){ //Send sensor data
         sum += sensorData[i];
         radio.print(sensorData[i]);
         radio.print(";"); //Semicolon-separated values
       }
       byte hash = (sum%256); //Simple hash used as checksum
       radio.print(hash);
       radio.print(";");
       radio.flush(); //Send the data that has been put in the radio's output buffer
       digitalWrite(P1_0, LOW); //Data sent, turn LED OFF.       
    }
  }
  
  now = timeMinutes();
  start = now;
  while(!validateTimeOut()){ //Periodically check: is it now time to wake up?
    now = timeMinutes();
    dco1MHz(); //Set DCO's speed to 1MHz
    __bis_status_register(LPM1_bits); //Low-Power Mode 1
  } 
}

byte validateTimeOut(){ //Has already passed enough time to wake up?
   unsigned int localNow = now;
   if (start > localNow){ //Just in case an overflow exist
     localNow += 3600; 
   }
   if (localNow < (start + TIME_OUT)){
     return 0;
   }else{
     return 1;
   }
}

unsigned int timeMinutes(){
  return rtc.RTC_sec + rtc.RTC_min*60;
}

void wakeUp(){
  __bic_status_register(LPM0_bits); 
}

void dco1MHz(){
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
}

void dco16MHz(){
 BCSCTL1 = CALBC1_16MHZ;
 DCOCTL = CALDCO_16MHZ;  
}

interrupt(TIMER1_A0_VECTOR) Tic_Tac(void){
  rtc.Inc_sec();
  __bic_status_register(LPM1_bits);
};

void enableRx(){
  radio.setRXaddress((void*)rxaddr);
  radio.enableRX();
  digitalWrite(LED1, HIGH);
}

void radioInit(){
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  pinMode(IRQ_PIN, INPUT_PULLUP);
  radio.begin();
}

void sensorsInit(){
  Wire.begin();
  lightMeter.begin();
  PSensor.begin();
  PSensor.refresh();
  PSensor.calculate();
}

void dump_radio_status_to_serialport(uint8_t status)
{
  Serial.print("Enrf24 radio transceiver status: ");
  switch (status) {
    case ENRF24_STATE_NOTPRESENT:
      Serial.println("NO TRANSCEIVER PRESENT");
      break;

    case ENRF24_STATE_DEEPSLEEP:
      Serial.println("DEEP SLEEP <1uA power consumption");
      break;

    case ENRF24_STATE_IDLE:
      Serial.println("IDLE module powered up w/ oscillators running");
      break;

    case ENRF24_STATE_PTX:
      Serial.println("Actively Transmitting");
      break;

    case ENRF24_STATE_PRX:
      Serial.println("Receive Mode");
      break;

    default:
      Serial.println("UNKNOWN STATUS CODE");
  }
}
