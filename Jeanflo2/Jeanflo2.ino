////////////////// Voir à la fin utiliser les filtres si reste temps

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
const int nb_nodes_max = 4;                  //////  15
int nb_nodes_activees;
int my_node = 3;
uint8_t list_nodes[nb_nodes_max];
int compteur;
bool initialisation;
bool attente;
unsigned long temps1;
unsigned long temps2;
byte pinPoten = 3;
int valeur_poten;
byte LSB_poten;
byte MSB_poten;
int numero_list;
int destinataire = 0;

void setup() {
  pinMode(3, INPUT);
  i2c_io.Write(IOCON, 0x04);   // makes I2C interrupt pin open-drain

  Serial.begin(9600);
  attachInterrupt(0, somethingReceived, FALLING);  // int received on pin 2 if JMP16 is in position A

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("RX-TX ");
  lcd.print(msgID,HEX);
  opmode = canutil.whichOpMode();
  //lcd.setCursor(0,1);
  lcd.print(" mode ");
  lcd.print(opmode, DEC);

  canutil.setOpMode(OPMODE_CONFIG); // sets configuration mode
  canutil.waitOpMode(OPMODE_CONFIG);  // waits configuration mode

  canutil.flashRxbf();  //just for fun!


  //can.write(CANINTE,0x01);  //disables all interrupts but RX0IE (received message in RX buffer 0)
  can_dev.write(CANINTE, 0x03);
  can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags
  canutil.setClkoutMode(CLKOUT_DISABLED, CLKOUT_DIV_1); // disables CLKOUT
  canutil.setTxnrtsPinMode(PIN_MODE_ALL_PURPOSE, PIN_MODE_ALL_PURPOSE, PIN_MODE_ALL_PURPOSE); // all TXnRTS pins as all-purpose digital input


  // Bit timing section
  //  setting the bit timing registers with Fosc = 16MHz -> Tosc = 62,5ns
  // data transfer = 125kHz -> bit time = 8us, we choose arbitrarily 8us = 16 TQ  (8 TQ <= bit time <= 25 TQ)
  // time quanta TQ = 2(BRP + 1) Tosc, so BRP =3
  // sync_seg = 1 TQ, we choose prop_seg = 2 TQ
  // Phase_seg1 = 7TQ yields a sampling point at 10 TQ (60% of bit length, recommended value)
  // phase_seg2 = 6 TQ SJSW <=4 TQ, SJSW = 1 TQ chosen
  can_dev.write(CNF1, 0x03); // SJW = 1, BRP = 3
  can_dev.write(CNF2, 0b10110001); //BLTMODE = 1, SAM = 0, PHSEG = 6, PRSEG = 1
  can_dev.write(CNF3, 0x05);  // WAKFIL = 0, PHSEG2 = 5



  // Settings for buffer RXB0
  //canutil.setRxOperatingMode(3, 1, 0);  // mask off  and rollover
  canutil.setRxOperatingMode(RXMODE_STDONLY, ROLLOVER_ENABLE, RX_BUFFER_0);  // standard ID messages only  and rollover
//  canutil.setAcceptanceFilter(0x2AB, 2000, NORMAL_FRAME, RX_ACCEPT_FILTER_0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 1 = extended, filter# 0
//  canutil.setAcceptanceFilter(0x2AC, 2001, NORMAL_FRAME, RX_ACCEPT_FILTER_1); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 1 = extended, filter# 1
//  canutil.setAcceptanceMask(0xFFFF, 0x00000000, RX_BUFFER_0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, buffer# 0
  canutil.setAcceptanceMask(0x000, 0x00000000, RX_BUFFER_0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, buffer# 0 ////////////////
  // in this case, only messages with ID equal to 0x2AB or 0x2AC will be accepted since mask is set to 0xFFF
  // for example, if mask is set to 0xFF0, all the message with ID beginning with 0x2A will be accepted


  // Settings for buffer RXB1
  canutil.setRxOperatingMode(RXMODE_STDONLY, ROLLOVER_ENABLE, RX_BUFFER_1);  // std  ID messages  rollover 
//  canutil.setAcceptanceFilter(0x2AA, 2002, NORMAL_FRAME, RX_ACCEPT_FILTER_2); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 
//  canutil.setAcceptanceFilter(0x2AA, 2003, NORMAL_FRAME, RX_ACCEPT_FILTER_3); // 0 <= stdID <= 2047, 0 <= extID <= 262143,
//  canutil.setAcceptanceFilter(0x2AA, 2004, NORMAL_FRAME, RX_ACCEPT_FILTER_4); // 0 <= stdID <= 2047, 0 <= extID <= 262143,
//  canutil.setAcceptanceFilter(0x2AA, 2005, NORMAL_FRAME, RX_ACCEPT_FILTER_5);// 0 <= stdID <= 2047, 0 <= extID <= 262143,
//  canutil.setAcceptanceMask(0xFFF, 0xFFFFFFFF, RX_BUFFER_1); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 
  canutil.setAcceptanceMask(0x000, 0xFFFFFFFF, RX_BUFFER_1); // 0 <= stdID <= 2047, 0 <= extID <= 262143,   //////////////

  canutil.setOpMode(OPMODE_NORMAL); // sets normal mode
  opmode = canutil.whichOpMode();
  lcd.setCursor(15, 0);
  lcd.print(opmode, DEC);

  canutil.setTxBufferID(msgID, 2000, NORMAL_FRAME, TX_BUFFER_0); // TX standard messsages with buffer 0
  canutil.setTxBufferDataLength(SEND_DATA_FRAME, 1, TX_BUFFER_0); // TX normal data, 1 byte long, with buffer 0
  


  for (int i = 0; i < 8; i++) {
    tosend[i] = 0;
    list_nodes[i] = 0;
  }

  lcd.setCursor(0, 1);
  lcd.print("                ");


  isInt = 0;
  can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags
  push = 0;

  i2cIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)
  spiIo.Write(IODIR, 0x0F);   // sets I2C port direction for individual bits (Je sais pas ce que çça fait, mais c'est nécessaire)

  initialisation = false;

   delay(1000);

}



