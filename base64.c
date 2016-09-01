#include <stdlib.h>
#include <string.h>
#include "base64.h"

uint8 IsBase64Terminator(char const* ptr)
{
	switch (*ptr)
	{
	case '=':
	case '\0':
	case '\"':
		return 1;

	default:
		return 0;
	}
}

uint8 ConvertBase64CharacterToBinary(char in)
{
	if (in >= 'A' && in <= 'Z')
		return (uint8)(in - 'A');
	else if (in >= 'a' && in <= 'z')
		return (uint8)(in - 'a' + 26);
	else if (in >= '0' && in <= '9')
		return (uint8)(in - '0' + 52);
	else if (in == '+')
		return 62;
	else if (in == '/')
		return 63;
	else
		return ~0;
}

char ConvertBinaryToBase64Character(uint8 in)
{
	in &= 0x3F;

	if (in <= 25)
		return (schar)(in + 'A');
	else if (in <= 51)
		return (schar)(in - 26 + 'a');
	else if (in <= 61)
		return (schar)(in - 52 + '0');
	else if (in == 62)
		return '+';
	else 
		return '/';
}

uint16 ConvertBinaryArrayToBase64Text(uint8 const in[], uint16 inputBytes, char out[], uint8 pad)
{
	uint8 const* iptr = in;
	uint8 const* const end = in + inputBytes;
	char* optr = out;

	//new bits arrive from the RIGHT
	for (; iptr < end; iptr += 3)
	{
		sint32 bytes = end - iptr;

		if (bytes > 3)
			bytes = 3;

		// avoid data from outside the array getting into store
		uint32 store = (uint32)(iptr[0]) << 16;
		
		if (bytes > 1)
			store |= (uint32)(iptr[1]) << 8;
		
		if (bytes > 2)
			store |= iptr[2];

		*(optr++) = ConvertBinaryToBase64Character((uint8)(store >> 18));	//1 input byte requires 2 characters
		*(optr++) = ConvertBinaryToBase64Character((uint8)(store >> 12));

		if ((bytes > 1) || pad)
			*(optr++) = (bytes > 1) ? ConvertBinaryToBase64Character((uint8)(store >> 6)) : '=';

		if ((bytes > 2) || pad)
			*(optr++) = (bytes > 2) ? ConvertBinaryToBase64Character((uint8)(store)) : '=';
	}
	*optr = '\0';
	return (uint16)(optr - out);
}

sint16 ConvertBase64TextToBinaryArray(char const in[], uint8 out[], uint16 outputBytes)
{
	static const unsigned bitsPerCharacter = 6;
	static const unsigned bitsPerByte = 8;
	uint32 store = 0;
	uint8 bits = 0;

	memset(out,0,outputBytes);

	char const* iptr = in;
	uint8* optr = out;

	uint8 const* const end = out + outputBytes;

	for (;; iptr++)
	{
		uint8 readValue = ConvertBase64CharacterToBinary(*iptr);

		if (readValue == (uint8)(~0))
		{
			if (IsBase64Terminator(iptr))
			{
				if (bits == 0)
					break;
				else if (bits == 2 * bitsPerCharacter)
				{
					if (optr >= end) // about to overflow output
						return -1;
					*(optr++) = (uint8)(store >> 4);
				}
				else if (bits == 3 * bitsPerCharacter)
				{
					if (optr >= end - 1) // about to overflow output
						return -1;
					store >>= 2;
					*(optr++) = (uint8)(store >> 8);
					*(optr++) = (uint8)(store);
				}
				else
					return -1;
				break;
			}
			else
				return -1;
		}
		else
		{
			store = (store << bitsPerCharacter) + readValue;
			bits += bitsPerCharacter;
			if (bits >= bitsPerByte * 3)
			{
				if (optr >= end - 2) // about to overflow output
					return 0;
				*(optr++) = (uint8)(store >> 16);
				*(optr++) = (uint8)(store >> 8);
				*(optr++) = (uint8)(store);
				bits = 0;
				store = 0;
			}
		}
	}
	
	return (uint16)(optr - out);
}



