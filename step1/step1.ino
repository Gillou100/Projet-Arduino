#include <LiquidCrystal.h>
LiquidCrystal lcd(15, 0, 14, 4, 5, 6, 7);  //  Ecran LCD

byte un[8] = 
{
    0b00000,
    0b00000,
    0b00000,
    0b00100,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
};

byte deux[8] = 
{
    0b00000,
    0b00001,
    0b00000,
    0b00000,
    0b00000,
    0b10000,
    0b00000,
    0b00000,
};

byte trois[8] = 
{
    0b00000,
    0b00001,
    0b00000,
    0b00100,
    0b00000,
    0b10000,
    0b00000,
    0b00000,
};

byte quatre[8] = 
{
    0b00000,
    0b10001,
    0b00000,
    0b00000,
    0b00000,
    0b10001,
    0b00000,
    0b00000,
};

byte cinq[8] = 
{
    0b00000,
    0b10001,
    0b00000,
    0b00100,
    0b00000,
    0b10001,
    0b00000,
    0b00000,
};

byte six[8] = 
{
    0b00000,
    0b10001,
    0b00000,
    0b10001,
    0b00000,
    0b10001,
    0b00000,
    0b00000,
};

void setup()
{
    Serial.begin(9600);
    
    lcd.createChar(1, un);
    lcd.createChar(2, deux);
    lcd.createChar(3, trois);
    lcd.createChar(4, quatre);
    lcd.createChar(5, cinq);
    lcd.createChar(6, six);

    lcd.begin(16,2);

    lcd.clear();
}

void loop()
{
    lcd.setCursor(1, 1);
    for(int i = 1; i <= 6; i++)
    {
        delay(500);
        lcd.write(i);
        lcd.write(" ");
    }
    delay(2000);
    lcd.clear();
}
