

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
 * RST/Reset   RST      Blue    9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS) Black     10            53        D10        10               10
 * SPI MOSI    MOSI   White      11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO   Yellow      12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK    RED      13 / ICSP-3   52        D13        ICSP-3           15
 * 
 */

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <string.h>

/*
 * RFID parameters
 */
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
#define dataBaseSize    30           // size of the users of a door
#define interruptPinUp    2
#define interruptPinDown    3
#define BELL_RINGER 8
#define RESET_PIN 5
//#define UP 6
//#define DOWN 5
#define F_PIN 4
#define MOGHAABEL A0
#define MOTOR_ONE A1
#define MOTOR_TWO A2
#define MOTOR_OFF 0
#define MOTOR_ON 1023
#define MOGHAABEL_ON HIGH
#define MOGHAABEL_OFF LOW
#define DOOR_OPEN_TIME 10000
#define DELAY_BEFORE_CLOSING 1000
#define nodeMcuOpen A4
#define buzzerPin A3

class StopWatch {
public:
    unsigned long time;

    StopWatch() {
        time = millis();
    }

    void resetTime() {
        time = millis();   
    }

    unsigned long getTimeMillis() {
        return millis() - time;
    }

    unsigned long getTimeSeconds() {
        return getTimeMillis() / 1000;
    }
};

boolean output = LOW;
String bef_state = "";
StopWatch stopWatch = StopWatch();
// StopWatch cardDetected = StopWatch();
int EEPROMaddr = 0;
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
float batteryVoltage;
float negativeEdge;
float positiveEdge;
float lastbatteryVoltage;
unsigned long button_time = 0;  
unsigned long last_button_time = 0;
unsigned long beginEdgeTime = 0;
unsigned long endEdgeTime = 0;
boolean batteryMode = false;
boolean pulseStart = false;
int value;
boolean falling; // lastMove = true : rising // lastMove = false : falling
int counter;


void trace(String type, String str) {
    Serial.print(type);
    Serial.print(": ");
    Serial.println(str);
}


int getValue(boolean up, boolean f, boolean down) {
    int ret = 0;
    ret += up ? 1 : 0;
    ret *= 2;
    ret += f ? 1 : 0;
    ret *= 2;
    ret += down ? 1 : 0;
    return ret;
}


void openDoor() {
    if (bef_state != F("openDoor")) {
        trace(F("state"), F("openning door"));
        bef_state = F("openDoor");
    }
    digitalWrite(MOGHAABEL, MOGHAABEL_ON);
    analogWrite(MOTOR_ONE, MOTOR_ON);
    analogWrite(MOTOR_TWO, MOTOR_OFF);
}


void closeDoor() {
    if (bef_state != F("closeDoor")) {
        trace(F("state"), F("closing door"));
        bef_state = F("closeDoor");
    }
    digitalWrite(MOGHAABEL, MOGHAABEL_OFF);
    analogWrite(MOTOR_ONE, MOTOR_OFF);
    analogWrite(MOTOR_TWO, MOTOR_ON);
}


void stayRelax() {
    if (bef_state != F("stayRelax")) {
        trace(F("state"), F("stay still"));
        bef_state = F("stayRelax");
    }
    digitalWrite(MOGHAABEL, MOGHAABEL_OFF);
    analogWrite(MOTOR_ONE, MOTOR_OFF);
    analogWrite(MOTOR_TWO, MOTOR_OFF);
}


void bellTrigger() {
    if (stopWatch.getTimeMillis() < DOOR_OPEN_TIME)
        return;
    trace(F("alert"), F("bell triggered!"));
    stopWatch.resetTime();
    output = HIGH;
}


void handleTimeDelay() {
    if (output == HIGH && stopWatch.getTimeMillis() > DOOR_OPEN_TIME) {
        trace(F("alert"), F("timeout reached!"));
        output = LOW;
    }
}


