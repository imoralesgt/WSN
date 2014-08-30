#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
#include <pfatfs.h>
#include <stdlib.h>
#include <RTCplus.h>

#define MAX_RETRIES 3  //3 max retransmission attempts
#define RTY_TIMEOUT 1  //1 second for timeout before attempting retransmission

const byte IRQ_PIN         = P2_2; //NRF24L01+ IRQ Pin
const byte CE_PIN          = P2_0; //NRF24L01+ CE Pin
const byte CS_PIN          = P2_1; //NRF24L01+ CS Pin
const byte CS_SD_PIN       = P1_4; //SD Card CS Pin
const byte STN_CNT_PINS[3] = {P2_3, P2_4, P2_5}; //Station Count PINs

Enrf24 radio(CE_PIN, CS_PIN, IRQ_PIN);
RealTimeClock rtc;

uint8_t txaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01 };
const uint8_t rxaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00 };
const uint16_t RADIO_SPEED = 250000;

const char *str_on = "ON";
const char *str_off = "OFF";

const char *STR_RQST_DATA = "DATA";
const char *STR_SET_TIMEOUT = "TIMEOUT";

const int DATA_LEN = 6;
const int BUFFER_SIZE = 33;

unsigned int TIME_OUT = 10;
byte STATIONS_COUNT = 0;
unsigned int sensorData[DATA_LEN];
byte failed=0; //Failed attempts to get date in current sampling

unsigned short int bw = 0; //SD Card Write buffer
int rc=0; //SD Card error return
char dat; //Data to be written to SD Card
byte enableSD;

//WARNING: PFATFS LIBRARY HAS BEEN MODIFIED TO CHANGE MISO PIN
//CHECK OUT LINE NUMBER 30 FROM pfatfs.cpp
//Replaced "uint8_t _MISO = 14;"
//With     "uint8_t _MISO = 11;"

void dump_radio_status_to_serialport(uint8_t);

void radioInit(void){
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  radio.begin(RADIO_SPEED);  // Defaults 1Mbps, channel 0, max TX power
  radio.setTXaddress((void*)txaddr);
}

void countRemoteStations(void){
  byte cnt;
  /*
  cnt  = digitalRead(STN_CNT_PINS[0])   +
         digitalRead(STN_CNT_PINS[1])*2 +
         digitalRead(STN_CNT_PINS[2])*4;
  */
  cnt = 1; //Just for debugging
  STATIONS_COUNT = cnt;
}

void die(int pff_err){ //If anything goes wrong, this procedure will be triggered
  Serial.println();
  Serial.print("Failed with rc=");
  Serial.print(pff_err,DEC);
  for (;;){
    digitalWrite(RED_LED, 1);
    delay(50);
    digitalWrite(RED_LED, 0);
    delay(350);
  }
}

void openSDfile(void){
  rc = FatFs.open("T_WRITE.TXT");
  if (rc) die(rc);
  rc = FatFs.write("\n", 1, &bw);
  if (rc) die(rc);
}

void initSDcard(void){
  FatFs.begin(CS_SD_PIN, 4);
}

void setup() {
  int j;
  
  pinMode(RED_LED, OUTPUT);
  pinMode(PUSH2, INPUT_PULLUP);
 
  //Disable SD Card operations during TEST MODE
  enableSD = digitalRead(PUSH2);
  if(enableSD){
    initSDcard();
    openSDfile();
  }

  for(j = 0; j < 3; j++){
    pinMode(STN_CNT_PINS[j], INPUT_PULLUP);
  }
  
  Serial.begin(9600);
  countRemoteStations(); //set value on STATIONS_COUNT variable
  
  
  
  radioInit();
  
  rtc.Set_Time(10,12,0);
  rtc.Set_Date(2014,8,19);
  rtc.begin();
}