void loop() {

  while (initialisation == false){
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
      }
  }


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
    /*Serial.print("nodes activees : ");
    Serial.println(nb_nodes_activees);*/
    afficher_liste();
  }
  //Serial.println("");

  //afficher_liste();


  /*if ( digitalRead(3) == 0 && push == 0) {
    sendMessage();
    push = 1;
  }


  if ( digitalRead(3) == 1) {
  push = 0;
}

 if (isInt==1){
  displayMessage();
 }*/


}






//******************************************************************
//                     other routines
//******************************************************************

void Master(){

  nb_nodes_activees = 0;
  //delay(5000);                            // Remettre délai 5s debut
  msgID = 0x100;
  canutil.setTxBufferID(msgID, 2000, NORMAL_FRAME, TX_BUFFER_0);
  canutil.setTxBufferDataLength(SEND_DATA_FRAME, 1, TX_BUFFER_0);
  Serial.println("initialisation trame");
  
  for(compteur = 1; compteur < nb_nodes_max + 1; compteur++){
    lcd.setCursor(11, 0);
    lcd.print(compteur);
    if(nb_nodes_activees < 8){
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
    }
    Serial.println("");
  }

  /*for(compteur = nb_nodes_activees; compteur < nb_nodes_max; compteur++){
    list_nodes[compteur] = 0;
  }*/
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


void sendMessage() {
  tosend[0]++;
  canutil.setTxBufferDataField(tosend, TX_BUFFER_0);   // fills TX buffer
  //Serial.println("setTx");
  canutil.messageTransmitRequest(TX_BUFFER_0, TX_REQUEST, TX_PRIORITY_HIGHEST); // requests transmission of buffer 0 with highest priority
  //Serial.println("msgTx");

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
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 3, TX_BUFFER_0);
              break;
    case 7:   tosend[1] = 0x01;
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 3, TX_BUFFER_0);
              break;
    case 6:   tosend[1] = 0x02;
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 2, TX_BUFFER_0);
              Serial.print("Destinataire : ");
              Serial.println(destinataire);
              Serial.println("Demande poten envoyee");
              break;
              /*while(isInt == 0){
                continue;
              }
              isInt = 0;
              can_dev.write(CANINTF, 0x00);
              Reaction();*/
              
    case 5:  tosend[1] = 0x03;
              tosend[2] = LSB_poten;
              tosend[3] = MSB_poten;
              canutil.setTxBufferDataLength(SEND_DATA_FRAME, 4, TX_BUFFER_0);
              Serial.println("Reponse poten envoyee");
              break;
  }  
  canutil.setTxBufferDataField(tosend, TX_BUFFER_0);
  canutil.messageTransmitRequest(TX_BUFFER_0, TX_REQUEST, TX_PRIORITY_HIGHEST);  
  delay(1000);

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
    switch (recData[1]){
      case 0x02:  destinataire = canutil.whichStdID(RX_BUFFER_0);
                    destinataire -= 0x200;
                    valeur_poten = analogRead(pinPoten);
                    Serial.print("Valeur poten envoyee : ");
                    Serial.println(valeur_poten);
                    LSB_poten = valeur_poten % 256;
                    MSB_poten = valeur_poten / 256;
                    Action(5);
                    break;
      case 0x03:  valeur_poten = recData[3] * 256 + recData[2];
                    Serial.print("Valeur poten recue : ");
                    Serial.println(valeur_poten);
                    lcd.clear();
                    lcd.write("Potentiometre");
                    lcd.setCursor(0, 1);
                    lcd.write("valeur --> ");
                    lcd.setCursor(12, 1);
                    lcd.print(valeur_poten);
                    while(!appuis(5)){
                      continue;
                    }
                    break;
    }
    
  }
}


void displayMessage() {
  isInt = 0; // resets interrupt flag

  can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags

  recSize = canutil.whichRxDataLength(RX_BUFFER_0); // checks the number of bytes received in buffer 0 (max = 8)

  for (int i = 0; i < recSize; i++) { // gets the bytes
    recData[i] = canutil.receivedDataValue(RX_BUFFER_0, i);
  }


  lcd.setCursor(0, 1);
  lcd.print("rec data =      ");
  lcd.setCursor(10, 1);
  lcd.print(recData[0], HEX);
  lcd.print(" Hex");
}


void afficher_liste(){
  valeur_poten = analogRead(pinPoten);
  //numero_list = map(valeur_poten, 0, 1023, 0, nb_nodes_activees - 1);
  numero_list = (int) ((valeur_poten * nb_nodes_activees) / 1023);
  Serial.println(numero_list);
  Serial.println(valeur_poten);
  Serial.println(nb_nodes_activees);
  Serial.println("");
  if(numero_list == nb_nodes_activees){
    numero_list--;
  }
  destinataire = list_nodes[numero_list];
  Serial.println(numero_list);
  lcd.clear();
  lcd.write("Node choisie : ");
  lcd.setCursor(0, 1);
  lcd.write("numero ");
  lcd.setCursor(8, 1);
  lcd.print(destinataire);
  delay(500);
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
