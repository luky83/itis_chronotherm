#include <math.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#define DS3231_I2C_ADDR             0x68
#define DS3231_TEMPERATURE_ADDR     0x11

// LCD module connections (RS, E, D4, D5, D6, D7)
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// thermostat relay pin
int outPin = 13;

// minimum and maximum regulation temperature
const float t_min = 12; // degrees celsius
const float t_max = 32; // degrees celsius

char strTime[]     = "  :  :   -   .  ";    // time template string for LCD output
char strTemp[] = "TEMP:  h   . c   ";       // temperature template string for LCD output
byte i, second, minute, hour, date, month, year;

// holds most recent temperature reading
float currentTemp = 0;

// hold the ON or OFF status of the thermostat relay
int relayStatus = LOW;

// 24 h temperature array
float setPoint[] = {20.0, // c° at 0:00 h
                    20.0, // c° at 1:00 h
                    20.0, // c° at 2:00 h
                    20.0, // c° at 3:00 h
                    20.0, // c° at 4:00 h
                    21.0, // c° at 5:00 h
                    22.0, // c° at 6:00 h
                    22.0, // c° at 7:00 h
                    22.0, // c° at 8:00 h
                    22.0, // c° at 9:00 h
                    22.5, // c° at 10:00 h
                    22.5, // c° at 11:00 h
                    22.5, // c° at 12:00 h
                    22.5, // c° at 13:00 h
                    22.5, // c° at 14:00 h
                    22.5, // c° at 15:00 h
                    22.5, // c° at 16:00 h
                    22.5, // c° at 17:00 h
                    22.0, // c° at 18:00 h
                    22.0, // c° at 19:00 h
                    22.0, // c° at 20:00 h
                    22.0, // c° at 21:00 h
                    21.0, // c° at 22:00 h
                    21.0  // c° at 23:00 h
                   };


void setup() {
  Serial.begin(9600);
  pinMode(8, INPUT_PULLUP);  // first pushbutton wired at pin 8
  pinMode(9, INPUT_PULLUP);  // second pushbutton wired at pin 9
  pinMode(outPin, OUTPUT);   // thermostat relay wired at outPin
  lcd.begin(16, 2);
  Wire.begin(); 
}

void loop() {

  // read current temperature from sensor
  //currentTemp = getTermistorTmp();
  //currentTemp = getTMP36Tmp();
  currentTemp = getDS3231Tmp();
  
  // call the regulation algorithm
  regulation();

  // set current time on RTC module
  setTimeTemp();

  // print data on LCD module
  LCDPrint();

  // Uncomment to print debugging info on serial port
  //printInfo();

  delay(50);
}

/* 
Simple regulation algorithm.
Turn the thermostat relay on or off based on a threshold value plus or minus a delta to void output flicker.
*/
void regulation() {
  float delta = 0.2;
  if ( currentTemp < setPoint[hour] - delta) {
    relayStatus = HIGH;
  }
  if ( currentTemp > setPoint[hour] + delta) {
    relayStatus = LOW;
  }
  digitalWrite(outPin, relayStatus);
}

void printInfo() {
  Serial.print("set point: ");
  Serial.print(setPoint[hour]);
  Serial.print(" status: ");
  if (relayStatus == HIGH) Serial.print("ON");
  else Serial.print("OFF");
  Serial.print(" temp: ");
  Serial.println(currentTemp);
}

float getTermistorTmp() {
  float res;
  long Rpad = 10000;
  float val;
  val = float(analogRead(A0));
  res = Rpad * (1023.0 / val - 1);
  currentTemp = 1.0 / (1.0 / 298.15 + 1.0 / 4300.0 * log(res / 50000.0)) - 273;
  return currentTemp;
}

float getTMP36Tmp() {
  currentTemp = ((analogRead(A0) * 0.00488) - 0.5) / 0.01;
  return currentTemp;
}

float getDS3231Tmp() {
  currentTemp = DS3231_get_treg();
  return currentTemp;
}

