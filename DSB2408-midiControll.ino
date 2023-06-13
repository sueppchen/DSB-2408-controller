/*
Midi-interface for DSB2408 Stagebox
Block 0  1  2
CH    1  5  9 cc88..cc90 = Change BLOCK --> 0..1..2 BLOCK!
CH    2  6 10 = in  1.. 8
CH    3  7 11 = in  9..16
CH    4  8 12 = in 17..24
Controller:
PhantomGroup cc20 (0= 1-4 off, else = 1-4 +48V) - not used
PhantomGroup cc21 (0= 5-8 off, else = 5-8 +48V) - not used
Gain    cc 24..cc 31 (0/1 = 0dB , 2/3 = 10dB, 4/5 = 11dB .. 112-127=65dB)  --> n = 2*(xdB - 9dB)
Pad     cc104..cc111 (0=off, else = -20dB)
Phantom cc112..cc119 (0=off, else = +48V)

longpress value = menue
- Param save ok
- send all
- reset & send
- backlight timeout

while sending PAD / PHANTOM gain is reduced to zero.


UI:
    --------------------
sel | 01  -20dB  V=off | value
    | XXXXXXXX      45 |                      
    --------------------

S:BackLight              S:Pad
L:Phantom                L:menue



input
 2x Rotary Encoder
 Pushbutton = pad(value) / Phantom(longpress sel)
 *      ____
 * ____|    |___
 *   ____      ____
 * _|    |____|

 Pins:
 2 int EncClockA
 3 int EncClockB
 4 EncDataA
 5 EncDataB
 6 EncPushA
 7 EncPushB
 A4 SDA
 A5 SCL

*/

//#define DEBUG

#define BLOCK   0

//Encoder A ( left, select, Push= PAD / menue)
#define CL_ENCA  3
#define DA_ENCA  5
#define PU_ENCA  7

//Encoder B (right, value , Push= light / phantom)
#define CL_ENCB  2
#define DA_ENCB  4
#define PU_ENCB  6

#define LED     13

#define PRESS_SHORT   300
#define PRESS_LONG   2000

#define BL_UPPER   1000
#define BL_LOWER      0
#define BL_TIMEOUT   10
int blTimeout;                   //coming soon ;-)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>


#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

// make the custom characters:

// empty= 0x20
byte tick1[8] = { 0x10,0x10,0x10,0x10, 0x10,0x10,0x10,0x10 };
byte tick2[8] = { 0x18,0x18,0x18,0x18, 0x18,0x18,0x18,0x18 };
byte tick3[8] = { 0x1c,0x1c,0x1c,0x1c, 0x1c,0x1c,0x1c,0x1c };
byte tick4[8] = { 0x1e,0x1e,0x1e,0x1e, 0x1e,0x1e,0x1e,0x1e };
//all on =  0xff