void writeSDdate(){
  dat = (rtc.RTC_day/10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  dat = (rtc.RTC_day%10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  rc = FatFs.write("/", 1, &bw);
  if (rc) die(rc);
  
  dat = (rtc.RTC_month/10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  dat = (rtc.RTC_month%10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  rc = FatFs.write("/", 1, &bw);
  if (rc) die(rc);
  
  rc = FatFs.write("20", 2, &bw);
  if (rc) die(rc);
  char tempYear = (char)(rtc.RTC_year % 100);
  dat = (tempYear/10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  dat = (tempYear%10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);

  rc = FatFs.write(";", 1, &bw);
  if (rc) die(rc);
}

void writeSDtime(){
  dat = (rtc.RTC_hr/10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  dat = (rtc.RTC_hr%10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  rc = FatFs.write(":", 1, &bw);
  if (rc) die(rc);

  dat = (rtc.RTC_min/10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  dat = (rtc.RTC_min%10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  rc = FatFs.write(":", 1, &bw);
  if (rc) die(rc);

  dat = (rtc.RTC_sec/10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  dat = (rtc.RTC_sec%10)+48;
  rc = FatFs.write(&dat, 1, &bw);
  if (rc) die(rc);
  rc = FatFs.write(";", 1, &bw);
  if (rc) die(rc);
}

unsigned int deltaSeconds(int start, int now){ //Compute time gap between two samples
  if(start > now){
    now += 60;
  }
  return (now - start);
}

void splitBufferToData(char *buffer, unsigned int *data){
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

void dumpTimeToSerial(void){
  byte hrs  = rtc.RTC_hr;
  byte mins = rtc.RTC_min;
  byte secs = rtc.RTC_sec;
  if (hrs < 10)
    Serial.print("0");
  Serial.print(hrs, DEC);
  Serial.print(":");
  if (mins < 10)
    Serial.print("0");
  Serial.print(mins, DEC);
  Serial.print(":");
  if (secs < 10)
    Serial.print("0");
  Serial.print(secs, DEC);
  Serial.print(" -->  ");
}
  
void dumpDataToSerial(char *data){
  dumpTimeToSerial();
  Serial.print(data);
}

void sendRadioDataRequest(byte destAddr){
  txaddr[4] = destAddr;
  radio.setTXaddress(txaddr);
  radio.print(STR_RQST_DATA);
  radio.flush(); 
}

void setRadioRxMode(void){
  radio.begin(RADIO_SPEED);
  radio.setRXaddress((void*)rxaddr);
  radio.enableRX(); //Start listening
}

unsigned int timeMinutes(){
  return rtc.RTC_sec + rtc.RTC_min*60;
}

byte validateSleepTime(unsigned int start, unsigned int now){ //Has already passed enough time to wake up?
   if(!digitalRead(PUSH2)){
     return 1;
   }
   unsigned int localNow = now;
   if (start > localNow){ //Just in case an overflow exist
     localNow += 3600; 
   }
   if (localNow < (start + TIME_OUT - failed*MAX_RETRIES*RTY_TIMEOUT)){
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

byte RQSTandReadData(byte ID, char *inbuf){
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
      sendRadioDataRequest(ID);
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
  
  if(!dataValid){
    dumpTimeToSerial();
    Serial.print("SIN RESPUESTA DE ESTACION #");
    Serial.println(ID, DEC);
    failed++;
  }

  if(radio.read(inbuf)) {
    dumpDataToSerial(inbuf);   
    if(verifyHash(inbuf)){
      dataValid = 1;
      splitBufferToData(inbuf,sensorData);
      Serial.println("");
      return 1;
    }else{
      Serial.println("  Error de Hash");
      dataValid = 0;
      return 0;
    }    
  }
  
}

void writeToSD(char *dat){
  Serial.print("BufferSize: ");
  Serial.println(sizeof(dat), DEC);
  Serial.print("Data: ");
  Serial.println(dat);
  rc = FatFs.write("Hola Mundo\nHola Mundo\n", 22, &bw);
  if (rc) die(rc);
}

void loop() {
  char inbuf[BUFFER_SIZE];
  
  byte stn;

  failed = 0; //Remove deadtime if failure in reading sensor happens
              //This is to improve nodes' battery life
  for(stn = 1; stn <= STATIONS_COUNT; stn++){
    if(RQSTandReadData(stn, inbuf)){
      //digitalWrite(CS_SD_PIN, LOW);
      
      //Intentar con LSEEK para mover el puntero
      //y reabrir el puerto cada vez que se utilice
      //initSDcard();
      //openSDfile();
      if(enableSD){
        writeSDdate();
        writeSDtime();
        writeToSD(inbuf);
        Serial.println("SD OK!");
      }
      //FOR EACH STATION, STORE DATA IN SD CARD HERE!
    }
  }
  //digitalWrite(CS_SD_PIN, HIGH); //Manually de-select SD Card
  
  radioInit();

  int now = timeMinutes();
  int start = now;
  
  while(!validateSleepTime(start, now)){ //Is it now time to wake up?
    now = timeMinutes();
    __bis_status_register(LPM1_bits);
  }
  
  if(!digitalRead(PUSH2)){
    rc = FatFs.write(0, 0, &bw);
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
