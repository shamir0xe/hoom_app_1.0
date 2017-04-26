

/**                    IN THE NAME OF GOD
 * HOOM.co
 * Farid Lotfi
 * ----------------------------------------------------------------------------
 * This is access control with RFID module MFRC522 code;
 * 
 * NOTE: The library file MFRC522.h has a lot of useful info. Please read it.
 * 
 * ----------------------------------------------------------------------------
 * 
 * BEWARE: Data will be written to the PICC, in sector #1 (blocks #4 to #7).
 * 
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno           Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 * 
 */
 /*
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 * 
  */
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>

#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
#define dataBaseSize    7           // size of the users of a door
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;


MFRC522::MIFARE_Key keys[2132];
struct  dataBase {
    byte    ID[dataBaseSize][4] ;  // Number of bytes in the UID. 4, 7 or 10.
    byte    sKey[dataBaseSize][6];     // specific key for the ID
  };
dataBase my_dataBase;
byte sector;
byte blockAddr;        
byte trailerBlock = 15;
byte buffer[18];
byte size = sizeof(buffer);
byte randByte;
byte newKey [16];
byte newTrailer[16];
byte cardLocation;
byte dataBasePointer = 2; //0 is for add card and 1 is for remove card
byte dataBlock[] = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0x07,  
        0x80, 0x69, 0xFF, 0xFF,  
        0xFF, 0xFF, 0xFF, 0xFF
    };
byte  removeCard_data[] = {
        0x51, 0x21, 0x25, 0x36,
        0x29, 0x51, 0x25, 0x21,
        0x29, 0x36, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
      };
byte  addCard_data[] = {
        0x51, 0x21, 0x25, 0x36,
        0x29, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
      };
File myFile;
enum data : byte{
  KEYDATA = 0,
  ID = 1
};
int openButton = 7;
byte a;
byte myCursor = 0;
byte num = 0;
byte modeData = ID;
/**
 * Initialize.
 */
void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("database.txt", FILE_WRITE);
  // close the file:
  myFile.close();
  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }
  pinMode(openButton,OUTPUT);
  digitalWrite(openButton,LOW);
  readFromCard();
  randomSeed(analogRead(6));
}
 
/**
 * Main loop.
 */
