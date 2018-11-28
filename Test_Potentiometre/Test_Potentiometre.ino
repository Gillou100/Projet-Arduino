int valeur;
byte pinPoten = 3;
byte LS;
byte MS;

void setup()
{
    Serial.begin(9600);
}

void loop()
{
    valeur = analogRead(pinPoten);
    LS = valeur % 256;
    MS = valeur / 256;
    Serial.print(MS, HEX);
    Serial.print(" ");
    Serial.println(LS, HEX);
}
