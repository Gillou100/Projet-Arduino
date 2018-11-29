//************************************************************
// normal mode, sends and recs one byte, std messages
//
//************************************************************

#include <MCP23008.h>
#include <MCP23S08.h>
#include <MCP2510.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <SPI.h>
#include <Canutil.h>

#define MCP23008_ADDR 0x00  // Adresse I2C par défaut
#define MCP23S08_ADDR 0x00  // Adresse SPI par défaut
#define MASQ5  0b1000   // Masque pour reconnaitre les appuis sur le bouton 5
#define MASQ6  0b0100   // Masque pour reconnaitre les appuis sur le bouton 6
#define MASQ7  0b0010   // Masque pour reconnaitre les appuis sur le bouton 7
#define MASQ8  0b0001   // Masque pour reconnaitre les appuis sur le bouton 8
#define MASQ9  0b1000   // Masque pour reconnaitre les appuis sur le bouton 9
#define MASQ10 0b0100   // Masque pour reconnaitre les appuis sur le bouton 10
#define MASQ11 0b0010   // Masque pour reconnaitre les appuis sur le bouton 11
#define MASQ12 0b0001   // Masque pour reconnaitre les appuis sur le bouton 12

MCP2510  can_dev(9); // defines pb0 (arduino pin8) as the _CS pin for MCP2510
LiquidCrystal lcd(15, 0, 14, 4, 5, 6, 7);  //  4 bits without R/W pin
MCP23008 i2c_io(MCP23008_ADDR);         // Init MCP23008
Canutil  canutil(can_dev);
MCP23008 i2cIo(MCP23008_ADDR);      // Objet pour le protocole I2C
MCP23S08 spiIo(MCP23S08_ADDR, 10);  // Objet pour le protocole SPI

uint8_t opmode, txstatus;
volatile int isInt;
uint8_t tosend[8];
uint8_t recSize, recData[8];
uint8_t push = 1;
uint16_t msgID = 0x2AB;
uint8_t swState;
const int nb_nodes_max = 15;
int nb_nodes_activees;
int my_node = 9;
uint8_t list_nodes[nb_nodes_max];
int compteur;
bool initialisation;
bool attente;
unsigned long temps1;
unsigned long temps2;
byte pinPoten = 3;
int ancienne_valeur_poten;
int valeur_poten;
byte LSB_poten;
byte MSB_poten;
int numero_list;
int destinataire = 0;

void setup() {
  i2c_io.Write(IOCON, 0x04);   // makes I2C interrupt pin open-drain

  Serial.begin(9600);
  lcd.begin(16, 2);
  attachInterrupt(0, somethingReceived, FALLING);  // int received on pin 2 if JMP16 is in position A

  canutil.setOpMode(OPMODE_CONFIG); // sets configuration mode
  canutil.waitOpMode(OPMODE_CONFIG);  // waits configuration mode

  canutil.flashRxbf();  //just for fun!

  can_dev.write(CANINTE, 0x03);
  can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags
  canutil.setClkoutMode(CLKOUT_DISABLED, CLKOUT_DIV_1); // disables CLKOUT
  canutil.setTxnrtsPinMode(PIN_MODE_ALL_PURPOSE, PIN_MODE_ALL_PURPOSE, PIN_MODE_ALL_PURPOSE); // all TXnRTS pins as all-purpose digital input

  can_dev.write(CNF1, 0x03); // SJW = 1, BRP = 3
  can_dev.write(CNF2, 0b10110001); //BLTMODE = 1, SAM = 0, PHSEG = 6, PRSEG = 1
  can_dev.write(CNF3, 0x05);  // WAKFIL = 0, PHSEG2 = 5

  canutil.setRxOperatingMode(RXMODE_STDONLY, ROLLOVER_ENABLE, RX_BUFFER_0);
  canutil.setRxOperatingMode(RXMODE_STDONLY, ROLLOVER_ENABLE, RX_BUFFER_1);
  canutil.setOpMode(OPMODE_NORMAL);


  for (int i = 0; i < 8; i++) {
    tosend[i] = 0;
    list_nodes[i] = 0;
  }

  isInt = 0;

  i2cIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits
  spiIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits

  initialisation = false;

  isInt = 0;
  can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags
  push = 0;

  delay(1000);

}



