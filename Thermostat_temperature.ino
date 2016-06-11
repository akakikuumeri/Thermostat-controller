#include <OneWire.h>

// DS18S20 Temperature chip i/o
OneWire ds(9);  // on pin 9

 int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  

// include the library code:
#include <LiquidCrystal.h>
#include <VirtualWire.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 5, 4, 3, 2);

int pin1 = A1;
int pin2 = A2;
#define PUSHTIME 100
#define WAITTIME 100

int heaterSetting = 0;
int currentSetting = 0;
double timer = 0;
int targetTemp = 20;
int previousTemp = 20;
double sampletime = 60*1000;//in ms. period between PID updates and heater resettings

#define TIMEUP 200 //how many timer updates till we do PID and change temperature
#define SENDTIMEUP 10 //ho often to send the same command over and over for redundancy
#define DELAYTIME 200 //between screen updates

#define CALIBRATEINTERVAL 1000
int calibratetimer = 0;

#include <PID_v1.h>

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double P = 2;
double I = 5;
double D = 1;
PID myPID(&Input, &Output, &Setpoint,P ,I,D, DIRECT);



byte char1[8] = {//現
  B00111,
  B10101,
  B10111,
  B10101,
  B10111,
  B00110,
  B01011,
};

byte char2[8] = {//在
  B00100,
  B11111,
  B01100,
  B11010,
  B10111,
  B10010,
  B10111,
};

byte char3[8] = {
  B00110,//設
  B10111,
  B00101,
  B10000,
  B10111,
  B10010,
  B10101,
};

byte char4[8] = {
  B00100,//定
  B11111,
  B10001,
  B01110,
  B00111,
  B01100,
  B10011,
};

byte char5[8] = {
  B11111,//dou
  B10001,
  B11111,
  B10001,
  B11101,
  B11101,
  B10011,
};

byte char6[8] = {
  B01011,//ki
  B01011,
  B11011,
  B11011,
  B11011,
  B11011,
  B11101,
};

byte char7[8] = {
  B00100,//naka
  B11111,
  B10101,
  B10101,
  B11111,
  B00100,
  B00100,
};

byte char8[8] = {
  B00000,//antenna
  B01110,
  B10001,
  B10101,
  B01010,
  B00000,
  B00100,
};

void setup(void) {
  
  
  // initialize inputs/outputs
  // start serial port
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  //lcd.print("hello, world!");
  
  Serial.begin(9600);
  
  pinMode(A0, INPUT);
  
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  
  lcd.createChar(0, char1);
  lcd.createChar(1, char2);
  lcd.createChar(2, char3);
  lcd.createChar(3, char4);
  lcd.createChar(4, char5);
  lcd.createChar(5, char6);
  lcd.createChar(6, char7);
    lcd.createChar(7, char8);
  
  //initialize the variables we're linked to
  Input = 2000;
  Setpoint = targetTemp*100;

  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  //myPID.SetSampleTime(sampletime);//ms. This is every half a second
  myPID.SetOutputLimits(0,900);
  myPID.SetTunings(P, I, D);
  
  vw_set_ptt_inverted(true);  // Required by the RF module
  vw_setup(2000);            // bps connection speed
  vw_set_tx_pin(12);         // Arduino pin to connect the receiver data pin
  vw_set_rx_pin(11);
  vw_set_ptt_pin(10);
  
  calibrateNow();
  lcd.clear();
}

