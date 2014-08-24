#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
#include <stdlib.h>

const byte IRQ_PIN = P2_2; //NRF24L01+ IRQ Pin
const byte CE_PIN  = P2_0; //NRF24L01+ CE Pin
const byte CS_PIN  = P2_1; //NRF24L01+ CS Pin
const byte HUM_PIN = P1_4; //DHT11 Data Pin

Enrf24 radio(CE_PIN, CS_PIN, IRQ_PIN);
const uint8_t txaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x04 };
const uint8_t rxaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00 };

const char *str_on = "ON";
const char *str_off = "OFF";

const char *STR_RQST_DATA = "DATA";
const char *STR_SET_TIMEOUT = "TIMEOUT";

const int DATA_LEN = 6;

int TIME_OUT = 15;

void dump_radio_status_to_serialport(uint8_t);

void setup() {
  Serial.begin(9600);

  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  radio.begin();  // Defaults 1Mbps, channel 0, max TX power
  //dump_radio_status_to_serialport(radio.radioState());

  radio.setTXaddress((void*)txaddr);
}

void loop() {
  //Serial.print("Sending packet: ");
  //Serial.println(str_on);
  
  char inbuf[33];
  radio.setTXaddress((void*)txaddr);
  radio.print(STR_RQST_DATA);
  radio.flush();  // Force transmit (don't wait for any more data)
  //dump_radio_status_to_serialport(radio.radioState());  // Should report IDLE
  radio.begin();
  radio.setRXaddress((void*)rxaddr);
  radio.enableRX();  // Start listening
  //dump_radio_status_to_serialport(radio.radioState());
  while (!radio.available(true))
    ;
  if(radio.read(inbuf)) {
    Serial.print(inbuf);
    unsigned int data[DATA_LEN];
    int i = 0;
    char *token;
    char *search = ";";
    token = strtok(inbuf, search);
    while(token != NULL){
      data[i] = atoi(token);
      token = strtok(NULL, search);
      i++;
    }

    long sum = 0;
    for(i = 0; i < DATA_LEN-1; i++){
      sum += data[i];
    }
    sum %= 256;
    if(sum==data[DATA_LEN-1]){
      Serial.println("  OK!");
    }else{
      Serial.println("  Error de Hash");
    }

    /*
    for(i = 0; i < DATA_LEN; i++){
      Serial.println(data[i]);
    }
    */
    
    
  }
  
  //dump_radio_status_to_serialport(radio.radioState());
  
  delay((TIME_OUT+1)*1000);

  //Serial.print("Sending packet: ");
  //Serial.println(str_off);
  //radio.print(str_off);
  //radio.flush();  //
  //dump_radio_status_to_serialport(radio.radioState());  // Should report IDLE
  //delay(4000);
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