void loop() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
      return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
      return;

  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  MFRC522::StatusCode status;
  if ( AddCheck() )
    return;
  if ( RemoveCheck() )
    return;
  printDataBase();
  Serial.println(F("nightsWatch is ready for next card")); Serial.println();
  if ( nightsWatch() )
  {
    keyChanger (cardLocation);
    writeToCard();
    Welcome();
    delay(5000);
  }
  else
  {
    NotWelcome();
    delay(5000);
  }
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}
/**
 * Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
/**
* printDataBase
*/
void printDataBase ()
{
  Serial.println();
  Serial.print(F("printing DataBase...")); Serial.println();
  Serial.print(F("number of cards in database:"));
  Serial.println(dataBasePointer); Serial.println();
  Serial.print(F("cardNumber")); Serial.print("\t");
  Serial.print(F("ID")); Serial.print("\t");Serial.print("\t");
  Serial.print(F("sKey")); Serial.print("\t"); Serial.println();
  for ( byte i = 0 ; i < dataBasePointer ; i++ )
  {
    Serial.print(i); Serial.print("\t");Serial.print("\t");
    dump_byte_array(my_dataBase.ID[i],4); Serial.print("\t");
    dump_byte_array(my_dataBase.sKey[i],6); Serial.print("\t");
    Serial.println();
  }
}
/**
* ReadBlock
*/
void ReadBlock ( byte MYblockAddr ) // felan ba in farz k buffer kharab nemishe vagarna k in nabayad void bashe o output dare
{
   MFRC522::StatusCode status;
   // Read data from the block
  Serial.print(F("Reading data from block ")); Serial.print(MYblockAddr);
  Serial.println(F(" ..."));
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(MYblockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) 
  {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data in block ")); Serial.print(MYblockAddr); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();
}
/**
 * array equality check
 */
 boolean ArrCheck (byte Arr1[] ,byte Arr2[] ,byte arrSize)
 {
  boolean result;
  for (byte i=0 ; i < arrSize ; i++)
  {
    if (Arr1[i] == Arr2[i])
      result = true;//thats ok lets go for next i
    else
      return false;
  }
  return true;
 }
//***
  //key generator 
void keyGen ( void )
  {
    for ( byte i = 0 ; i < 6 ; i++ ) // key_A is 6 bytes
    {
      randByte = random(0xFF); // making a random byte
      newKey[i] = randByte;
    }
  }
//***//

//**
  // masking trailer with new key_A
void trailerMasker ( void )
{
    for ( byte i = 0 ;  i < 16 ; i++ )
    {
      if ( i < 6 )
      {
        newKey[i] = newKey[i] | 0x00;
        dataBlock[i] = 0xFF;
      }
      else
        newKey[i] = newKey[i] | 0xFF;
    }
    for ( byte i = 0 ; i < 16 ; i++ )
      dataBlock[i] = newKey[i] & dataBlock[i]; // now our block is refreshed with a new key_A
}
/////
/**
* authenticating with key_A
*/
boolean authenticate_keyA (byte cardNumber)
{
  // Authenticate using key A
  MFRC522::StatusCode status;
  for (byte i = 0; i < 6; i++) 
  {
    key.keyByte[i] = my_dataBase.sKey[cardNumber][i];
  }
  Serial.println(F("Authenticating using key A...")); Serial.println();
  dump_byte_array(key.keyByte,6); Serial.println();
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) 
  {
    Serial.print(F("Authenticate_with_keyA failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  else
  {
    Serial.print(F("Authenticate_with_keyA passed: "));
    return true;
  }
}

/**
 * AddCardIdentifier
 */
 boolean AddCardIdentifier ( void )
 {
     MFRC522::StatusCode status;
     for (byte i = 0; i < 6; i++)
     {
      key.keyByte[i] = 0xFF;
     }
      // Authenticate using key A
      trailerBlock = 11; //hamishe saabet chon too block 2 data neveshti
      Serial.println(F("Authenticating using key A..."));
      status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) 
      {
        Serial.println(F("PCD_Authenticate() failed: "));
        Serial.println(F("sorry, its not an add card "));
        return false;
      }
      else
      {
        Serial.print(F("mmm, it can be an add card "));
        ReadBlock (8);
        boolean result;
        result = ArrCheck (buffer, addCard_data, 16);
        if ( result == true )
        {
          Serial.println(F("ooo,yeah, thats an add card "));
          return true;
        }
        else
        {
          Serial.println(F("oops, its not an add card "));
          return false;
        }
      }
  }
/**
 * checks if this is a remove card
 */
 boolean RemoveCardIdentifier ( void )
 {
     MFRC522::StatusCode status;
     for (byte i = 0; i < 6; i++)
     {
      key.keyByte[i] = 0xFF;
     }
      // Authenticate using key A
      trailerBlock = 11; //hamishe saabete ahmagh chon too block 2 data neveshti
      Serial.println(F("Authenticating using key A..."));
      status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) 
      {
        Serial.println(F("PCD_Authenticate() failed: "));
        Serial.println(F("sorry, its not a remove card "));
        return false;
      }
      else
      {
        Serial.println(F("mmm, it can be a remove card "));
        ReadBlock (8);
        boolean result;
        result = ArrCheck (buffer, removeCard_data, 16);
        if ( result == true )
        {
          Serial.println(F("ooo,yeah, thats a remove card "));
          return true;
        }
        else
        {
          Serial.println(F("oops, its not a remove card "));
          return false;
        }
      }
  }
/**
* AddToDataBase
*/
void AddToDataBase ()
{
  MFRC522::StatusCode status;
  boolean ID_result = false;
  for ( byte j = 0; j < dataBasePointer; j++ )
  {
    ID_result = ArrCheck (my_dataBase.ID[j], mfrc522.uid.uidByte, 4);
    if ( ID_result == true ) // it means this card is one of our cards
    {
      Serial.println(F("we already have it in our database... "));
      return;
    }
  }
  // it means this card is not in our database and ID_result has stayed false
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }
  // Authenticate using key A
  trailerBlock = 15;
  Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) 
  {
    Serial.println(F("PCD_Authenticate() failed: "));
    Serial.println(F("sorry, this card (this sector) has been used before for some other application, buy a new one... "));
    return;
  }
  //every thing is ok lets add it
  keyGen();
  // put the key into the card
  trailerMasker();
  WriteToblock(3,15); // we write to 6th sector because I like it :D when we had more applications for a card we need a plan
  // now save the card into our dataBase
  for ( byte i = 0 ; i < 4 ; i++) // adding ID to the data base
    my_dataBase.ID [dataBasePointer][i] =  mfrc522.uid.uidByte[i];
  for ( byte i = 0 ; i < 6 ; i++) // adding sKey to the data base
    my_dataBase.sKey [dataBasePointer][i] = newKey[i];
  dataBasePointer++;
  printDataBase();
