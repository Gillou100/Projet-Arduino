#include <MCP23008.h> // I2C
#include <MCP23S08.h> // SPI

#define MCP23008_ADDR 0x00 // Adresse I2C par défaut
#define MCP23S08_ADDR 0x00 // Adresse SPI par défaut

MCP23008 i2cIo(MCP23008_ADDR);     // Objet pour le protocole I2C
MCP23S08 spiIo(MCP23S08_ADDR, 10); // Objet pour le protocole SPI

uint8_t swStateI2c; // État des boutons I2C
uint8_t swStateSpi; // État des boutons SPI

void setup()
{
    Serial.begin(9600);

    I2cIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
    spiIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
}

void loop()
{
    swStateI2c = i2cIo.Read(GPIO);          // Lecture de l'état des boutons
    swStateI2c = (swStateI2c & 0x0F) << 4;  // Décalage des bits, je ne sais plus pourquoi xD
    i2cIo.Write(GPIO, swStateI2c);

    swStateSpi = spiIo.Read(GPIO);
    swStateSpi = (swStateI2c & 0x0F) << 4;
    spiIo.write(GPIO, swStateSpi);

    delay(100);
}