void resetDataBase() {
    EEPROMaddr = 1;
    EEPROM.write(0, 1); 
    dataBasePointer = 2;
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
void printDataBase() {
    Serial.println();
    Serial.print(F("printing DataBase...")); Serial.println();
    Serial.print(F("number of cards in database:"));
    Serial.println(dataBasePointer); Serial.println();
    Serial.print(F("cardNumber")); Serial.print("\t");
    Serial.print(F("ID")); Serial.print("\t");Serial.print("\t");
    Serial.print(F("sKey")); Serial.print("\t"); Serial.println();
    for (byte i = 0; i < dataBasePointer; i++) {
        Serial.print(i); Serial.print("\t");Serial.print("\t");
        dump_byte_array(my_dataBase.ID[i],4); Serial.print("\t");
        dump_byte_array(my_dataBase.sKey[i],6); Serial.print("\t");
        Serial.println();
    }
    Serial.print(F("EEPROM address is "));
    Serial.println(int(EEPROMaddr));
    Serial.println(F("======================================="));
}


/**
* ReadBlock
*/
// felan ba in farz k buffer kharab nemishe vagarna k in nabayad void bashe o output dare
void ReadBlock(byte MYblockAddr) {
    MFRC522::StatusCode status;
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(MYblockAddr, buffer, &size);
}


/**
 * array equality check
 */
boolean ArrCheck (byte Arr1[] ,byte Arr2[] ,byte arrSize) {
    boolean result;
    for (byte i=0 ; i < arrSize ; i++) {
        if (Arr1[i] == Arr2[i])
            result = true;//thats ok lets go for next i
        else
            return false;
    }
    return true;
}


// key generator 
void keyGen(void) {
    // key_A is 6 bytes
    for ( byte i = 0 ; i < 6 ; i++ ) {
        randByte = 0xFF; // making a random byte
        newKey[i] = randByte;
    }
}


// masking trailer with new key_A
void trailerMasker(void) {
    for ( byte i = 0 ;  i < 16 ; i++ ) {
        if ( i < 6 ) {
            newKey[i] = newKey[i] | 0x00;
            dataBlock[i] = 0xFF;
        } else {
            newKey[i] = newKey[i] | 0xFF;
        }
    }
    for ( byte i = 0 ; i < 16 ; i++ ) {
        dataBlock[i] = newKey[i] & dataBlock[i]; // now our block is refreshed with a new key_A
    }
}


/////
/**
* authenticating with key_A
*/
boolean authenticate_keyA (byte cardNumber) {
    // Authenticate using key A
    MFRC522::StatusCode status;
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = my_dataBase.sKey[cardNumber][i];
    }
    Serial.println(F("Authenticating using key A...")); Serial.println();
    dump_byte_array(key.keyByte,6); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authenticate_with_keyA failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }
    else {
        Serial.print(F("Authenticate_with_keyA passed: "));
        return true;
    }
}


/**
 * AddCardIdentifier
 */
boolean AddCardIdentifier(void) {
    MFRC522::StatusCode status;
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    // Authenticate using key A
    trailerBlock = 11; //hamishe saabet chon too block 2 data neveshti
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.print(F("sorry, its not an add card "));
        return false;
    } else {
        Serial.print(F("mmm, it can be an add card "));
        ReadBlock (8);
        boolean result;
        result = ArrCheck (buffer, addCard_data, 16);
        if ( result == true ) {
            Serial.print(F("ooo,yeah, thats an add card "));
            return true;
        } else {
            Serial.print(F("oops, its not an add card "));
            return false;
        }
    }
}


/**
 * checks if this is a remove card
 */
boolean RemoveCardIdentifier(void) {
    MFRC522::StatusCode status;
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    // Authenticate using key A
    trailerBlock = 11; //hamishe saabete ahmagh chon too block 2 data neveshti
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.print(F("sorry, its not a remove card "));
        return false;
    }
    else {
        Serial.print(F("mmm, it can be a remove card "));
        ReadBlock (8);
        boolean result;
        result = ArrCheck (buffer, removeCard_data, 16);
        if ( result == true ) {
            Serial.print(F("ooo,yeah, thats a remove card "));
            return true;
        } else {
            Serial.print(F("oops, its not a remove card "));
            return false;
        }
    }
}