// Halt PICC
mfrc522.PICC_HaltA();
// Stop encryption on PCD
mfrc522.PCD_StopCrypto1();
}
/**
* RemoveFromDataBase
*/
void RemoveFromDataBase ()
{
  boolean ID_result = false;
  boolean authenticate_result;
  for ( byte j = 2; j < dataBasePointer; j++ )
  {
    ID_result = ArrCheck (my_dataBase.ID[j], mfrc522.uid.uidByte, 4);
    if ( ID_result == true ) // it means this card is one of our cards
    {
      Serial.println(F("removing card with j = ... ")); Serial.print(j); Serial.println();
      authenticate_result = authenticate_keyA(j);
      if (authenticate_result == true)
      {
        for (byte i = 0; i < 6; i++)
          {
            newKey[i] = 0xFF;
          }
        trailerMasker();
        WriteToblock(3,15); // we change 7th sector as it was at chip delivery
        Serial.print(F("card key changed successfully (card removed)")); Serial.println();
        //now remove the card from database
        byte cardNum = j;
        for( cardNum ; cardNum < dataBasePointer ;  cardNum++ )
        {
          for ( byte i = 0 ; i < 4 ; i++) // removing ID from the data base
            my_dataBase.ID [cardNum][i] =  my_dataBase.ID [cardNum + 1][i];
          for ( byte i = 0 ; i < 6 ; i++) // removing sKey from the data base
            my_dataBase.sKey [cardNum][i] = my_dataBase.sKey [cardNum + 1][i];
        }
        dataBasePointer--; //akhario paak nakardam vali khob hamin -- kardane pointer dar vaghe oono az beyn mibare
        Serial.print(F("card removed successfully... ")); Serial.println();
        printDataBase();
        return;
      }
      Serial.println(F("ID is in the database but the key is not correct "));
      return;
    }
  }
  // it means this card is not in our database and ID_result has stayed false
  Serial.println(F("this card is not in our database ")); Serial.println();
  printDataBase();
}
/**  
 *   write to block
 */
 void WriteToblock ( byte MYsector, byte MYblockAddr )
 {
    MFRC522::StatusCode status;
    byte buffer[16];
    byte size = sizeof(buffer); 
   // Write data to the block
    Serial.println(F("Writing data into block ")); Serial.print(MYblockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(MYblockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.println(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print(F("Reading data from block ")); Serial.print(MYblockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(MYblockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(MYblockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
        
    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println(F("Checking result..."));
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print(F("Number of bytes that match = ")); Serial.println(count);
    if (count == 16) {
        Serial.println(F("Success :-)"));
    } else {
        Serial.println(F("Failure, no match :-("));
        Serial.println(F("  perhaps the write didn't work properly..."));
    }
    Serial.println();
        
    // Dump the sector data
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, MYsector);
    Serial.println();
}
/**
* checks if the user wants to add a card to database
*/
boolean AddCheck()
{
//lets add a card
MFRC522::StatusCode status;
boolean add_temp;
add_temp = AddCardIdentifier();
  if ( add_temp == true )
  { // add the ADD CARD to databse
    // check database0 maybe the add card had been added before
    if ( dataBasePointer == 0 )//in ghalate!!!! hamishe true e
    {
      Serial.println(F("You have added your add card to database before...")); 
      Serial.println();
    }
    else
    {
      for ( byte i = 0 ; i < 4 ; i++ ) // adding the ID of add card to first place of data base
      {
        my_dataBase.ID[0][i] = mfrc522.uid.uidByte[i];
      }
      for ( byte i = 0 ; i < 6 ; i++ ) // adding the sKey of add card to first place of data base
      {
        my_dataBase.sKey[0][i] = 0xFF;
      }
      Serial.println(F("ADD CARD added successfully...")); Serial.println();
    }
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
    byte state = 0;
    unsigned long time1;
    unsigned long time2;
    while (1) // we wait for another card to come
    {
      if (state == 0)
      {
        time1 = millis();
        state = 1; 
      } 
      else if (state == 1)
      {
        time2 = millis();
        Serial.println(F("waiting for the second card to come")); Serial.println();
        state = 2;
      }
      else if (state == 2)
      {
        if ( mfrc522.PICC_IsNewCardPresent() )
        {
          Serial.println(F("another card is coming")); Serial.println();
          if ( !mfrc522.PICC_ReadCardSerial() )
          {
            Serial.println(F("we couldnt select it, hold it closer")); Serial.println();
            continue;
          }
          Serial.println(F("and yeah it is selected baby")); Serial.println();
          //now add second cards ID and key to data base
          AddToDataBase();
          break;
        }
      }
    }
    // if ( time2 - time1 > 10000 ) //its been more than 10 seconds from we came too the loop
     // {
     //   Serial.println(F("10 seconds passed, you are too slow buddy, I can't wait more...")); Serial.println();
      //  break;
     // }
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
    return add_temp;
  }
writeToCard();
}
/**
* checks if the User wants to remove a card from data base
*/
boolean RemoveCheck()
{
  //lets remove a card
  MFRC522::StatusCode status;
  boolean remove_temp;
  remove_temp = RemoveCardIdentifier();
  if ( remove_temp == true )
  { // add the REMOVE CARD from databse
    // check database0 maybe the add card had been added before
    if ( dataBasePointer == 0 )//in ghalate!!!! hamishe true e
    {
      Serial.println(F("You have added your add card to database before...")); 
      Serial.println();
    }
    else
    {
      for ( byte i = 0 ; i < 4 ; i++ ) // adding the ID of remove card to second place of data base
      {
        my_dataBase.ID[1][i] = mfrc522.uid.uidByte[i];
      }
      for ( byte i = 0 ; i < 6 ; i++ ) // adding the sKey of remove card to second place of data base
      {
        my_dataBase.sKey[1][i] = 0xFF;
      }
      Serial.println(F("REMOVE CARD added successfully...")); Serial.println();
    }
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
    byte state = 0;
    unsigned long time1;
    unsigned long time2;
    while (1) // we wait for another card to come
    {
      if (state == 0)
      {
        time1 = millis();
        state = 1; 
      } 
      else if (state == 1)
      {
        time2 = millis();
        Serial.println(F("waiting for the second card to come")); Serial.println();
        state = 2;
      }
      else if (state == 2)
      {
        if ( mfrc522.PICC_IsNewCardPresent() )
        {
          Serial.println(F("another card is coming")); Serial.println();
          if ( !mfrc522.PICC_ReadCardSerial() )
          {
            Serial.println(F("we couldnt select it, hold it closer")); Serial.println();
            continue;
          }
          Serial.println(F("and yeah it is selected baby")); Serial.println();
          //now remove second cards ID and key from data base
          trailerBlock = 15;
          RemoveFromDataBase();
          break;
        }
      }
    }
    // if ( time2 - time1 > 10000 ) //its been more than 10 seconds from we came too the loop
     // {
     //   Serial.println(F("10 seconds passed, you are too slow buddy, I can't wait more...")); Serial.println();
      //  break;
     // }
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
    return remove_temp;
  }
writeToCard();
}
/**
* nightsWatch :D cheking the card ID and if we had it check the key and authenticate and output T/F
*/
boolean nightsWatch ()
{
  boolean ID_result = false;
  boolean authenticate_result;
  trailerBlock = 15;
  for ( byte j = 0; j < dataBasePointer; j++ )
  {
    ID_result = ArrCheck (my_dataBase.ID[j], mfrc522.uid.uidByte, 4);
    if ( ID_result == true ) // it means this card is one of our cards
    {
      authenticate_result = authenticate_keyA(j);
      cardLocation = j;
      return authenticate_result;
    }
  }
  // it means this card is not in our database and ID_result has stayed false
  return false;
}
/**
* you have a correct card you can enter
*/
void Welcome()
{
  Serial.println(F("you have a correct card you can enter...")); Serial.println();
  digitalWrite(openButton,HIGH);
}
/**
* you Dont have a correct card , go home Dear thief
*/
void NotWelcome()
{
  Serial.println(F("you Dont have a correct card , go home Dear thief...")); Serial.println();
  digitalWrite(openButton,LOW);
}
/**
* When a card is been used, this function changes its key
* This function needs the location of card in the database as an input
*/
void keyChanger (byte cardLoc)
{
  //every thing is ok lets add it
  keyGen();
  // put the key into the card
  trailerMasker();
  WriteToblock(3,15); // we write to third sector because I like it :D when we had more applications for a card we need a plan
  // now change the key into our dataBase
  for ( byte i = 0 ; i < 6 ; i++) // changing sKey in the data base
    my_dataBase.sKey [cardLoc][i] = newKey[i];
}
void readFromCard (){
  
  // re-open the file for reading:
  myFile = SD.open("database.txt");
  if (myFile) {
    Serial.println("database.txt:");

    // read from the file until there's nothing else in it:
    myCursor = 0;
    num = 0;
    while (myFile.available()) {
      a = myFile.read();
      Serial.println(a);
      if (a == '\t'){
        modeData = KEYDATA;
        myCursor = 0;
      }
      else if (a == '\n'){
        modeData = ID;
        num++;
        myCursor = 0;
      }
      else{ //it is a valid char
        if ( modeData == ID ){
          my_dataBase.ID[num][myCursor] = a;
          myCursor++;
        }
        else if ( modeData == KEYDATA ){
          my_dataBase.sKey[num][myCursor] = a;
          myCursor++;
        }
      }
    }
    num = 0;
    printDataBase();  
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening database.txt");
  }
}
void writeToCard (){
  myFile = SD.open("database.txt");

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.println("Writing to database.txt...");
    for (int i=0 ; i<dataBasePointer ; i++){
      myFile.write(my_dataBase.ID[i],4);
      myFile.print("\t");
      myFile.write(my_dataBase.sKey[i],6);
      myFile.print("\n");  
    }
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening databse.txt");
  }
}
