#include <MCP23008.h>   // I2C (boutons/Leds)
#include <MCP23S08.h>   // SPI (boutons/leds)

#define MCP23008_ADDR 0x00  // Adresse I2C par défaut
#define MCP23S08_ADDR 0x00  // Adresse SPI par défaut
#define MASQ6 0b0100
#define MASQ7 0b0010
#define MASQ8 0b0001
#define MASQ9 0b1000

MCP23008 i2cIo(MCP23008_ADDR);      // Objet pour le protocole I2C
MCP23S08 spiIo(MCP23S08_ADDR, 10);  // Objet pour le protocole SPI


uint8_t swState;    // États des boutons;
uint8_t donnees[8];

byte boutons[] = {7,8};
byte bouton;

void setup()
{
    Serial.begin(9600);

    i2cIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
    spiIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
}

void loop()
{
    bouton = 0;
    for(int i = 0; i < 2; i++)
    {
        if(appuis(boutons[i]))
        {
            bouton = boutons[i];
            break;
        }
    }
    commanderLeds(bouton);
    // Bouton 8 -> Led SPI (envoyer) : (i2cIo.Read(GPIO) & 0x0F) << 4
    // Bouton 8 -> Les SPI (reception) : spiIo.Write(GPIO, donneeRecue);
    
    // Bouton 7 -> Led I2C (envoyer) : (i2cIo.Read(GPIO) & 0x0F) << 4 (oui, c'est la même chose que l'autre bouton)
    // Bouton 7 -> Led I2C (reception) : i2cIo.Write(GPIO, donneeRecue);
    
}

boolean appuis(int bouton)
{
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
void commanderLeds(int bouton)
{
    switch(bouton)
    {
        case 0:
            i2cIo.Write(GPIO, 0b1111 << 4);
            spiIo.Write(GPIO, 0b1111 << 4);
            break;
        case 7:
            //swState = (i2cIo.Read(GPIO) & 0x0F) << 4;
            swState = i2cIo.Read(GPIO);
            //Serial.println(swState, BIN);
            swState = (swState & 0x0F) << 4;
            Serial.println(swState, BIN);
            i2cIo.Write(GPIO, swState);
            break;
        case 8:
            //swState = (i2cIo.Read(GPIO) & 0x0F) << 4;
            swState = i2cIo.Read(GPIO);
            //Serial.println(swState, BIN);
            swState = (swState & 0x0F) << 4;
            Serial.println(swState, BIN);
            spiIo.Write(GPIO, swState);
            break;
        default:
            Serial.println("Ce bouton ne correspon à aucune commande de Led");
    }
}