/**
* AddToDataBase
*/
void AddToDataBase () {
    MFRC522::StatusCode status;
    boolean ID_result = false;
    for ( byte j = 0; j < dataBasePointer; j++ ) {
        ID_result = ArrCheck (my_dataBase.ID[j], mfrc522.uid.uidByte, 4);
        // it means this card is one of our cards
        if ( ID_result == true ) {
            // Serial.print(F("we already have it in our database... "));
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
    //  Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
    //    Serial.print(F("PCD_Authenticate() failed: "));
    //    Serial.print(F("sorry, this card (this sector) has been used before for some other application, buy a new one... "));
        return;
    }
    //every thing is ok lets add it
    keyGen();
    // put the key into the card
    trailerMasker();
    WriteToblock(3,15); // we write to 6th sector because I like it :D when we had more applications for a card we need a plan
    // now save the card into our dataBase
    EEPROMaddr = int(EEPROM.read(0));
    // adding ID to the data base
    for ( byte i = 0 ; i < 4 ; i++) {  
        my_dataBase.ID [dataBasePointer][i] =  mfrc522.uid.uidByte[i];
        EEPROM.write(EEPROMaddr, mfrc522.uid.uidByte[i]);
        EEPROMaddr++;
    }
    // adding sKey to the data base
    for ( byte i = 0 ; i < 6 ; i++) { 
        my_dataBase.sKey [dataBasePointer][i] = newKey[i];
        EEPROM.write(EEPROMaddr, newKey[i]);
        EEPROMaddr++;
    }
    EEPROM.write(0, byte(EEPROMaddr));
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
void RemoveFromDataBase () {
    boolean ID_result = false;
    boolean authenticate_result;
    for ( byte j = 2; j < dataBasePointer; j++ ) {
        ID_result = ArrCheck (my_dataBase.ID[j], mfrc522.uid.uidByte, 4);
        // it means this card is one of our cards
        if (ID_result == true) {
            Serial.print(F("removing card with j = ... ")); Serial.print(j); Serial.println();
            authenticate_result = authenticate_keyA(j);
            if (authenticate_result == true) {
                for (byte i = 0; i < 6; i++) {
                    newKey[i] = 0xFF;
                }
                trailerMasker();
                WriteToblock(3,15); // we change 7th sector as it was at chip delivery
                Serial.print(F("card key changed successfully (card removed)")); Serial.println();
                //now remove the card from database
                byte cardNum = j;
                for( cardNum ; cardNum < dataBasePointer ;  cardNum++ ) {
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
            Serial.print(F("ID is in the database but the key is not correct "));
            return;
        }
    }
    // it means this card is not in our database and ID_result has stayed false
    Serial.print(F("this card is not in our database ")); Serial.println();
    printDataBase();
}


/**  
 *   write to block
 */
void WriteToblock (byte MYsector, byte MYblockAddr) {
    MFRC522::StatusCode status;
    byte buffer[16];
    byte size = sizeof(buffer); 
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(MYblockAddr, dataBlock, 16);
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(MYblockAddr, buffer, &size);
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
}


/**
* checks if the user wants to add a card to database
*/
boolean AddCheck() {
    //lets add a card
    MFRC522::StatusCode status;
    boolean add_temp;
    add_temp = AddCardIdentifier();
    if ( add_temp == true ) { // add the ADD CARD to databse
        // check database0 maybe the add card had been added before
        //in ghalate!!!! hamishe true e
        if ( dataBasePointer == 0 ) {
            Serial.println(F("You have added your add card to database before...")); 
            Serial.println();
        } else {
            // adding the ID of add card to first place of data base
            for ( byte i = 0 ; i < 4 ; i++ ) {
                my_dataBase.ID[0][i] = mfrc522.uid.uidByte[i];
            }
            // adding the sKey of add card to first place of data base
            for ( byte i = 0 ; i < 6 ; i++ ) {
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
         // we wait for another card to come
        while (1) {
            if (state == 0) {
                time1 = millis();
                state = 1; 
            } else if (state == 1) {
                time2 = millis();
                Serial.println(F("waiting for the second card to come")); Serial.println();
                state = 2;
            } else if (state == 2) {
                if ( mfrc522.PICC_IsNewCardPresent() ) {
                    Serial.println(F("another card is coming")); Serial.println();
                    if ( !mfrc522.PICC_ReadCardSerial() ) {
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
        mfrc522.PICC_HaltA();
        // Stop encryption on PCD
        mfrc522.PCD_StopCrypto1();
        return add_temp;
    }
}


/**
* checks if the User wants to remove a card from data base
*/
boolean RemoveCheck() {
    //lets remove a card
    MFRC522::StatusCode status;
    boolean remove_temp;
    remove_temp = RemoveCardIdentifier();
    if ( remove_temp == true ) { // add the REMOVE CARD from databse
        // check database0 maybe the add card had been added before
        //in ghalate!!!! hamishe true e
        if (dataBasePointer == 0) {
            Serial.println(F("You have added your add card to database before...")); 
            Serial.println();
        } else {
            // adding the ID of remove card to second place of data base
            for (byte i = 0; i < 4; i++) {
                my_dataBase.ID[1][i] = mfrc522.uid.uidByte[i];
            }
            // adding the sKey of remove card to second place of data base
            for (byte i = 0; i < 6; i++) {
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
        // we wait for another card to come
        while (1) {
            if (state == 0) {
                time1 = millis();
                state = 1; 
            }  else if (state == 1) {
                time2 = millis();
                Serial.println(F("waiting for the second card to come")); Serial.println();
                state = 2;
            } else if (state == 2) {
                if ( mfrc522.PICC_IsNewCardPresent() ) {
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
        // Halt PICC
        mfrc522.PICC_HaltA();
        // Stop encryption on PCD
        mfrc522.PCD_StopCrypto1();
        return remove_temp;
    }
}


/**
* nightsWatch :D cheking the card ID and if we had it check the key and authenticate and output T/F
*/
boolean nightsWatch () {
    boolean ID_result = false;
    boolean authenticate_result;
    trailerBlock = 15;
    for ( byte j = 2; j < dataBasePointer; j++ ) {
        ID_result = ArrCheck (my_dataBase.ID[j], mfrc522.uid.uidByte, 4);
        // it means this card is one of our cards
        if ( ID_result == true ) {
            authenticate_result = authenticate_keyA(j);
            cardLocation = j;
            // Halt PICC
            mfrc522.PICC_HaltA();
            // Stop encryption on PCD
            mfrc522.PCD_StopCrypto1();
            return authenticate_result;
        }
    }
    // it means this card is not in our database and ID_result has stayed false
    return false;
}


/**
* you have a correct card you can enter
*/
void Welcome() {
    Serial.println(F("you have a correct card you can enter..."));
    analogWrite(buzzerPin, 600);
    delay(500);
    analogWrite(buzzerPin, 0);
    bellTrigger();
}


/**
* you Dont have a correct card , go home Dear thief
*/
void NotWelcome() {
    Serial.println(F("you Dont have a correct card , go home Dear thief..."));
    analogWrite(buzzerPin, 600);
    delay(500);
    analogWrite(buzzerPin, 0);
    delay(200);
    analogWrite(buzzerPin, 600);
    delay(500);
    analogWrite(buzzerPin, 0);
    delay(200);
    analogWrite(buzzerPin, 600);
    delay(500);
    analogWrite(buzzerPin, 0);
}


/**
* When a card is been used, this function changes its key
* This function needs the location of card in the database as an input
*/
void keyChanger (byte cardLoc) {
    //every thing is ok lets add it
    keyGen();
    // put the key into the card
    trailerMasker();
    WriteToblock(3, 15); // we write to third sector because I like it :D when we had more applications for a card we need a plan
    // now change the key into our dataBase
    for ( byte i = 0 ; i < 6 ; i++) // changing sKey in the data base
        my_dataBase.sKey [cardLoc][i] = newKey[i];
}


void doorDecision() {
    // moghaabek ro baraabar e output gharaar bede
    int value = getValue(digitalRead(interruptPinUp) == LOW, digitalRead(F_PIN) == LOW, digitalRead(interruptPinDown) == LOW);
    // yek moteghayer baraaye inke force koni dar baaz she
    boolean open_door = output == HIGH;
    if (value == B00000000) {
        openDoor();
    } else if (value == B00000001) {
        stayRelax();
    } else if (value == B00000010) {
        if (open_door) {
            openDoor();
        } else {
            closeDoor();
        }
    } else if (value == B00000011) {
        if (open_door) {
            stayRelax();
        } else {
            // ba'de 1 saaniye baste she
            delay(DELAY_BEFORE_CLOSING);
            closeDoor();
        }
    } else if (value == B00000100) {
        openDoor();
    } else if (value == B00000101) {
        trace(F("error"), F("UP and DOWN signals occured simultaneously!"));
    } else if (value == B00000110) {
        if (open_door) {
            openDoor();
        } else {
            stayRelax();
        }
    } else if (value == B00000111) {
        trace(F("error"), F("UP and DOWN signals occured simultaneously!"));
    }
    handleTimeDelay();
    
    if (digitalRead(BELL_RINGER) == LOW) {
        bellTrigger();
    }
}

void intMakeDec() {
    // moghaabek ro baraabar e output gharaar bede
    int value = getValue(digitalRead(interruptPinUp) == LOW, digitalRead(F_PIN) == LOW, digitalRead(interruptPinDown) == LOW);
    // yek moteghayer baraaye inke force koni dar baaz she
    boolean open_door = output == HIGH;
    if (value == B00000000) {
        openDoor();
    } else if (value == B00000001) {
        stayRelax();
    } else if (value == B00000010) {
        if (open_door) {
            openDoor();
        } else {
            closeDoor();
        }
    } else if (value == B00000011) {
        if (open_door) {
            stayRelax();
        } else {
            // ba'de 1 saaniye baste she
            delay(DELAY_BEFORE_CLOSING);
            closeDoor();
        }
    } else if (value == B00000100) {
        openDoor();
    } else if (value == B00000101) {
        trace(F("error"), F("UP and DOWN signals occured simultaneously!"));
    } else if (value == B00000110) {
        if (open_door) {
            openDoor();
        } else {
            stayRelax();
        }
    } else if (value == B00000111) {
        trace(F("error"), F("UP and DOWN signals occured simultaneously!"));
    }
    handleTimeDelay();
    
    if (digitalRead(BELL_RINGER) == LOW) {
        bellTrigger();
    }
}
/**
 * Initialize.
 */
void setup() {
    /*
    * RFID setUp
    */
    Serial.begin(115200); // Initialize serial communications with the PC

    trace(F("alert"), F("here we are! now!!"));
//    pinMode(UP, INPUT);
//    pinMode(DOWN, INPUT);
    pinMode(F_PIN, INPUT);
    pinMode(BELL_RINGER, INPUT);
    pinMode(RESET_PIN, INPUT);
    pinMode(MOGHAABEL, OUTPUT);
    pinMode(MOTOR_ONE, OUTPUT);
    pinMode(MOTOR_TWO, OUTPUT);
    pinMode(interruptPinUp, INPUT_PULLUP);
    pinMode(interruptPinDown, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(interruptPinUp), intMakeDec, CHANGE);
    attachInterrupt(digitalPinToInterrupt(interruptPinDown), intMakeDec, CHANGE);
    // pinMode(pulsePin, OUTPUT);
    // pinMode(openPin,OUTPUT);

    digitalWrite(BELL_RINGER, HIGH);
    digitalWrite(interruptPinUp, HIGH);
    digitalWrite(interruptPinDown, HIGH);
    digitalWrite(F_PIN, HIGH);
    digitalWrite(RESET_PIN, HIGH);
    // digitalWrite(openPin,LOW);
    // analogWrite(pulsePin, 0);
    stayRelax();

    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    randomSeed(analogRead(6));
    EEPROMaddr = EEPROM.read(0);
    if ((EEPROMaddr - 1) % 10 != 0) {
        resetDataBase();
    }
    for (int i = 1; i < EEPROMaddr; i += 10) {
        dataBasePointer++;
        for (int j = 0; j < 4; ++j) {
            my_dataBase.ID[(i - 1) / 10 + 2][j] = EEPROM.read(i + j);
        }
        for (int j = 0; j < 6; ++j) {
            my_dataBase.sKey[(i - 1) / 10 + 2][j] = EEPROM.read(i + 4 + j);
        }
    }
    printDataBase();
    counter = 0;
}


/**
 * Main loop.
 */
void loop() {
    counter ++;
    if (counter % 20 == 0)
        doorDecision();
    /* 
    * battery handeling 
    */
    if (digitalRead(RESET_PIN) == LOW)
        resetDataBase();

    if (output == HIGH) {
        return;
    }
    nodeMcuCommand();
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
    printDataBase();
    Serial.println(F("nightsWatch is ready for next card")); Serial.println();
    if (nightsWatch()) {
        keyChanger(cardLocation);
        Welcome();
        //digitalWrite(pulsePin, 0);
        return;
    }
    if (AddCheck())
        return;
    NotWelcome();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
void nodeMcuCommand(){
  int temp = analogRead(A4);
  if (temp > 400){
    Serial.println(F("nodeMcu says open"));
    bellTrigger();
  }
}