void loop() {

  while (initialisation == false){
      canutil.setAcceptanceFilter(0x100, 2001, NORMAL_FRAME, RX_ACCEPT_FILTER_0);
      canutil.setAcceptanceMask(0xFF0, 0x00000000, RX_BUFFER_0);
      lcd.clear();
      lcd.write("Attente");
      delay(500);
      if(appuis(9)){
        lcd.clear();
        lcd.write("Master --> ");
        Master();
      }
      else if (isInt == 1){
        isInt = 0;
        can_dev.write(CANINTF, 0x00);
        reponse_Master();
        Serial.print("nodes activees : ");
        Serial.println(nb_nodes_activees);
      }
  }

  canutil.setAcceptanceFilter(0x200, 2001, NORMAL_FRAME, RX_ACCEPT_FILTER_0);
  canutil.setAcceptanceMask(0xF00, 0x00000000, RX_BUFFER_0);

  if (isInt == 1){
    isInt = 0;
    can_dev.write(CANINTF, 0x00);
    Serial.println("Message recu");
    Reaction();
  }
  else if(appuis(8)){
    Action(8);
  }
  else if(appuis(7)){
    Action(7);
  }
  else if(appuis(6)){
    Action(6);
  }
  else {
    temps1 = millis();
    temps2 = millis();
    while ((temps2 - temps1) < 1000){
      if (isInt == 1 || afficher_liste() || appuis(8) || appuis(7) || appuis(6)){  
          Serial.println("Sortie boucle");
          break;
        }
      temps2 = millis();
    }
    if (isInt == 0){
      spiIo.Write(GPIO, 0b0000 << 4);
      i2cIo.Write(GPIO, 0b0000 << 4);
    }
  }

}




//******************************************************************
//                     other routines
//******************************************************************


void Master(){
  canutil.setAcceptanceFilter(0x101, 2001, NORMAL_FRAME, RX_ACCEPT_FILTER_0);
  canutil.setAcceptanceMask(0xFFF, 0x00000000, RX_BUFFER_0);
  nb_nodes_activees = 0;
  delay(5000);                            // Remettre délai 5s debut
  msgID = 0x100;
  canutil.setTxBufferID(msgID, 2000, NORMAL_FRAME, TX_BUFFER_0);
  canutil.setTxBufferDataLength(SEND_DATA_FRAME, 1, TX_BUFFER_0);
  Serial.println("initialisation trame");
  
  for(compteur = 1; compteur < nb_nodes_max + 1; compteur++){
    if(nb_nodes_activees == 8){
      break;
    }
    lcd.setCursor(11, 0);
    lcd.print(compteur);
    if (compteur != my_node){
      
      // envoie
      tosend[0] = compteur;
      canutil.setTxBufferDataField(tosend, TX_BUFFER_0);
      canutil.messageTransmitRequest(TX_BUFFER_0, TX_REQUEST, TX_PRIORITY_HIGHEST);
      Serial.println("initialisation envoie");
      do {
        txstatus = canutil.isTxError(TX_BUFFER_0);  // checks tx error
        Serial.print("TX error = ");
        Serial.println(txstatus, DEC);
        txstatus = canutil.isArbitrationLoss(TX_BUFFER_0);  // checks for arbitration loss
        Serial.print("arb. loss = ");
        Serial.println(txstatus, DEC);
        txstatus = canutil.isMessageAborted(TX_BUFFER_0);  // ckecks for message abort
        Serial.print("TX abort = ");
        Serial.println(txstatus, DEC);
        txstatus = canutil.isMessagePending(TX_BUFFER_0);   // checks transmission
        Serial.print("mess. pending = ");
        Serial.println(txstatus, DEC);
        delay(500);
      }
      while (txstatus != 0);

      // réponse
      attente = true;
      temps1 = millis();
      temps2 = millis();
      Serial.println("avant reponse");
      while (attente == true && (temps2 - temps1) < 2000){
        if (isInt == 1){
          can_dev.write(CANINTF, 0x00);
          msgID = canutil.whichStdID(RX_BUFFER_0);
          if(msgID == 0x101){ 
            recSize = canutil.whichRxDataLength(RX_BUFFER_0);
            for (int i = 0; i < recSize; i++) {
              recData[i] = canutil.receivedDataValue(RX_BUFFER_0, i);
            }
            if(recData[0] == compteur){
              Serial.println("Ajout au tableau");
              list_nodes[nb_nodes_activees] = compteur;
              nb_nodes_activees++;
              isInt = 0;
              attente = false;
            }
          }
       }
         temps2 = millis();
      }
      Serial.println("apres reponse");
    }
    
    else if (compteur == my_node){
      list_nodes[nb_nodes_activees] = compteur;
      nb_nodes_activees++;
      Serial.println("c'est moi");
    }
  Serial.println("");
  }

  lcd.clear();
  lcd.write("Liste :");
  lcd.setCursor(0, 1);
  lcd.print(nb_nodes_activees);
  lcd.setCursor(2, 1);
  lcd.write("node(s)");
  delay(1000);
  if (nb_nodes_activees > 1){
    lcd.clear();
    lcd.write("Envoie liste");
    delay(1000);
    for(compteur = 0; compteur < nb_nodes_activees; compteur++){
      tosend[compteur] = list_nodes[compteur];
    }
  
    msgID = 0x102;
    canutil.setTxBufferID(msgID, 2000, NORMAL_FRAME, TX_BUFFER_0);
    canutil.setTxBufferDataLength(SEND_DATA_FRAME, nb_nodes_activees, TX_BUFFER_0);
    canutil.setTxBufferDataField(tosend, TX_BUFFER_0);
    canutil.messageTransmitRequest(TX_BUFFER_0, TX_REQUEST, TX_PRIORITY_HIGHEST);    
    do {
      txstatus = canutil.isTxError(TX_BUFFER_0);  // checks tx error
      Serial.print("TX error = ");
      Serial.println(txstatus, DEC);
      txstatus = canutil.isArbitrationLoss(TX_BUFFER_0);  // checks for arbitration loss
      Serial.print("arb. loss = ");
      Serial.println(txstatus, DEC);
      txstatus = canutil.isMessageAborted(TX_BUFFER_0);  // ckecks for message abort
      Serial.print("TX abort = ");
      Serial.println(txstatus, DEC);
      txstatus = canutil.isMessagePending(TX_BUFFER_0);   // checks transmission
      Serial.print("mess. pending = ");
      Serial.println(txstatus, DEC);
      delay(500);
    }
    while (txstatus != 0);
    Serial.println("tableau envoye");
    lcd.clear();
    lcd.write("Liste envoyee");
    delay(1000);
    initialisation = true;
  }
}


