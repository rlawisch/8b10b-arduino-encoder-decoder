#include <Wire.h>

#define I2C_DEV_ADDR 0x01

// Encoding lookup tables
unsigned char lookup3b4b[9] = {11, 9, 5, 12, 13, 10, 6, 14, 7};
unsigned char lookup5b6b[32] = {39, 29, 45, 49, 53, 41, 25, 56, 57, 37, 21, 52, 13, 44, 28, 23, 27, 35, 19, 50, 11, 42, 26, 58, 51, 38, 22, 54, 14, 46, 30, 43};

char runningDisparity = -1; // As per specification, runningDisparity always starts at -1
bool shouldHandleDisparityInversion;

bool hasDisparity(unsigned short int data, unsigned char bits) {
  unsigned char ones = 0, zeros = 0;

  // For each bit of data, shifting i to the left at each iteration
  for (unsigned int i = 1; bits > 0; i <<= 1, bits--) {
    // If current bit is a one
    if (data & i)
      ones++; // Increment ones counter
    else
      zeros++; // Increment zeros counter
  }

  // Return true if the counts don't match
  return (ones != zeros);
}

unsigned char to4B(unsigned char data, bool shouldHandleFiveConsecEqualBits) {
  unsigned char encoded;
  bool currentDisparity;

  // Use lookup table to encode 3 bit part, handling special cases
  if (data == 7 && shouldHandleFiveConsecEqualBits && runningDisparity < 0)
    encoded = lookup3b4b[8];
  else if (data == 7 && shouldHandleFiveConsecEqualBits && runningDisparity > 0)
    encoded = ~lookup3b4b[8] & 0xF;
  else {
    encoded = lookup3b4b[data];
    currentDisparity = hasDisparity(encoded, 4);

    if (shouldHandleDisparityInversion && (encoded == 12 || currentDisparity))
      encoded = ~encoded & 0xF; // 4 bits, 0xF = 0000 1111
  }

  return encoded;
}

unsigned char to6B(unsigned char data) {
  unsigned char encoded;
  bool currentDisparity;

  // Use lookup table to encode 5 bit part
  encoded = lookup5b6b[data];

  // Check disparity of encoded part
  currentDisparity = hasDisparity(encoded, 6);

  shouldHandleDisparityInversion = currentDisparity ^ (runningDisparity < 0);

  if (runningDisparity > 0 && (encoded == 56 || currentDisparity))
    encoded = ~encoded & 0x3F; // 6 bits, 0x3F = 0011 1111

  return encoded;
}

unsigned short int to10B(unsigned char data) {
  unsigned char EDCBA, HGF, abcdei, fghj;
  unsigned short int encodedChar;
  bool shouldHandleFiveConsecEqualBits;

  // Divide the 8 bits into parts of 5 and 3 bits
  EDCBA = data & 0x1F; // EDCBA, 0x1F = 0001 1111
  HGF = (data & 0xE0) >> 5; // HGF, 0xE0 = 1110 0000

  // Check if special cases should be handled
  // Five consecutive equal bits should be avoided, so there are alternate encoded versions to pick
  shouldHandleFiveConsecEqualBits = ((EDCBA == 17 || EDCBA == 18 || EDCBA == 20) && runningDisparity < 0) || ((EDCBA == 11 || EDCBA == 13 || EDCBA == 14) && runningDisparity > 0);

  // Encode divided parts
  abcdei = to6B(EDCBA);
  fghj = to4B(HGF, shouldHandleFiveConsecEqualBits);

  // Mount entire 10-bit data with encoded parts
  encodedChar = (unsigned short int) ((abcdei << 4) | fghj);

  // Keep running disparity information
  if (hasDisparity(encodedChar, 10))
    runningDisparity = -runningDisparity;

  return encodedChar;
}

void encodeAndTransmit(String data) {
  // Get data length
  int dataLength = data.length();

  // Print data info
  Serial.print("Sending \"");
  Serial.print(data);
  Serial.print("\" (");
  Serial.print(dataLength);
  Serial.println(" bytes):");

  // Print column headers
  Serial.println("CHR | DEC | 8b10b HEX");

  unsigned char message[dataLength + 1];
  strcpy((char*) message, data.c_str());

  char strBuffer[25];
  short int encodedChar;

  // Start I2C transmission
  Wire.beginTransmission(I2C_DEV_ADDR);

  // For each char of message
  for (int i = 0; i < dataLength; i++) {
    // Encode current char to 10b
    encodedChar = to10B(message[i]);

    // Print formatted output
    sprintf(strBuffer, "  %c | %3d | 0x%03X", message[i], message[i], encodedChar);
    Serial.println(strBuffer);

    // Transmit over I2C
    Wire.write(highByte(encodedChar));
    Wire.write(lowByte(encodedChar));
  }

  // End I2C transmission
  Wire.endTransmission();

  // Print confirmation message
  Serial.print(dataLength * 2);
  Serial.println(" bytes sent.");
}

void setup() {
  Serial.begin(115200); // Starts serial peripheral
  Wire.begin();         // Starts I2C peripheral
}

void loop() {
  // If serial buffer has bytes waiting to be read
  if (Serial.available()) {
    // Encode and transmit it
    encodeAndTransmit(Serial.readString());
  }
}
