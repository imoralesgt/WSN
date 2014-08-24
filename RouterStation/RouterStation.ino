#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
#include <stdlib.h>
#include <RTCplus.h>

#define MAX_RETRIES 3  //3 max retransmission attempts
#define RTY_TIMEOUT 3  //3 seconds timeout before attempting retransmission

const byte IRQ_PIN = P2_2; //NRF24L01+ IRQ Pin
const byte CE_PIN  = P2_0; //NRF24L01+ CE Pin
const byte CS_PIN  = P2_1; //NRF24L01+ CS Pin
const byte HUM_PIN = P1_4; //DHT11 Data Pin

Enrf24 radio(CE_PIN, CS_PIN, IRQ_PIN);
RealTimeClock rtc;

uint8_t txaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01 };
const uint8_t rxaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00 };

const char *str_on = "ON";
const char *str_off = "OFF";

const char *STR_RQST_DATA = "DATA";
const char *STR_SET_TIMEOUT = "TIMEOUT";

const int DATA_LEN = 6;

int TIME_OUT = 15;

void dump_radio_status_to_serialport(uint8_t);

void radioInit(void){
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  radio.begin();  // Defaults 1Mbps, channel 0, max TX power
  radio.setTXaddress((void*)txaddr);
}

void setup() {
  Serial.begin(9600);
  pinMode(RED_LED, OUTPUT);
  
  radioInit();
  
  rtc.Set_Time(10,12,0);
  rtc.begin();
}

unsigned int deltaSeconds(int start, int now){ //Compute time gap between two samples
  if(start > now){
    now += 60;
  }
  return (now - start);
}

unsigned int splitBufferToData(char *buffer, unsigned int *data){
  int i = 0;
  char *token;
  char *search = ";";
  token = strtok(buffer, search);
  while(token != NULL){
    data[i] = atoi(token);
    token = strtok(NULL, search);
    i++;
  }
}

byte verifyHash(char *buffer){
  unsigned int data[DATA_LEN];
  splitBufferToData(buffer, data);

  int i;
  long sum = 0;
  for(i = 0; i < DATA_LEN-1; i++){
    sum += data[i];
  }
  sum %= 256;
  if(sum==data[DATA_LEN-1]){
    return 1;
  }else{
    return 0;
  }
}

void dumpDataToSerial(char *data){
  Serial.print(rtc.RTC_hr, DEC);
  Serial.print(":");
  Serial.print(rtc.RTC_min, DEC);
  Serial.print(":");
  Serial.print(rtc.RTC_sec, DEC);
  Serial.print(" -->  ");
  Serial.print(data);
}

void sendRadioDataRequest(byte destAddr){
  txaddr[4] = destAddr;
  radio.setTXaddress(txaddr);
  radio.print(STR_RQST_DATA);
  radio.flush(); 
}

void setRadioRxMode(void){
  radio.begin();
  radio.setRXaddress((void*)rxaddr);
  radio.enableRX(); //Start listening
}

unsigned int timeMinutes(){
  return rtc.RTC_sec + rtc.RTC_min*60;
}

byte validateSleepTime(unsigned int start, unsigned int now){ //Has already passed enough time to wake up?
   unsigned int localNow = now;
   if (start > localNow){ //Just in case an overflow exist
     localNow += 3600; 
   }
   if (localNow < (start + TIME_OUT + 1)){
     return 0;
   }else{
     return 1;
   }
}

void dco1MHz(){
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
}

void dco16MHz(){
 BCSCTL1 = CALBC1_16MHZ;
 DCOCTL = CALDCO_16MHZ;  
}

void loop() {
  delay(1);
  char inbuf[33];
  int now, start, retries, dataValid;
  now = rtc.RTC_sec;
  start = now;
  retries = 0;
  
  dataValid = 0;
  
  while((retries < MAX_RETRIES) && (!dataValid)){
    start = rtc.RTC_sec;
    while(deltaSeconds(start,now) < RTY_TIMEOUT){
      digitalWrite(RED_LED, HIGH);
      now = rtc.RTC_sec;
      sendRadioDataRequest(1);
      setRadioRxMode();
      digitalWrite(RED_LED, LOW);
      if(radio.available(true)){
        dataValid = 1;
        break;
      }
    }
    retries++;
  }
  digitalWrite(RED_LED, LOW);
  
  if(!dataValid)
    Serial.println("SIN RESPUESTA DE ESTACION REMOTA");

  if(radio.read(inbuf)) {
    dumpDataToSerial(inbuf);   
    if(verifyHash(inbuf)){
      Serial.println("  OK!");
    }else{
      Serial.println("  Error de Hash");
    }    
  }
  
  radioInit();

  now = timeMinutes();
  start = now;
  
  while(!validateSleepTime(start, now)){ //Is it now time to wake up?
    now = timeMinutes();
    __bis_status_register(LPM1_bits);
  }

}

interrupt(TIMER1_A0_VECTOR) Tic_Tac(void){
  rtc.Inc_sec();
  __bic_status_register(LPM1_bits);
}


void dump_radio_status_to_serialport(uint8_t status){
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