LiquidCrystal_I2C lcd(0x3f,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

byte channel = 0;

struct dsb2408{
  byte gain;
  bool pad;
  bool phantom;
};

dsb2408 stagebox[25];

bool intOccured, encA, buttonA, buttonAltA, buttonB, buttonAltB, backlight=1;
byte change;
unsigned long now, timeout, buttonDown;
byte menue;

//

void IntEncA() {
  encA = 1;
  intOccured = 1;
}

void IntEncB() {
  encA = 0;
  intOccured = 1;
}

void setBlock(byte block){
  // better do nothing
}

void sendGain(byte ch, bool silence){
  byte mChan = ch / 8 + 4*BLOCK + 1;
  byte gainCC = ch % 8 + 24;
  byte gainValue;
  gainValue = silence ? 0 : (stagebox[ch].gain == 0) ? 0 : 2 * (stagebox[ch].gain - 9) ;
  
  #ifdef DEBUG
    Serial.print("CH:");
    Serial.print(mChan);
    Serial.print(" CC:");
    Serial.print(gainCC);
    Serial.print(" val:");
    Serial.print(gainValue);
    Serial.print(" message:");
    Serial.print(0xB0 + mChan , HEX);
    Serial.print(" ");
    Serial.print(gainCC, HEX);
    Serial.print(" ");
    Serial.print(gainValue, HEX);
    Serial.println();
  #else
    Serial.write(0xB0+mChan);
    Serial.write(gainCC);
    Serial.write(gainValue);
  #endif
}
void sendPad(byte ch){
  byte mChan = ch / 8 + 4*BLOCK + 1;
  byte padCC = ch % 8 + 104;
  byte padValue = stagebox[ch].pad ? 127 : 0 ;

  #ifdef DEBUG
    Serial.print("CH:");
    Serial.print(mChan);
    Serial.print(" CC:");
    Serial.print(padCC);
    Serial.print(" val:");
    Serial.print(padValue);
    Serial.print(" message:");
    Serial.print(0xB0 + mChan , HEX);
    Serial.print(" ");
    Serial.print(padCC, HEX);
    Serial.print(" ");
    Serial.print(padValue, HEX);
    Serial.println();
  #else
    Serial.write(0xB0+mChan);
    Serial.write(padCC);
    Serial.write(padValue);
  #endif

}
void sendPhantom(byte ch){
  byte mChan = ch / 8 + 4*BLOCK + 1;
  byte phantomCC = ch % 8 + 112;
  byte phantomValue = stagebox[ch].phantom ? 127 : 0 ;

  #ifdef DEBUG
    Serial.print("CH:");
    Serial.print(mChan);
    Serial.print(" CC:");
    Serial.print(phantomCC);
    Serial.print(" val:");
    Serial.print(phantomValue);
    Serial.print(" message:");
    Serial.print(0xB0 + mChan , HEX);
    Serial.print(" ");
    Serial.print(phantomCC, HEX);
    Serial.print(" ");
    Serial.print(phantomValue, HEX);
    Serial.println();
  #else
    Serial.write(0xB0+mChan);
    Serial.write(phantomCC);
    Serial.write(phantomValue);
  #endif

}

#include <EEPROM.h>

void saveEepromData(){
  for (int i = 0 ; i < 24  ; i ++) {
    EEPROM.put(i * sizeof(dsb2408), stagebox[i]);     //addr, Data
  }
}

void readEepromData(){
  for (int i = 0 ; i < 24 ; i ++) {
    EEPROM.get(i * sizeof(dsb2408), stagebox[i]);
    //check Data:
    if(stagebox[i].gain > 65){ //empty cell
      stagebox[i].gain = 0;
      stagebox[i].pad = 0;
      stagebox[i].phantom = 0;
    }
  }
  
}

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #else
    Serial.begin(31250);
  #endif

  lcd.init();                      // initialize the lcd 
  lcd.backlight();

  // create a new character
  lcd.createChar(0, tick1);
  lcd.createChar(1, tick2);
  lcd.createChar(2, tick3);
  lcd.createChar(3, tick4);
  lcd.home();
  lcd.cursor_off();
  // put your setup code here, to run once:

  pinMode(CL_ENCA, INPUT_PULLUP); 
  pinMode(DA_ENCA, INPUT_PULLUP);
  pinMode(PU_ENCA, INPUT_PULLUP); 
  pinMode(CL_ENCB, INPUT_PULLUP); 
  pinMode(DA_ENCB, INPUT_PULLUP);
  pinMode(PU_ENCB, INPUT_PULLUP); 
  pinMode(LED, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(CL_ENCA), IntEncA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CL_ENCB), IntEncB, CHANGE);

  //enable Interrupts;
  setBlock(BLOCK);

  readEepromData();

}

void uiShow(byte ch){
  lcd.setCursor(0, 0); //sign, line
  if((ch+1)<10) lcd.print(0);
  lcd.print(ch+1);
  lcd.print(stagebox[ch].pad     ? "  -20dB ":"  P=off ");
  lcd.print(stagebox[ch].phantom ? "!+48V!":" V=off");

  byte value = stagebox[ch].gain;
  lcd.setCursor(0, 1); //sign, line
  lcd.print("             ");
  lcd.setCursor(0, 1);
  
  do{
    if(value > 4){
      lcd.printByte(0xff);
      value -= 5;
    }
    else if(value > 3){
      lcd.printByte(3);
      value -= 4;
    }
    else if(value > 2){
      lcd.printByte(2);
      value -= 3;
    }
    else if(value > 1){
      lcd.printByte(1);
      value -= 2;
    }
    else if(value > 0){
      lcd.printByte(0);
      value -= 1;
    }
    
  }while (value > 0);
  lcd.setCursor(13, 1);
  lcd.print(" ");
  if(stagebox[ch].gain<10) lcd.print(" ");
  lcd.print(stagebox[ch].gain);
}

void menueShow(){
  lcd.clear();
  lcd.setCursor(0, 0); //sign, line
  if(menue == 10){
    lcd.print("all Param Save?");
  }
  if(menue == 11){
    lcd.print("send all data");
  }
  if(menue == 12){
    lcd.print("reset all + send");
  }
  if(menue == 13){
    lcd.print("backlight time");
  }
  #ifdef DEBUG
    lcd.setCursor(14, 0);                    //sign, line
    lcd.print(menue);
  #endif
  lcd.setCursor(0, 1);                     //sign, line
  if((menue > 9) && (menue<13)){
    lcd.print("long press VALUE");
  }
  if(menue == 13){
    lcd.print("time = ");
    lcd.print(blTimeout);
    lcd.print("s");
  }

}

