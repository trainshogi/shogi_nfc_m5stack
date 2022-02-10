#include <SPI.h>
#include <MFRC522.h>
//#include <M5Stack.h>

constexpr uint8_t RST_PIN = 33;//9;          // Configurable, see typical pin layout above

#define SS_1_PIN        21 //10         // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 2
#define SS_2_PIN        17 //6          // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 1

#define NR_OF_READERS   2

byte ssPins[] = {SS_1_PIN, SS_2_PIN};

MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.

SPIclass SPI2;

void setup() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  digitalWrite(SS_1_PIN,HIGH);
  digitalWrite(SS_2_PIN,HIGH);
  SPI2.begin();      // Init SPI bus

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    digitalWrite(ssPins[reader], LOW);
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();    
    digitalWrite(ssPins[reader], HIGH);
  }

  // m5stack初期化
//  M5.begin();
  // QR Code
//  M5.Lcd.qrcode("content",50,10,220,6);
}

void loop() {
  String nfcdata;

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    // Look for new cards
    digitalWrite(ssPins[reader], LOW);

     if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
      Serial.print(F("Reader "));
      Serial.print(reader);
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card UID:"));
      dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      Serial.println();
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = mfrc522[reader].PICC_GetType(mfrc522[reader].uid.sak);
      Serial.println(mfrc522[reader].PICC_GetTypeName(piccType));
      // PICC_DumpMifareUltralightToSerial(reader);
      nfcdata = PICC_DumpMifareUltralightToSerialArray(reader);
      Serial.println(nfcdata);

      

      // Halt PICC
      mfrc522[reader].PICC_HaltA();
      // Stop encryption on PCD
      mfrc522[reader].PCD_StopCrypto1();
     } //if (mfrc522[reader].PICC_IsNewC
    
    digitalWrite(ssPins[reader], HIGH);

  } //for(uint8_t reader

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
 * Dumps memory contents of a MIFARE Ultralight PICC.
 */
void PICC_DumpMifareUltralightToSerial(uint8_t reader) {
  MFRC522::StatusCode status;
  byte byteCount;
  byte buffer[18];
  byte i;
  
  Serial.println(F("Page  0  1  2  3"));
  // Try the mpages of the original Ultralight. Ultralight C has more pages.
  for (byte page = 4; page < 16; page +=4) { // Read returns data for 4 pages at a time.
    // Read pages
    byteCount = sizeof(buffer);
    status = mfrc522[reader].MIFARE_Read(page, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(MFRC522::GetStatusCodeName(status));
      break;
    }
    // Dump data
    for (byte offset = 0; offset < 4; offset++) {
      i = page + offset;
      if(i < 10)
        Serial.print(F("  ")); // Pad with spaces
      else
        Serial.print(F(" ")); // Pad with spaces
      Serial.print(i);
      Serial.print(F("  "));
      for (byte index = 0; index < 4; index++) {
        i = 4 * offset + index;
        if(buffer[i] < 0x10)
          Serial.print(F(" 0"));
        else
          Serial.print(F(" "));
        Serial.print(buffer[i], HEX);
      }
      Serial.println();
    }
  }
} // End PICC_DumpMifareUltralightToSerial()

/**
 * Convert memory contents of a MIFARE Ultralight PICC to String.
 */
String PICC_DumpMifareUltralightToSerialArray(uint8_t reader) {
  MFRC522::StatusCode status;
  byte byteCount;
  byte buffer[18];
  byte page = 4;
  
  // Read returns data for 4 pages at a time.
  byteCount = sizeof(buffer);
  status = mfrc522[reader].MIFARE_Read(page, buffer, &byteCount);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(MFRC522::GetStatusCodeName(status));
    return "";
  }
  if ((buffer[0] != 0x03) || (buffer[2] != 0xD1) || (buffer[3] != 0x01) || (buffer[5] != 0x54)) {
    Serial.println(F("NFC format is wrong."));
    return "";
  }
  
  byte byteData[buffer[4]+1];
  cutArray(buffer, 6, buffer[4], byteData);
  return String((char*) byteData);
    
} // End PICC_DumpMifareUltralightToSerialArray()

void cutArray(byte array1[], uint8_t offset, uint8_t dataLength, byte result[]){
  for(int i=0; i<dataLength; i++){
      result[i] = array1[offset+i];
  }
  result[dataLength] = '\0';
}
