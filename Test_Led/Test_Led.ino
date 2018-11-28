#include <MCP23008.h> // I2C (boutons/Leds)
#include <MCP23S08.h> // SPI (boutons/leds)
#include <MCP2510.h>  // Bus communication
#include <Canutil.h>

#define MYID 10
#define MCP23008_ADDR 0x00 // Adresse I2C par défaut
#define MCP23S08_ADDR 0x00 // Adresse SPI par défaut
#define MASQ7 0b0010
#define MASQ8 0b0001

MCP23008 i2cIo(MCP23008_ADDR);     // Objet pour le protocole I2C
MCP23S08 spiIo(MCP23S08_ADDR, 10); // Objet pour le protocole SPI

uint8_t swStateI2c; // État des boutons I2C
uint8_t swStateSpi; // État des boutons SPI

void setup()
{
    Serial.begin(9600);

    i2cIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
    spiIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
}

void loop()
{
    swStateSpi = spiIo.Read(GPIO);
    
    if(appuis(7))
    {
        
        ledI2c(true);
    }
    else
    {
        ledI2c(false);
    }
    
    if (appuis(8))
    {
        ledSpi(true);
    }
    else
    {
        ledSpi(false);
    }

    delay(100);
}

boolean appuis(int bouton)
{
    if(bouton == 7)
    {
        return !(swStateSpi & MASQ7);
    }
    if(bouton == 8)
    {
        return !(swStateSpi & MASQ8);
    }
}
void ledI2c(bool ok)
{
    if(ok)
    {
        swStateI2c = i2cIo.Read(GPIO);
        swStateI2c = (swStateI2c & 0x0F) << 4;
        i2cIo.Write(GPIO, swStateI2c);
    }
    else
    {
        i2cIo.Write(GPIO, 0b1111 << 4);
    }
}
void ledSpi(bool ok)
{
    if(ok)
    {
        swStateI2c = i2cIo.Read(GPIO);
        swStateI2c = (swStateI2c & 0x0F) << 4;
        spiIo.Write(GPIO, swStateI2c);
    }
    else
    {
        spiIo.Write(GPIO, 0b1111 << 4);
    }   
}
