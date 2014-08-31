#include <pfatfs.h>
#include <stdlib.h>
#include <string.h>

const byte CS_SD_PIN = P1_4;
const byte ENABLE_RADIO_PIN = P1_7; //Output used to enable neighbor uC
const uint8_t SD_BUFFER_SIZE = 55;

unsigned short int bw = 0; //SD Card Write buffer
int rc=0; //SD Card error return
char dat; //Data to be written to SD Card
byte enableSD;
char buffer[SD_BUFFER_SIZE];
uint8_t i = 0; //Buffer's index


//WARNING: PFATFS LIBRARY HAS BEEN MODIFIED TO CHANGE MISO PIN
//CHECK OUT LINE NUMBER 30 FROM pfatfs.cpp
//Replaced with:
/*
uint8_t _SCLK = 13;
 uint8_t _MOSI = 12;
 uint8_t _MISO = 11;
 */

void die(int pff_err){ //If anything goes wrong, this procedure will be triggered
  //Serial.println();
  //Serial.print("Failed with rc=");
  //Serial.print(pff_err,DEC);
  digitalWrite(ENABLE_RADIO_PIN, HIGH);
  for (;;){
    digitalWrite(RED_LED, 1);
    delay(50);
    if (pff_err > 0)
      digitalWrite(RED_LED, 0);
    delay(350);
  }
}

void initSDcard(void){
  FatFs.begin(CS_SD_PIN, 4); //DIV = 4 => SPI_CLK @ 4MHz
}

void openSDfile(void){
  rc = FatFs.open("log.csv");
  if (rc) die(rc);
  rc = FatFs.write("Fecha,Hora,ID,Temperatura,Presion,Luminosidad,Humedad,Hash,\n", 60, &bw);
  if (rc) die(rc);
}

void writeToSD(){
  //Serial.print("BufferSize: ");
  //Serial.println(sizeof(buffer), DEC);
  //Serial.print("Data: ");
  //Serial.println(buffer);
  rc = FatFs.write(buffer, sizeof(buffer), &bw);
  if (rc) die(rc);
}

void setup(){
  
  Serial.begin(9600);
  
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, 0);
  
  pinMode(PUSH2, INPUT_PULLUP);
  
  pinMode(ENABLE_RADIO_PIN, OUTPUT);
  digitalWrite(ENABLE_RADIO_PIN, HIGH);
  
  i = 0;
  
  enableSD = digitalRead(PUSH2);
  if(enableSD){
    initSDcard();
    openSDfile();
  }
  
  
}

void loop()
{
  char data;
  digitalWrite(ENABLE_RADIO_PIN, LOW); //Neighbor uC ENABLED
  if(enableSD){
    if(Serial.available()>0){
      data = Serial.read();
      if (data == '\n'){
        writeToSD();
        int j;
        for(j = 0; j < SD_BUFFER_SIZE; j++){
          buffer[j] = (char)0;
        }
        i = 0;
      }else{
        buffer[i++] = data;
      }
    }
    
    if(!digitalRead(PUSH2)){
      digitalWrite(ENABLE_RADIO_PIN, HIGH); //Neighbor uC DISABLED
      FatFs.write(0, 0, &bw);
      die(-1);
    }
  }
  
}




/*
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
*/

/*
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
*/
