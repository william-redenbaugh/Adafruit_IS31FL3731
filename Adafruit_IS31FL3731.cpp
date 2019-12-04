#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <Adafruit_IS31FL3731.h>

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif


/**************************************************************************/
/*!
    @brief Constructor for breakout version
    @param width Desired width of led display
    @param height Desired height of led display
*/
/**************************************************************************/

Adafruit_IS31FL3731::Adafruit_IS31FL3731(uint8_t width, uint8_t height) : Adafruit_GFX(width, height) {
}

/**************************************************************************/
/*!
    @brief Constructor for FeatherWing version (15x7 LEDs)
*/
/**************************************************************************/
Adafruit_IS31FL3731_Wing::Adafruit_IS31FL3731_Wing(void) : Adafruit_IS31FL3731(15, 7) {
}

/**************************************************************************/
/*!
    @brief Initialize hardware and clear display
    @param addr The I2C address we expect to find the chip at
    @returns True on success, false if chip isnt found
*/
/**************************************************************************/
bool Adafruit_IS31FL3731::begin(uint8_t addr) {
  Wire.begin();
  Wire.setClock(400000);
  
  _i2caddr = addr;
  _frame = 0;

  // A basic scanner, see if it ACK's
  Wire.beginTransmission(_i2caddr);
  if (Wire.endTransmission () != 0) {
    return false;
  }

  // shutdown
  writeRegister8(ISSI_BANK_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0x00);

  delay(10);

  // out of shutdown
  writeRegister8(ISSI_BANK_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0x01);

  // picture mode
  writeRegister8(ISSI_BANK_FUNCTIONREG, ISSI_REG_CONFIG, ISSI_REG_CONFIG_PICTUREMODE);

  displayFrame(_frame);

  // all LEDs on & 0 PWM
  clear(); // set each led to 0 PWM

  for (uint8_t f=0; f<8; f++) {  
    for (uint8_t i=0; i<=0x11; i++)
      writeRegister8(f, i, 0xff);     // each 8 LEDs on
  }

  audioSync(false);

  return true;
}

/**************************************************************************/
/*!
    @brief Sets all LEDs on & 0 PWM for current frame.
*/
/**************************************************************************/
void Adafruit_IS31FL3731::clear(void) {
  selectBank(_frame);

  for (uint8_t i=0; i<6; i++) {
    Wire.beginTransmission(_i2caddr);
    Wire.write((byte) 0x24+i*24);
    // write 24 bytes at once
    for (uint8_t j=0; j<24; j++) {
      Wire.write((byte) 0);
    }
    Wire.endTransmission();
  }
}

/**************************************************************************/
/*!
    @brief Low level accesssor - sets a 8-bit PWM pixel value to a bank location
    does not handle rotation, x/y or any rearrangements!
    @param lednum The offset into the bank that corresponds to the LED
    @param bank The bank/frame we will set the data in
    @param pwm brightnes, from 0 (off) to 255 (max on)
*/
/**************************************************************************/
void Adafruit_IS31FL3731::setLEDPWM(uint8_t lednum, uint8_t pwm, uint8_t bank) {
  if (lednum >= 144) return;
  writeRegister8(bank, 0x24+lednum, pwm);
}

/**************************************************************************/
/*!
    @brief Adafruit GFX low level accesssor - sets a 8-bit PWM pixel value
    handles rotation and pixel arrangement, unlike setLEDPWM
    @param x The x position, starting with 0 for left-most side
    @param y The y position, starting with 0 for top-most side
    @param color Despite being a 16-bit value, takes 0 (off) to 255 (max on)
*/
/**************************************************************************/
void Adafruit_IS31FL3731_Wing::drawPixel(int16_t x, int16_t y, uint16_t color) {
 // check rotation, move pixel around if necessary
  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = 15 - x - 1;
    break;
  case 2:
    x = 15 - x - 1;
    y = 7 - y - 1;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = 9 - y - 1;
    break;
  }

  // charlie wing is smaller:
  if ((x < 0) || (x >= 16) || (y < 0) || (y >= 7)) return;

  if (x > 7) {
    x=15-x;
    y += 8;
  } else {
    y = 7-y;
  }

  _swap_int16_t(x, y);
 
  if (color > 255) color = 255; // PWM 8bit max

  setLEDPWM(x + y*16, color, _frame);
  return;
}