void reponse_Master(){
  Serial.print("Entrez dans reponse : ");
  msgID = canutil.whichStdID(RX_BUFFER_0);
  Serial.println(msgID, HEX);
  if(msgID == 0x100 || msgID == 0x102){
      lcd.clear();
      lcd.write("Message recu");
      recSize = canutil.whichRxDataLength(RX_BUFFER_0);
      for (int i = 0; i < recSize; i++) {
        recData[i] = canutil.receivedDataValue(RX_BUFFER_0, i);
      }
      if (msgID == 0x100 && recData[0] == my_node){
        tosend[0] = my_node;
        msgID = 0x101;
        canutil.setTxBufferID(msgID, 2000, NORMAL_FRAME, TX_BUFFER_0);
        canutil.setTxBufferDataField(tosend, TX_BUFFER_0);
        canutil.messageTransmitRequest(TX_BUFFER_0, TX_REQUEST, TX_PRIORITY_HIGHEST);
        Serial.println("initialisation reponse");
        do {
          txstatus = canutil.isTxError(TX_BUFFER_0);  // checks tx error
          Serial.print("TX error = ");
          Serial.println(txstatus, DEC);
          txstatus = canutil.isArbitrationLoss(TX_BUFFER_0);  // checks for arbitration loss
          Serial.print("arb. loss = ");
          Serial.println(txstatus, DEC);
          txstatus = canutil.isMessageAborted(TX_BUFFER_0);  // ckecks for message abort
          Serial.print("TX abort = ");
          Serial.println(txstatus, DEC);
          txstatus = canutil.isMessagePending(TX_BUFFER_0);   // checks transmission
          Serial.print("mess. pending = ");
          Serial.println(txstatus, DEC);
          delay(500);
        }
        while (txstatus != 0);
        Serial.println("reponse envoyee");
        lcd.clear();
        lcd.write("Reponse envoyee");
        delay(1000);
      }
      else if (msgID == 0x102){
        nb_nodes_activees = recSize;
        Serial.print("Nombre nodes activees : ");
        Serial.println(nb_nodes_activees);
        for (compteur = 0; compteur < 8; compteur++){
          if (compteur < recSize){
            Serial.println(recData[compteur]);
            list_nodes[compteur] = recData[compteur];
          }
          else {
            list_nodes[compteur] = 0;
          }
        }
        lcd.clear();
        lcd.write("Liste recue");
        delay(1000);
        initialisation = true;
     }
  }
}


