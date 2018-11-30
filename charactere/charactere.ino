#include <LiquidCrystal.h>
// initialize the library with the numbers of the interface pins
# define rs 15
# define rw 0
# define en 14
# define d4 4
# define d5 5
# define d6 6
# define d7 7
LiquidCrystal lcd(rs, rw, en, d4, d5, d6, d7);
// make some custom characters:
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};
byte smiley[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};
byte frownie[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b10001
};
byte armsDown[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b01010
};
byte armsUp[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00100,
  0b01010
};

byte degree[8] = {
  0b01000,
  0b10100,
  0b01000,
  0b00011,
  0b00100,
  0b00100,
  0b00011,
  0b00000
};
char toto;
void setup() {
  // create a new character
  lcd.createChar(0, heart);
  // create a new character
  lcd.createChar(1, smiley);
  // create a new character
  lcd.createChar(2, frownie);
  // create a new character
  lcd.createChar(3, armsDown);
  // create a new character
  lcd.createChar(4, armsUp);
  // set up the lcd's number of columns and rows:
  lcd.begin(16, 2);
  lcd.clear();
  // Print a message to the lcd.
  lcd.print("I ");
  toto = 0;
  lcd.write(toto);
  lcd.print(" Arduino! ");
  toto = 1;
  lcd.print(toto);
}
void loop() {
  // read the potentiometer:
  int sensorReading = analogRead(3);
  // map the result to 100 - 1000:
  int delayTime = map(sensorReading, 0, 1023, 100, 1000);
  // set the cursor to the bottom row, 5th position:
  lcd.setCursor(4, 1);
  // draw the little man, arms down:
  lcd.write(3);
  delay(delayTime);
  lcd.setCursor(4, 1);
  // draw him arms up:
  lcd.write(4);
  delay(delayTime);
}
