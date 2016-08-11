#ifndef _BASE64_H_
#define _BASE64_H_

#include "claa_base.h"

uint8 IsBase64Terminator(char const* ptr);
uint8 ConvertBase64CharacterToBinary(char in);
char ConvertBinaryToBase64Character(uint8 in);
uint16 ConvertBinaryArrayToBase64Text(uint8 const in[], uint16 inputBytes, char out[], uint8 pad);
sint16 ConvertBase64TextToBinaryArray(char const in[], uint8 out[], uint16 outputBytes);

#endif

