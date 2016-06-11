//RECEIVER
#include <VirtualWire.h>
int pin1 = 8;
int pin2 = 9;

int currentTemp = 0;
int targetTemp = 0;

#define PUSHTIME 100
#define WAITTIME 100

 
void setup()
{
    Serial.begin(9600);          // Configure the serial connection to the computer
    vw_set_ptt_inverted(true);  // Required by the RF module
    vw_setup(2000);            // bps connection speed
    vw_set_rx_pin(2);         // Arduino pin to connect the receiver data pin
    vw_rx_start();           // Start the receiver
    
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    
    calibrateNow();
}
 
void calibrateNow() {
  for (int i = 0; i < 10; i++) {
    push(1); //push down 10 times
  }
  currentTemp = 0; //we are now sure the current setting has to be 0.
  setTemp(); //go back to normal operation
}

void setTemp() {
  while (currentTemp < targetTemp) {
    push(2); //push up
    currentTemp++; //temp rises by 1
  }
  while (currentTemp > targetTemp) {
    push(1); //push down
    currentTemp--; //temp falls by 1
  }
}

void push(int button) {
  if (button == 1) {
       digitalWrite(pin1, HIGH);
       delay(PUSHTIME);
       digitalWrite(pin1, LOW);
       delay(WAITTIME);
  } else if (button == 2) {
       digitalWrite(pin2, HIGH);
       delay(PUSHTIME);
       digitalWrite(pin2, LOW);
       delay(WAITTIME);
  }
}
 
void loop()
{
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  if (vw_get_message(buf, &buflen))      // We check if we have received data
  {
    int i;
    // Message with proper check    
    for (i = 0; i < buflen; i++)
    {
      Serial.write((uint8_t)buf[i]);  // The received data is stored in the buffer
      if (buf[i] == (uint8_t)'C') { //calibration  
        calibrateNow();

      } else if (buf[i] >= '0' && buf[1] <= '9') { //if a number                  
          targetTemp = constrain( buf[i] - '0' , 0, 9);
          
          setTemp();
      }
    }
    Serial.print(targetTemp);
    Serial.println();
  }
}