void DS3231_display(){
  // Converto BCD in decimal
  second = (second >> 4) * 10 + (second & 0x0F);
  minute = (minute >> 4) * 10 + (minute & 0x0F);
  hour   = (hour >> 4)   * 10 + (hour & 0x0F);
  date   = (date >> 4)   * 10 + (date & 0x0F);  // unused
  month  = (month >> 4)  * 10 + (month & 0x0F); // unused
  year   = (year >> 4)   * 10 + (year & 0x0F);  // unused
  // End conversion

  // add current temperature at time string ( first LCD line )
  sprintf(strTime, "  :  :   - %d.%02d", (int)currentTemp, (int)(currentTemp*100)%100);
  // update time string with most recent values
  strTime[7]     = second % 10 + 48;
  strTime[6]     = second / 10 + 48;
  strTime[4]      = minute % 10 + 48;
  strTime[3]      = minute / 10 + 48;
  strTime[1]      = hour   % 10 + 48;
  strTime[0]      = hour   / 10 + 48;

  // add current set point temperature at Temp string ( second LCD line )
  sprintf(strTemp, "TEMP:  h %d.%02dc", (int)setPoint[hour], (int)(setPoint[hour]*100)%100);
  // update temp string with set point hour
  strTemp[6] = hour   % 10 + 48;
  strTemp[5] = hour   / 10 + 48;
  
  lcd.setCursor(0, 0);
  lcd.print(strTime);                               // print time on the LCD module
  lcd.setCursor(0, 1);
  lcd.print(strTemp);                           // print temperature on the LCD module
}

// make the parameter to be set blink
void blink_parameter(){
  byte j = 0;
  while(j < 10 && digitalRead(8) && digitalRead(9)){
    j++;
    delay(25);
  }
}

byte edit_time(byte x, byte y, byte parameter){
  char text[3];
  while(!digitalRead(8));                        // Wait until pin 8 is released
  while(true){
    while(!digitalRead(9)){                      // If pin 9 is pressed
      parameter++;
      if(i == 0 && parameter > 23)               // if hours > 23 then hours = 0
        parameter = 0;
      if(i == 1 && parameter > 59)               // if minutes > 59 then minutes = 0
        parameter = 0;
      sprintf(text,"%02u", parameter);
      lcd.setCursor(x, y);
      lcd.print(text);
      delay(200);                                // Wait 200ms
    }
    lcd.setCursor(x, y);
    lcd.print("  ");                             // Insert two spaces
    blink_parameter();
    sprintf(text,"%02u", parameter);
    lcd.setCursor(x, y);
    lcd.print(text);
    blink_parameter();
    if(!digitalRead(8)){                         // If pin 8 is pressed
      i++;                                       // Increment 'i' for next call to set the next parameter
      return parameter;                          // Return parameter value and exits
    }
  }
}

byte edit_temp(byte x, byte y, byte parameter){
  char text[5];
  while(!digitalRead(8));                        // Wait until pin 8 is released
  while(true){
    while(!digitalRead(9)){                      // If pin 9 is pressed
      setPoint[parameter] = setPoint[parameter]+0.5;
      if(i >= 2 && setPoint[parameter] > t_max)               // If temperature > t_max then temperature = t_min
        setPoint[parameter] = t_min;
      sprintf(text,"%d.%01d", (int)setPoint[parameter], (int)(setPoint[parameter]*100)%100);
      lcd.setCursor(x+4, y);
      lcd.print(text);
      delay(200);                                // Wait 200ms
    }
    sprintf(text,"%d.%01d", (int)setPoint[parameter], (int)(setPoint[parameter]*100)%100);
    lcd.setCursor(x+4, y);
    lcd.print(text);
    lcd.setCursor(x, y);
    lcd.print("  ");                             // Insert two spaces
    blink_parameter();
    sprintf(text,"%02u", parameter);
    lcd.setCursor(x, y);
    lcd.print(text);
    blink_parameter();
    if(!digitalRead(8)){                         // If pin 8 is pressed
      i++;                                       // Increment 'i' for next call to set the next parameter
      return parameter;                          // Return parameter value and exits
    }
  }
}