void Action(int action){
  msgID = 0x200 + my_node;
  canutil.setTxBufferID(msgID, 2000, NORMAL_FRAME, TX_BUFFER_0);
  tosend[0] = destinataire;
  Serial.print("Action : ");
  Serial.println(action);
  
  switch (action){
    
    case 8:   tosend[1] = 0x00;
              tosend[2] = (i2cIo.Read(GPIO) & 0x0F) << 4;
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 3, TX_BUFFER_0);
              break;
              
    case 7:   tosend[1] = 0x01;
              tosend[2] = (i2cIo.Read(GPIO) & 0x0F) << 4;
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 3, TX_BUFFER_0);
              break;
              
    case 6:   tosend[1] = 0x02;
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 2, TX_BUFFER_0);
              Serial.print("Destinataire : ");
              Serial.println(destinataire);
              Serial.println("Demande poten envoyee");
              break;
              
    case 5:  tosend[1] = 0x03;
              tosend[2] = LSB_poten;
              tosend[3] = MSB_poten;
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 4, TX_BUFFER_0);
              Serial.println("Reponse poten envoyee");
              break;
              
  }  
  canutil.setTxBufferDataField(tosend, TX_BUFFER_0);
  canutil.messageTransmitRequest(TX_BUFFER_0, TX_REQUEST, TX_PRIORITY_HIGHEST);
  do {
    txstatus = canutil.isTxError(TX_BUFFER_0);  // checks tx error
    Serial.print("TX error = ");
    Serial.println(txstatus, DEC);
    txstatus = canutil.isArbitrationLoss(TX_BUFFER_0);  // checks for arbitration loss
    Serial.print("arb. loss = ");
    Serial.println(txstatus, DEC);
    txstatus = canutil.isMessageAborted(TX_BUFFER_0);  // ckecks for message abort
    Serial.print("TX abort = ");
    Serial.println(txstatus, DEC);
    txstatus = canutil.isMessagePending(TX_BUFFER_0);   // checks transmission
    Serial.print("mess. pending = ");
    Serial.println(txstatus, DEC);
    delay(500);
  }
  while (txstatus != 0);
}


void Reaction(){
  recSize = canutil.whichRxDataLength(RX_BUFFER_0);
  for (int i = 0; i < recSize; i++) {
    recData[i] = canutil.receivedDataValue(RX_BUFFER_0, i);
  }
  Serial.print("node recu : ");
  Serial.println(recData[0]);
  Serial.print("my_node : ");
  Serial.println(my_node);
  if(recData[0] == my_node){
    Serial.print("opMode : ");
    Serial.println(recData[1]);
    lcd.clear();
    msgID = canutil.whichStdID(RX_BUFFER_0);
    lcd.print(msgID, HEX);
    lcd.setCursor(7, 0);
    destinataire = msgID - 0x200;
    lcd.print(destinataire);
    lcd.setCursor(12, 0);
    lcd.print(recData[1], HEX);
    lcd.setCursor(0, 1);
    
    switch (recData[1]){

      case 0x00:    lcd.print(recData[2], BIN);
                    spiIo.Write(GPIO, ~recData[2]);
                    break;

      case 0x01:    lcd.print(recData[2], BIN);
                    i2cIo.Write(GPIO, ~recData[2]);
                    break;
      
      case 0x02:    valeur_poten = analogRead(pinPoten);
                    Serial.print("Valeur poten envoyee : ");
                    Serial.println(valeur_poten);
                    LSB_poten = valeur_poten % 256;
                    MSB_poten = valeur_poten / 256;
                    Action(5);
                    break;
                    
      case 0x03:    valeur_poten = recData[3] * 256 + recData[2];
                    lcd.print(valeur_poten, HEX);
                    lcd.setCursor(4, 1);
                    lcd.write("->");
                    lcd.setCursor(7, 1);
                    lcd.print(valeur_poten, DEC);
                    Serial.print("Valeur poten recue : ");
                    Serial.println(valeur_poten);
                    /*lcd.clear();
                    lcd.write("Potentiometre");
                    lcd.setCursor(0, 1);
                    lcd.write("valeur --> ");
                    lcd.setCursor(12, 1);
                    lcd.print(valeur_poten);*/
                    break;
                    
    }
  }
}


bool afficher_liste(){
  valeur_poten = analogRead(pinPoten);
  if(valeur_poten <= ancienne_valeur_poten - 10 || valeur_poten >= ancienne_valeur_poten + 10){
    numero_list = (int) ((valeur_poten * nb_nodes_activees) / 1023);
    if(numero_list == nb_nodes_activees){
      numero_list--;
    }
    destinataire = list_nodes[numero_list];
    //Serial.println(numero_list);
    lcd.clear();
    lcd.write("Node choisie : ");
    lcd.setCursor(0, 1);
    lcd.write("numero ");
    lcd.setCursor(8, 1);
    lcd.print(destinataire);
    ancienne_valeur_poten = valeur_poten;
    delay(500);
    return(true);
  }
  return(false);
}




//************************************************
// routine attached to INT pin
//************************************************


void somethingReceived()
{
  isInt = 1;
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
        case 5:
            return !(swState & MASQ5);
            break;
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
        case 10:
            return !(swState & MASQ10);
            break;
        case 11:
            return !(swState & MASQ11);
            break;
        case 12:
            return !(swState & MASQ12);
            break; 
        default:
            Serial.println("Bouton non utilisé");
            return false;
            break;
    }
}