void loop(void) {
 

  ds.reset_search();
  if ( !ds.search(addr)) {
      //Serial.print("No more addresses.\n");
      ds.reset_search();
      return;
  }

  /*Serial.print("R=");
  for( i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
    Serial.print(" ");
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      return;
  }

  if ( addr[0] == 0x10) {
      Serial.print("Device is a DS18S20 family device.\n");
  }
  else if ( addr[0] == 0x28) {
      Serial.print("Device is a DS18B20 family device.\n");
  }
  else {
      Serial.print("Device family is not recognized: 0x");
      Serial.println(addr[0],HEX);
      return;
  }*/

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(DELAYTIME);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("P=");
  //Serial.print(present,HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
//  Serial.print(" CRC=");
//  Serial.print( OneWire::crc8( data, 8), HEX);
//  Serial.println();
  
  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

  Whole = Tc_100 / 100;  // separate off the whole and fractional portions
  Fract = Tc_100 % 100;
  
  previousTemp = targetTemp;
  targetTemp = map(analogRead(A0),0,1000,14,30);
  if (targetTemp == 14) { //turn off
    //write just the current temp
    lcd.clear();
    lcd.setCursor(0,0);
    
    //Serial.print(Whole);
  //  Serial.print(".");
    lcd.write(byte(0));//現在
    lcd.write(byte(1));
    lcd.write(' ');
    if (SignBit) // If its negative
    {
       lcd.print("-");
    }
    lcd.print(Whole);
    lcd.print(".");
    if (Fract < 10)
    {
       lcd.print("0");
    }
    //Serial.print(Fract);
    lcd.print(Fract);
    //lcd.write(' ');
    //lcd.write(byte(4));
    lcd.print((char)B11011111);//degree symbol
    lcd.print("C");
    
    //and then the word OFF
    lcd.setCursor(13,2);
    lcd.print("off");
    
    heaterSetting = 0;
    setTemp();//bring down to 0
    
    vw_send((uint8_t *) "0", 1); //send 0 to receiver a few times for redundancy
    vw_wait_tx();        // We wait to finish sending the message
    delay(100);
    vw_send((uint8_t *) "0", 1); //send 0 to receiver a few times for redundancy
    vw_wait_tx();        // We wait to finish sending the message
    delay(100);
    vw_send((uint8_t *) "0", 1); //send 0 to receiver a few times for redundancy
    vw_wait_tx();        // We wait to finish sending the message
    
    do {          //wait until setting changes
        delay(DELAYTIME);
        targetTemp = map(analogRead(A0),0,1000,14,30);
    } while (targetTemp == 14);
  }
  
  updateScreen();

  if (previousTemp != targetTemp && timer < TIMEUP - 2) { //if setting has changed, advande timer to at most 2 remaining
    timer = TIMEUP - 2;
  }
    
  if (timer++ > TIMEUP) { //if timer is up or temp setting has changed
    if (calibratetimer++ > CALIBRATEINTERVAL) {
      calibrateNow();
      
    }
    
    
    Input = (double)Tc_100;
    Setpoint = (double) (targetTemp * 100);
    myPID.Compute();
    heaterSetting = Output/100;
    
    updateScreen();
    
    timer = 0;
    
//    lcd.setCursor(15,0);
//    lcd.write(byte(7));//antenna symbol
//    
    char buf[10];
    vw_send((uint8_t *) itoa(heaterSetting, buf, 10), 1);
    vw_wait_tx();        // We wait to finish sending the message
    
//    delay(DELAYTIME);
//    lcd.setCursor(15,0);
//    lcd.write(' ');//clear the antenna symbol off
//    
    setTemp();
  }
  if ((int)timer % SENDTIMEUP == 0) {//even before timeup come up, send the same command over and over for redundancy
    char buf[10];
    vw_send((uint8_t *) itoa(heaterSetting, buf, 10), 1);
    vw_wait_tx();        // We wait to finish sending the message
  }
}

void calibrateNow() {
  //first send message to receiver to calibrate
  
  vw_send((uint8_t *) "C", 1);
  vw_wait_tx();        // We wait to finish sending the message
  lcd.clear();
  /*lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Calibrating");
  lcd.setCursor(0,1);
  lcd.print(" heater setting");*/
  lcd.setCursor(3,0);
  lcd.write((char)B11001011);//hi
  lcd.write('-');
  lcd.write((char)B11000000);//ta
  lcd.write('-');
  lcd.write(byte(2));//設定
  lcd.write(byte(3));
  lcd.write(byte(4));//同期中
  lcd.write((char)B10110111);//ki
  lcd.write(byte(6));
  for (int i = 0; i < 10; i++) {
    push(1); //push down 10 times
  }
  currentSetting = 0; //we are now sure the current setting has to be 0.
  setTemp(); //go back to normal operation
  lcd.clear();
  updateScreen();
}

void setTemp() {
  while (currentSetting < heaterSetting) {
    push(2); //push up
    currentSetting++; //temp rises by 1
  }
  while (currentSetting > heaterSetting) {
    push(1); //push down
    currentSetting--; //temp falls by 1
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

void updateScreen() {
  lcd.setCursor(0,0);
  
  //Serial.print(Whole);
//  Serial.print(".");
  lcd.write(byte(0));//現在
  lcd.write(byte(1));
  lcd.write(' ');
  if (SignBit) // If its negative
  {
     lcd.print("-");
  }
  
  lcd.print(Whole);
  lcd.print(".");
  if (Fract < 10)
  {
     lcd.print("0");
  }
  //Serial.print(Fract);
  lcd.print(Fract);
  //lcd.write(' ');
  //lcd.write(byte(4));
  lcd.print((char)B11011111);//degree symbol
  lcd.print("C");
  
  lcd.setCursor(0,1);
  lcd.write(byte(2));//設定
  lcd.write(byte(3));
  lcd.write(' ');
  lcd.print(String(targetTemp));
  //lcd.print(" ");
  //lcd.write(byte(4));
  lcd.print((char)B11011111);//degree symbol
  lcd.print("C");
  
  lcd.setCursor(10,1);
  lcd.write((char)B11001011);//hi
  lcd.write('-');
  lcd.write((char)B11000000);//ta
  lcd.write('-');
  lcd.write(' ');
  lcd.print(heaterSetting);

  //Serial.print("\n");
}
