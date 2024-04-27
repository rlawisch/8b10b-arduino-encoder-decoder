#include <Wire.h>

#define I2C_DEV_ADDR 0x01

// Decoding lookup tables
unsigned char lookup4b3b[15] = {0, 7, 4, 3, 0, 2, 6, 7, 7, 1, 5, 0, 3, 4, 7};
unsigned char lookup6b5b[59] = {0, 0, 0, 0, 0, 23, 8, 7, 0, 27, 4, 20, 24, 12, 28, 0, 0, 29, 2, 18, 31, 10, 26, 15, 0, 6, 22, 16, 14, 1, 30, 0, 0, 30, 1, 17, 16, 9, 25, 0, 15, 5, 21, 31, 13, 2, 29, 0, 0, 3, 19, 24, 11, 4, 27, 0, 7, 8, 23};

unsigned char to8B(unsigned int data) {
  unsigned char abcdei, fghj, EDCBA, HGF, decodedChar;

  // Divide received 10 bits into parts of 6 and 4 bits
  abcdei = (data & 0x3F0) >> 4; // abcdei0000, 0x3F0 = 11 1111 0000
  fghj = data & 0x00F; // 000000fghj, 0x00F = 00 0000 1111

  // Use lookup tables to decode received parts
  EDCBA = lookup6b5b[abcdei];
  HGF = lookup4b3b[fghj];

  // Return entire byte mounted with decoding parts
  return (HGF << 5) | EDCBA;
}

void handleI2cReceive(int receivedByteCount) {
  int dataLength = receivedByteCount / 2; // Chars are codified into 16-bit variables, so the actual size is half of the byte count

  // Print data info
  Serial.print("Receiving ");
  Serial.print(dataLength);
  Serial.println(" bytes:");

  // Print column headers
  Serial.println("8b10b HEX | DEC | CHR");

  unsigned short int encodedChar;
  unsigned char decodedChar;
  char strBuffer[25];

  // For each char of message
  for (int i = 0; i < dataLength; i++) {
    // Receive over I2C encoded data as 16-bit variable
    encodedChar = (Wire.read() << 8) | Wire.read();

    // Decode current char from 8b10b
    decodedChar = to8B(encodedChar);

    // Print formatted output
    sprintf(strBuffer, "    0x%03X | %3d | %c", encodedChar, decodedChar, decodedChar);
    Serial.println(strBuffer);
  }

  // Print confirmation message
  Serial.print(dataLength);
  Serial.println(" chars received.");
}

void setup()
{
  Serial.begin(115200);
  Wire.onReceive(handleI2cReceive);
  Wire.begin(I2C_DEV_ADDR);
}

void loop() {}