void loop() {
  do{
    now = millis();
    buttonA = digitalRead(PU_ENCA);
    buttonB = digitalRead(PU_ENCB);
    //digitalWrite(LED,(intOccured));
  }while( (buttonA == buttonAltA) && (buttonB == buttonAltB) && !intOccured && (now < timeout));
  
  timeout = now + 100000;

  if(intOccured > 0){
    if(encA){                                                         // value
      if(menue == 0) {
        if (digitalRead(CL_ENCA) != digitalRead(DA_ENCA)){
         if(stagebox[channel].gain < 65) stagebox[channel].gain ++;
          if(stagebox[channel].gain < 10) stagebox[channel].gain += 9;
        }  
        else{
          if(stagebox[channel].gain > 10) stagebox[channel].gain --;
          else if(stagebox[channel].gain > 0) stagebox[channel].gain -= 10;
        }
      } 
      if(menue == 13) {
        if (digitalRead(CL_ENCA) != digitalRead(DA_ENCA)){ if(blTimeout < BL_UPPER) blTimeout ++ ;}
        else                                             { if(blTimeout > BL_LOWER) blTimeout -- ;}
      } 
      #ifdef DEBUG
        Serial.println("Enc A");
      #endif
      change = 1;
    }
    
    else{                                                            // sel
      if(menue == 0) {
        if (digitalRead(CL_ENCB) != digitalRead(DA_ENCB))  (channel == 23) ? channel =  0 : channel++;
        else                                               (channel == 0)  ? channel = 23 : channel--;
        }
      if(menue > 9) {
        if (digitalRead(CL_ENCB) != digitalRead(DA_ENCB))  (menue == 13) ? menue = 10 : menue++;
        else                                               (menue == 10) ? menue = 13 : menue--;
        }
      #ifdef DEBUG
        Serial.println("Enc B");
      #endif
      }
    delay(1);   
  }

  if(buttonA < buttonAltA){       //A pressed
    buttonDown = now;             // store time when pressed
    #ifdef DEBUG
      Serial.println("A pressed ");
    #endif
  }

  if(buttonB < buttonAltB){       //B pressed
    buttonDown = now;             // store time when pressed
    #ifdef DEBUG
      Serial.println("B pressed");
    #endif
  }

  if(buttonA > buttonAltA){                                               // [Value]
    if((now - buttonDown) <= PRESS_SHORT){                                               //short
      //short-action
      if(menue == 0) backlight = !backlight;
      if(menue > 0) menue = 0 ; 
    } 
    else if( ((now - buttonDown) > PRESS_SHORT) && ((now - buttonDown) < PRESS_LONG)){   //medium
      //Bold-Action
    }
    else if((now - buttonDown) >= PRESS_LONG){                                           //long
      //long-action
      if(menue == 0){
        stagebox[channel].phantom = !stagebox[channel].phantom;
        change = 3;
      }
      else if(menue == 10){
          saveEepromData();
          menue = 0;
          lcd.clear();
          lcd.setCursor(0, 0); //sign, line
          lcd.print("  -= Save OK =-  ");
          menue = 0;
          delay(1000); 
      } 
      else if(menue == 11){                     //send all
        lcd.clear();
        for(byte i = 0;i<24; i++){
          lcd.setCursor(0, 0); //sign, line
          lcd.print(i);
          sendGain(i,1);
          sendPad(i);
          sendPhantom(i);        
        }
        lcd.setCursor(0, 0); //sign, line
        lcd.print("  -= silence =- ");
        delay(2000);
        for(byte i = 0;i<24; i++){
          sendGain(i,0);
        }
        lcd.setCursor(0, 0); //sign, line
        lcd.print("  -=  done  =- ");
        menue = 0;
        delay(1000); 
      }  
      else if(menue == 12){                    //reset all + send
        lcd.clear();
        for(byte i = 0;i<24; i++){
          lcd.setCursor(0, 0); //sign, line
          lcd.print(i);
          stagebox[i].gain = 0;
          sendGain(i,0);
          stagebox[i].pad = 0;
          sendPad(i);
          stagebox[i].phantom = 0;
          sendPhantom(i);
        }
        lcd.setCursor(0, 0); //sign, line
        lcd.print(" -= reset DONE =- ");
        lcd.setCursor(0, 1); //sign, line
        lcd.print("  -= all send =- ");
        menue = 0;
        delay(1000); 
      }  
    }
    #ifdef DEBUG
      Serial.println("A released ");
    #endif
  }

  if(buttonB > buttonAltB){                  //Select
    if((now - buttonDown) <= PRESS_SHORT){                                               //short
      //short-action
      if(menue == 0){
        stagebox[channel].pad = !stagebox[channel].pad;
        change = 2;
      }
      if(menue == 13) {
        blTimeout = BL_TIMEOUT;
      }  
    } 
    else if( ((now - buttonDown) > PRESS_SHORT) && ((now - buttonDown) < PRESS_LONG)){   //medium
      //bold-action
    }
    else if((now - buttonDown) >= PRESS_LONG){                                           //long
      //long-action
      if(menue==0) menue +=10;
    }
    #ifdef DEBUG
      Serial.println("B released ");
    #endif
  }


  if(menue == 0){
    uiShow(channel);
    backlight ? lcd.backlight() : lcd.noBacklight();
    if(change == 1) sendGain(channel,0);
    if(change == 2){
       sendGain(channel,1);
       sendPad(channel);
       delay(10);
       sendGain(channel,0);
     }
    if(change == 3){
       sendGain(channel,1);
       sendPhantom(channel);
       delay(500);
       sendGain(channel,0);
     }
  }
  else menueShow();
  
  
  intOccured = 0;
  buttonAltA = buttonA;
  buttonAltB = buttonB;
  change = 0;
}