/**************************************************************************/
/*!
    @brief Adafruit GFX low level accesssor - sets a 8-bit PWM pixel value
    handles rotation and pixel arrangement, unlike setLEDPWM
    @param x The x position, starting with 0 for left-most side
    @param y The y position, starting with 0 for top-most side
    @param color Despite being a 16-bit value, takes 0 (off) to 255 (max on)
*/
/**************************************************************************/
void Adafruit_IS31FL3731::drawPixel(int16_t x, int16_t y, uint16_t color) {
 // check rotation, move pixel around if necessary
  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = 16 - x - 1;
    break;
  case 2:
    x = 16 - x - 1;
    y = 9 - y - 1;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = 9 - y - 1;
    break;
  }

  if ((x < 0) || (x >= 16)) return;
  if ((y < 0) || (y >= 9)) return;
  if (color > 255) color = 255; // PWM 8bit max

  setLEDPWM(x + y*16, color, _frame);
  return;
}

/**************************************************************************/
/*!
    @brief Set's this object's frame tracker (does not talk to the chip)
    @param frame Ranges from 0 - 7 for the 8 frames
*/
/**************************************************************************/
void Adafruit_IS31FL3731::setFrame(uint8_t frame) {
  _frame = frame;
}

/**************************************************************************/
/*!
    @brief Have the chip set the display to the contents of a frame
    @param frame Ranges from 0 - 7 for the 8 frames
*/
/**************************************************************************/
void Adafruit_IS31FL3731::displayFrame(uint8_t frame) {
  if (frame > 7) frame = 0;
  writeRegister8(ISSI_BANK_FUNCTIONREG, ISSI_REG_PICTUREFRAME, frame);
}


/**************************************************************************/
/*!
    @brief Switch to a given bank in the chip memory for future reads
    @param bank The IS31 bank to switch to
*/
/**************************************************************************/
void Adafruit_IS31FL3731::selectBank(uint8_t bank) {
 Wire.beginTransmission(_i2caddr);
 Wire.write((byte)ISSI_COMMANDREGISTER);
 Wire.write(bank);
 Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief Enable the audio 'sync' for brightness pulsing (not really tested)
    @param sync True to enable, False to disable
*/
/**************************************************************************/
void Adafruit_IS31FL3731::audioSync(boolean sync) {
  if (sync) {
    writeRegister8(ISSI_BANK_FUNCTIONREG, ISSI_REG_AUDIOSYNC, 0x1);
  } else {
    writeRegister8(ISSI_BANK_FUNCTIONREG, ISSI_REG_AUDIOSYNC, 0x0);
  }
}

/**************************************************************************/
/*!
    @brief Write one byte to a register located in a given bank
    @param bank The IS31 bank to write the register location
    @param reg the offset into the bank to write
    @param data The byte value
*/
/**************************************************************************/
void Adafruit_IS31FL3731::writeRegister8(uint8_t bank, uint8_t reg, uint8_t data) {
  selectBank(bank);

  Wire.beginTransmission(_i2caddr);
  Wire.write((byte)reg);
  Wire.write((byte)data);
  Wire.endTransmission();
  //Serial.print("$"); Serial.print(reg, HEX);
  //Serial.print(" = 0x"); Serial.println(data, HEX);
}


/**************************************************************************/
/*!
    @brief  Read one byte from a register located in a given bank
    @param   bank The IS31 bank to read the register location
    @param   reg the offset into the bank to read
    @return 1 byte value
*/
/**************************************************************************/
uint8_t  Adafruit_IS31FL3731::readRegister8(uint8_t bank, uint8_t reg) {
 uint8_t x;

 selectBank(bank);

 Wire.beginTransmission(_i2caddr);
 Wire.write((byte)reg);
 Wire.endTransmission();

 Wire.requestFrom(_i2caddr, (byte)1);
 x = Wire.read();

// Serial.print("$"); Serial.print(reg, HEX);
// Serial.print(": 0x"); Serial.println(x, HEX);

  return x;
}