void setTimeTemp() {
  if(!digitalRead(8)){                           // If pin 8 is pressed
      i = 0;
      hour   = edit_time(0, 0, hour);
      minute = edit_time(3, 0, minute);
      edit_temp(5, 1, hour);
      edit_temp(5, 1, (hour+1)%24);
      edit_temp(5, 1, (hour+2)%24);
      edit_temp(5, 1, (hour+3)%24);
      edit_temp(5, 1, (hour+4)%24);
      edit_temp(5, 1, (hour+5)%24);
      edit_temp(5, 1, (hour+6)%24);
      edit_temp(5, 1, (hour+7)%24);
      edit_temp(5, 1, (hour+8)%24);
      edit_temp(5, 1, (hour+9)%24);
      edit_temp(5, 1, (hour+10)%24);
      edit_temp(5, 1, (hour+11)%24);
      edit_temp(5, 1, (hour+12)%24);
      edit_temp(5, 1, (hour+13)%24);
      edit_temp(5, 1, (hour+14)%24);
      edit_temp(5, 1, (hour+15)%24);
      edit_temp(5, 1, (hour+16)%24);
      edit_temp(5, 1, (hour+17)%24);
      edit_temp(5, 1, (hour+18)%24);
      edit_temp(5, 1, (hour+19)%24);
      edit_temp(5, 1, (hour+20)%24);
      edit_temp(5, 1, (hour+21)%24);
      edit_temp(5, 1, (hour+22)%24);
      edit_temp(5, 1, (hour+23)%24);
      // Convert from decimal to BCD
      minute = ((minute / 10) << 4) + (minute % 10);
      hour = ((hour / 10) << 4) + (hour % 10);
      date = ((date / 10) << 4) + (date % 10);
      month = ((month / 10) << 4) + (month % 10);
      year = ((year / 10) << 4) + (year % 10);
      // End conversion
      // Write data into DS3231 RTC module
      Wire.beginTransmission(0x68);               // Init I2C protocol at DS3231 address
      Wire.write(0);                              // Send address registration
      Wire.write(0);                              // Reset and start the oscillator
      Wire.write(minute);                         // Write minutes
      Wire.write(hour);                           // Write hours
      Wire.write(1);                              // Write weekday ( unused )
      Wire.write(date);                           // Write day ( unused )
      Wire.write(month);                          // Write mese ( unused )
      Wire.write(year);                           // Write year ( unused )
      Wire.endTransmission();                     // Stop transmission and release I2C bus
      delay(200);                                 // Wait 200ms
    }
}

void LCDPrint() {
    Wire.beginTransmission(0x68);                 // Init I2C protocol at DS3231 address
    Wire.write(0);                                // Send address registration
    Wire.endTransmission(false);                  // I2C restart
    Wire.requestFrom(0x68, 7);                    // Request 7 byte from DS3231 and release I2C bus at the end of reading
    second = Wire.read();                         // Read seconds from register 0
    minute = Wire.read();                         // Read minutes from register 1
    hour   = Wire.read();                         // Read hour from register 2
    Wire.read();                                  // Read weekday from register 3 ( unused )
    date   = Wire.read();                         // Read day from register 4 ( unused )
    month  = Wire.read();                         // Read month from register 5 ( unused )
    year   = Wire.read();                         // Read year from register 6 ( unused )
    DS3231_display();                             // Display time
    delay(50);                                    // Wait 50ms
}

float DS3231_get_treg(){
    //int rv;  // Reads the temperature as an int, to save memory
    float rv;
    
    uint8_t temp_msb, temp_lsb;
    int8_t nint;

    Wire.beginTransmission(DS3231_I2C_ADDR);
    Wire.write(DS3231_TEMPERATURE_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_I2C_ADDR, 2);
    temp_msb = Wire.read();
    temp_lsb = Wire.read() >> 6;

    if ((temp_msb & 0x80) != 0)
        nint = temp_msb | ~((1 << 8) - 1);      // if negative get two's complement
    else
        nint = temp_msb;

    rv = 0.25 * temp_lsb + nint;

    return rv;
}
