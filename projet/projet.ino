#include <MCP23008.h>   // I2C (boutons/Leds)
#include <MCP23S08.h>   // SPI (boutons/leds)

#define MCP23008_ADDR 0x00  // Adresse I2C par défaut
#define MCP23S08_ADDR 0x00  // Adresse SPI par défaut
#define MASQ6 0b0100    // Masque pour reconnaitre les appuis sur le bouton 6
#define MASQ7 0b0010    // Masque pour reconnaitre les appuis sur le bouton 7
#define MASQ8 0b0001    // Masque pour reconnaitre les appuis sur le bouton 8
#define MASQ9 0b1000    // Masque pour reconnaitre les appuis sur le bouton 9

MCP23008 i2cIo(MCP23008_ADDR);      // Objet pour le protocole I2C
MCP23S08 spiIo(MCP23S08_ADDR, 10);  // Objet pour le protocole SPI

void setup()
{
    Serial.begin(9600);

    i2cIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
    spiIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
}

void loop()
{
}

boolean appuis(int bouton)
{
    uint8_t swState;
    if(bouton >= 9 && bouton <= 12)
    {
        swState = i2cIo.Read(GPIO);
    }
    else if(bouton >= 5 && bouton <= 8)
    {
        swState = spiIo.Read(GPIO);
    }
    else
    {
        Serial.println("Bouton non reconnu");
        return false;
    }
    
    switch(bouton)
    {
        case 6:
            return !(swState & MASQ6);
            break;
        case 7:
            return !(swState & MASQ7);
            break;
        case 8:
            return !(swState & MASQ8);
            break;
        case 9:
            return !(swState & MASQ9);
            break;
        default:
            Serial.println("Bouton non utilisé");
            return false;
            break;
    }
}
