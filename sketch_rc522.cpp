#include <SPI.h>
#include <M5Stack.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <MFRC522Constants.h>

#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#define SS_1_PIN        21 //10         // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 2
#define SS_2_PIN        22 //6          // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 1

#define CK 5
#define MI 17
#define MO 16

constexpr uint8_t RST_PIN = 33;//9;          // Configurable, see typical pin layout above

SPIClass hspi(HSPI);

const SPISettings spiSettings = SPISettings(SPI_CLOCK_DIV4, MSBFIRST, SPI_MODE0); // May have to be set if hardware is not fully compatible to Arduino specifications.

byte ssPins[] = {SS_1_PIN, SS_2_PIN};

MFRC522DriverPinSimple ss_1_pin(SS_1_PIN); // Configurable, take an unused pin, only HIGH/LOW required, must be different to SS 2.
MFRC522DriverPinSimple ss_2_pin(SS_2_PIN); // Configurable, take an unused pin, only HIGH/LOW required, must be different to SS 1.

MFRC522DriverSPI driver_1{ss_1_pin, hspi, spiSettings};
MFRC522DriverSPI driver_2{ss_2_pin, hspi, spiSettings};

MFRC522 readers[]{driver_1, driver_2};   // Create MFRC522 instance.

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

String kifdata = "https://shogi.nkkuma.tokyo/animal?kif=";

void setup() {

  // m5stack初期化
  M5.begin();
  // QR Code
  M5.Lcd.qrcode("content",50,10,220,6);
  
//  Serial.begin(115200);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  digitalWrite(SS_1_PIN,HIGH);
  digitalWrite(SS_2_PIN,HIGH);

  hspi.begin(CK,MI,MO);      // Init SPI bus
  static uint8_t i = 0;

  for (MFRC522 reader : readers) {
    digitalWrite(ssPins[i], LOW);
    reader.PCD_Init(); // Init each MFRC522 card.
    Serial.print(F("Reader "));
    Serial.print(i);
    Serial.print(F(": "));
    MFRC522Debug::PCD_DumpVersionToSerial(reader, Serial);    
    digitalWrite(ssPins[i], HIGH);
    i++;
  }
}

void loop() {
  String nfcdata;
  uint8_t i = 0;

  for (MFRC522 reader : readers) {
    // Look for new cards
    digitalWrite(ssPins[i], LOW);

     if (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial()) {
      Serial.print(F("Reader "));
      Serial.print(i);
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card UID:"));
      dump_byte_array(reader.uid.uidByte, reader.uid.size);
      Serial.println();
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = reader.PICC_GetType(reader.uid.sak);
      Serial.println(MFRC522Debug::PICC_GetTypeName(piccType));
      nfcdata = PICC_DumpMifareUltralightToSerialArray(reader);
      Serial.println(nfcdata);
      if (nfcdata != "") {
        // place
        if (i == 0) { 
          playMP3("/sounds/11.mp3"); 
          kifdata += "11";
        }
        else if (i == 1) { 
          playMP3("/sounds/12.mp3"); 
          kifdata += "12";
        }

        //piece
          
        if (nfcdata == "enlion1") {
          playMP3("/sounds/104.mp3");
          kifdata += "lion,";
        }
        else if (nfcdata == "enlion2") {
          playMP3("/sounds/104.mp3");
          kifdata += "lion,";
        }
        else if (nfcdata == "enkirin1") {
          playMP3("/sounds/103.mp3");
          kifdata += "kirin,";
        }
        else if (nfcdata == "enkirin2") {
          playMP3("/sounds/103.mp3");
          kifdata += "kirin,";
        }
        else if (nfcdata == "enzou1") {
          playMP3("/sounds/102.mp3");
          kifdata += "zou,";
        }
        else if (nfcdata == "enzou2") {
          playMP3("/sounds/102.mp3");
          kifdata += "zou,";
        }

        M5.Lcd.qrcode(kifdata,50,10,220,6);
      }

      // Halt PICC
      reader.PICC_HaltA();
      // Stop encryption on PCD
      reader.PCD_StopCrypto1();
     } //if (mfrc522[reader].PICC_IsNewC
    
    digitalWrite(ssPins[i], HIGH);
    i++;

  } //for(uint8_t reader

  M5.update();
  if(M5.BtnA.isPressed()){
    playMP3("/sounds/11.mp3");
    playMP3("/sounds/101.mp3");
  }

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
 * Convert memory contents of a MIFARE Ultralight PICC to String.
 */
String PICC_DumpMifareUltralightToSerialArray(MFRC522 reader) {
  MFRC522::StatusCode status;
  byte byteCount;
  byte buffer[18];
  byte page = 4;
  
  // Read returns data for 4 pages at a time.
  byteCount = sizeof(buffer);
  status = reader.MIFARE_Read(page, buffer, &byteCount);
  if (status != MFRC522Constants::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(MFRC522Debug::GetStatusCodeName(status));
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

void playMP3(char *filename){
  file = new AudioFileSourceSD(filename);
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  out->SetGain(1.0);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
  while(mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  }
}
