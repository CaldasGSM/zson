#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "json.h"

typedef  signed char        int8;
typedef  unsigned char      uint8;
typedef  signed short       int16;
typedef  unsigned short     uint16;
typedef  signed long        int32;
typedef  unsigned long      uint32;
typedef  signed long long   int64;
typedef  unsigned long long uint64;

typedef  unsigned char      bool;
typedef  unsigned char      byte;

typedef struct JsonCursors
{
    byte* pRead;
    byte* pWrite;
    char* pError;
} JsonCursors;


/*
// the markers are actually an encoding that may contain data is self
6bit size [0-63]	    0	1	JsonMarkerSmallString
6bit size [0-63]        1	0	JsonMarkerSmallObject
6bit size [0-63]        1	1	JsonMarkerSmallArray
5bit int [0-31]	    1	0	0	JsonMarkerDecimal
4bit int [0-15] 1	0	0	0	JsonMarkerDigit
3bit [0-7]	1	0	0	0	0	JsonMarkerInt
1	0	0	0	0	0	0	0	JsonMarkerLargeString
1	0	1	0	0	0	0	0	JsonMarkerLargeObject
1	1	0	0	0	0	0	0	JsonMarkerLargeArray
1	1	1	0	0	0	0	0	JsonMarkerSequenceEnd
0	0	1	0	0	0	0	0	JsonMarkerNull
0	1	0	0	0	0	0	0	JsonMarkerTrue
0	1	1	0	0	0	0	0	JsonMarkerFalse
0	0	0	0	0	0	0	0	unused null

*/

typedef enum
{
    JsonMarkerSmallString = 0b00000001,//6bits reserved
    JsonMarkerSmallObject = 0b00000010,//6bits reserved
    JsonMarkerSmallArray = 0b00000011,//6bits reserved
    JsonMarkerExponent = 0b00000100,//5bits reserved
    JsonMarkerDigit = 0b00001000,//4bits reserved
    JsonMarkerInt = 0b00010000,//3bits reserved
    JsonMarkerLargeString = 0b10000000,
    JsonMarkerLargeObject = 0b10100000,
    JsonMarkerLargeArray = 0b11000000,
    JsonMarkerSequenceEnd = 0b11100000,
    JsonMarkerNull = 0b00100000,
    JsonMarkerTrue = 0b01000000,
    JsonMarkerFalse = 0b01100000,
} JsonMarker;

void Json_ParseUnkown(JsonCursors* oCursors);

byte* Json_SkipWhitespace(byte* pJson)
{

    while (*pJson > 0 && *pJson <= 32)
        pJson++;
    return pJson;
}
void Json_ParseObject(JsonCursors* oCursors)
{
    uint32 nLen = 2;//minimum object size
    byte* pMarker = oCursors->pWrite;//where to put the type, will be the { char
    oCursors->pWrite += 1;//leave space for the type
    oCursors->pRead = Json_SkipWhitespace(oCursors->pRead + 1); //skip the first { and spaces before first value
    byte sCurrent = 0;
    while (sCurrent = *(oCursors->pRead))
    {
        if (sCurrent == '}')//the reach the end of the object
        {
            *(oCursors->pWrite++) = JsonMarkerSequenceEnd;//put a signal marking the end
            oCursors->pRead++;
            if (nLen <= 63)
                *pMarker = (byte)((nLen << 2) | JsonMarkerSmallObject);
            else
                *pMarker = JsonMarkerLargeObject;
            return;
        }
        else if (sCurrent == ',')
        {
            oCursors->pRead = Json_SkipWhitespace(oCursors->pRead + 1); //skip the , and spaces before the next value
            if (*(oCursors->pRead) == ',') //{a,,b}
            {
                oCursors->pError = "unexpected ','";
                return;
            }
            if (*(oCursors->pRead) == '}') //{a,b,}
            {
                oCursors->pError = "trailing commas not suported";
                return;
            }
        }
        else if (sCurrent == '\0')
        {
            oCursors->pError = "unexpected end of stream";
            return;
        }
        else
        {
            byte* bBefore = oCursors->pWrite;
            Json_ParseUnkown(oCursors);
            if (oCursors->pError)
                return;
            oCursors->pRead = Json_SkipWhitespace(oCursors->pRead); //skip spaces after the key
            sCurrent = *(oCursors->pRead);
            if (sCurrent != ':')
            {
                oCursors->pError = "expected ':'";
                return;
            }
            oCursors->pRead += 1;//skip :
            oCursors->pRead = Json_SkipWhitespace(oCursors->pRead); //skip spaces before the value
            Json_ParseUnkown(oCursors);
            if (oCursors->pError)
                return;
            nLen += oCursors->pWrite - bBefore;//len of child
            oCursors->pRead = Json_SkipWhitespace(oCursors->pRead); //skip spaces after the value
        }
    }
}
void Json_ParseArray(JsonCursors* oCursors)
{
    int nLen = 2;
    byte* pMarker = oCursors->pWrite;//where to put the type, will be the [ char
    oCursors->pWrite += 1;//leave space for the type
    oCursors->pRead = Json_SkipWhitespace(oCursors->pRead + 1); //skip the first [ and spaces before first value
    byte sCurrent = 0;
    while (sCurrent = *(oCursors->pRead))
    {
        if (sCurrent == ']')//the reach the end of the array
        {
            *(oCursors->pWrite++) = JsonMarkerSequenceEnd;//put a signal marking the end
            oCursors->pRead++;
            if (nLen <= 63)
                *pMarker = (byte)((nLen << 2) | JsonMarkerSmallArray);
            else
                *pMarker = JsonMarkerLargeArray;

            return;
        }
        else if (sCurrent == ',')
        {
            oCursors->pRead = Json_SkipWhitespace(oCursors->pRead + 1); //skip the , and spaces before the next value
            if (*(oCursors->pRead) == ',') //[a,,b]
            {
                oCursors->pError = "unexpected ','";
                return;
            }
            if (*(oCursors->pRead) == ']') //[a,b,]
            {
                oCursors->pError = "trailing commas not suported";
                return;
            }
        }
        else
        {
            byte* bBefore = oCursors->pWrite;
            Json_ParseUnkown(oCursors);
            if (oCursors->pError)
                return;
            nLen += oCursors->pWrite - bBefore;
            oCursors->pRead = Json_SkipWhitespace(oCursors->pRead); //skip spaces after the value
        }
    }
}
void Json_ParseString(JsonCursors* oCursors)
{
    byte sCurrent = 0;
    int nLen = 2;
    byte* pMarker = oCursors->pWrite;//where to put the type, will be the " char
    oCursors->pWrite += 1;//leave space for the type
    oCursors->pRead += 1;//skip the first "
    oCursors->pError = "unexpected end of stream";//easier to clean this than to set
    while (sCurrent = *(oCursors->pRead))
    {
        if (sCurrent == '"')//the reached the end of string
        {
            *(oCursors->pWrite++) = '\0';//put a null terminator in the buffer to signal a null terminated string
            oCursors->pRead++;
            if (nLen <= 63)
                *pMarker = (byte)((nLen << 2) | JsonMarkerSmallString);
            else
                *pMarker = JsonMarkerLargeString;
            oCursors->pError = 0;
            return;
        }
        else if (sCurrent == '\\')//escaped character
        {
            oCursors->pRead += 1;
            byte sNext = *(oCursors->pRead);
            if (sNext == '\0')//whe reached the null char at end of buffer without finding a ' ou ", invalid string
                return;//"unexpected end of stream"
            else if (sNext == '"')//double quote
                *(oCursors->pWrite) = '"';
            else if (sNext == '\\')//back slash
                *(oCursors->pWrite) = '\\';
            else if (sNext == '/')//forward slash
                *(oCursors->pWrite) = '/';
            else if (sNext == 'b')//bell
                *(oCursors->pWrite) = '\b';
            else if (sNext == 'f')//form feed
                *(oCursors->pWrite) = '\f';
            else if (sNext == 'v')//vertical tab
                *(oCursors->pWrite) = '\v';
            else if (sNext == 'n')//new line
                *(oCursors->pWrite) = '\n';
            else if (sNext == 'r')//return
                *(oCursors->pWrite) = '\r';
            else if (sNext == 't')//tab
                *(oCursors->pWrite) = '\t';
            else if (sNext == 'u')//we are gonna read the next 4 hex chars
            {
                byte a = *(++(oCursors->pRead));
                if (a == '\0')
                    return;//"unexpected end of stream" 
                byte b = *(++(oCursors->pRead));
                if (b == '\0')
                    return;//"unexpected end of stream"
                byte c = *(++(oCursors->pRead));
                if (c == '\0')
                    return;//"unexpected end of stream"
                byte d = *(++(oCursors->pRead));
                if (d == '\0')
                    return;//"unexpected end of stream"

                if (!((a >= '0' && a <= '9') || (a >= 'a' && a <= 'f') || (a >= 'A' && a <= 'F'))
                    || !((b >= '0' && b <= '9') || (b >= 'a' && b <= 'f') || (b >= 'A' && b <= 'F'))
                    || !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
                    || !((d >= '0' && d <= '9') || (d >= 'a' && d <= 'f') || (d >= 'A' && d <= 'F')))
                {
                    oCursors->pError = "invalid unicode encoding";
                    return;
                }
                a = (a & 0xF) + 9 * (a >> 6);
                b = (b & 0xF) + 9 * (b >> 6);
                c = (c & 0xF) + 9 * (c >> 6);
                d = (d & 0xF) + 9 * (d >> 6);
                (*((uint16*)oCursors->pWrite)) = (uint16)((a << 12) | (b << 8) | (c << 4) | d);
                oCursors->pWrite += 1;
                nLen++; // the second byte will be incremented after the if
            }
            else
            {
                oCursors->pError = "invalid escape sequence";
                return;
            }
            nLen++;
        }
        else if (sCurrent == '\0')
            return;//"unexpected end of stream"
        else
        {
            *(oCursors->pWrite) = sCurrent;
            nLen++;
        }
        oCursors->pRead += 1;
        oCursors->pWrite += 1;
    }
}
void Json_ParseNumber(JsonCursors* oCursors)
{
    bool bIsNegative = 0;
    long long nMantissa = 0;
    long long nExponent = 0;
    int nDigits = 0;
    if (bIsNegative = *(oCursors->pRead) == '-')
        oCursors->pRead++;

    while (*(oCursors->pRead) >= '0' && *(oCursors->pRead) <= '9')
    {
        if (nMantissa < 9223372036854775797LL)
            nMantissa = (nMantissa * 10) + (*(oCursors->pRead)++ - '0');
        else
            nExponent++;
        nDigits++;
    }
    if (nDigits == 0)
    {
        oCursors->pError = "numerical value must start with a digit";
        return;
    }
    nDigits = 0;

    if (*(oCursors->pRead) == '.')//has decimals
    {
        oCursors->pRead++;//skip the .
        int nTrailingZeros = 0;
        while (*(oCursors->pRead) >= '0' && *(oCursors->pRead) <= '9')
        {
            nDigits++;
            if (nMantissa < 9223372036854775797LL)
            {
                char nDigit = (*(oCursors->pRead)++ - '0');
                if (nDigit == 0)//if have a 0, don't push the mantissa because if it is a trailing 0 we will be loosing accuracy for nothing 
                {
                    nTrailingZeros++;
                    continue;
                }
                if (nTrailingZeros > 0)//we found a digit AFTER some zeros.. we must push the mantissa
                {
                    int nOffset = (int64)pow(10, nTrailingZeros);
                    nMantissa *= nOffset;
                    nExponent -= nTrailingZeros;
                }
                nMantissa = (nMantissa * 10) + nDigit;
                nExponent--;
            }

        }
        if (nDigits == 0)
        {
            oCursors->pError = "numerical value must have a digits after decimal point";
            return;
        }
        nDigits = 0;
    }
    if (*(oCursors->pRead) == 'e' || *(oCursors->pRead) == 'e')//has exponent
    {
        long long nExplicitExponent = 0;
        bool bIsExplicitExponentNegative = 0;
        oCursors->pRead++;//skip the "e"
        if (bIsExplicitExponentNegative = *(oCursors->pRead) == '-')
            oCursors->pRead++;

        while (*(oCursors->pRead) >= '0' && *(oCursors->pRead) <= '9')
        {
            nExplicitExponent = (nExplicitExponent * 10) + (*(oCursors->pRead)++ - '0');
            nDigits++;
        }
        if (nDigits == 0)
        {
            oCursors->pError = "numerical value must have a digits after exponent ";
            return;
        }
        if (bIsExplicitExponentNegative)
            nExplicitExponent = -nExplicitExponent;

        nExponent += nExplicitExponent;
    }
    if (bIsNegative)
        nMantissa = -nMantissa;
    //double nValue = nMantissa * pow(10.0, nExponent);

    if (nExponent != 0)
    {
        if (nExponent < -16 || nExponent > 15)
        {
            oCursors->pError = "parser does not support more than 16 decimal places";
            return;
        }
        *(oCursors->pWrite) = (byte)((nExponent << 3) | JsonMarkerExponent);
        oCursors->pWrite += 1;
    }


    //write the int part
    if (nMantissa >= 0 && nMantissa < 10) // 0,1,2,2,4,5,6,7,8,9
    {
        *(oCursors->pWrite++) = (byte)((nMantissa << 4) | JsonMarkerDigit);
    }
    else if (nMantissa >= -128 && nMantissa <= 127)
    {
        *(oCursors->pWrite++) = (byte)((1 << 5) | JsonMarkerInt);
        *((int8*)oCursors->pWrite) = (int8)nMantissa;
        oCursors->pWrite += 1;
    }
    else if (nMantissa >= -32768 && nMantissa <= 32767)
    {
        *(oCursors->pWrite++) = (byte)((2 << 5) | JsonMarkerInt);
        *((int16*)oCursors->pWrite) = (int16)nMantissa;
        oCursors->pWrite += 2;
    }
    else if (nMantissa >= -2147483648 && nMantissa <= 2147483647)
    {
        *(oCursors->pWrite++) = (byte)((3 << 5) | JsonMarkerInt);
        *((int32*)oCursors->pWrite) = (int32)nMantissa;
        oCursors->pWrite += 4;
    }
    else if (nMantissa >= -9223372036854775806LL && nMantissa <= 9223372036854775807LL)
    {
        *(oCursors->pWrite++) = (byte)((4 << 5) | JsonMarkerInt);
        *((int64*)oCursors->pWrite) = (int64)nMantissa;
        oCursors->pWrite += 8;
    }
    else//out of range should not happen because double as less precision than int64
    {
        oCursors->pError = "numeric value out of range";
    }
}
void Json_ParseUnkown(JsonCursors* oCursors)
{
    oCursors->pRead = Json_SkipWhitespace(oCursors->pRead);
    byte sCurrent = *(oCursors->pRead);
    if (sCurrent == '{')
        Json_ParseObject(oCursors);
    else if (sCurrent == '[')
        Json_ParseArray(oCursors);
    else if (sCurrent == '"')
        Json_ParseString(oCursors);
    else if ((sCurrent >= '0' && sCurrent <= '9') || sCurrent == '-' || sCurrent == '.')
        Json_ParseNumber(oCursors);
    else if (sCurrent == 'n' && *(oCursors->pRead + 1) == 'u' && *(oCursors->pRead + 2) == 'l' && *(oCursors->pRead + 3) == 'l')
    {
        *(oCursors->pWrite) = JsonMarkerNull;
        oCursors->pWrite += 1;
        oCursors->pRead += 4;
    }
    else if (sCurrent == 'f' && *(oCursors->pRead + 1) == 'a' && *(oCursors->pRead + 2) == 'l' && *(oCursors->pRead + 3) == 's' && *(oCursors->pRead + 4) == 'e')
    {
        *(oCursors->pWrite) = JsonMarkerFalse;
        oCursors->pWrite += 1;
        oCursors->pRead += 5;
    }
    else if (sCurrent == 't' && *(oCursors->pRead + 1) == 'r' && *(oCursors->pRead + 2) == 'u' && *(oCursors->pRead + 3) == 'e')
    {
        *(oCursors->pWrite) = JsonMarkerTrue;
        oCursors->pWrite += 1;
        oCursors->pRead += 4;
    }
    else if (sCurrent == '\0')
        oCursors->pError = "Unexpected end of stream";
    else
        oCursors->pError = "Unexpected character";
}

/*************************
 * loading functions
**************************/
uint32 Json_GetSize(const byte* pJson)
{
    byte nType = *pJson;
    uint32 nSize = 0;
    switch (nType & 0x3)
    {
        case JsonMarkerSmallString:
        case JsonMarkerSmallObject:
        case JsonMarkerSmallArray:
            nSize = (nType >> 2);
            break;
        default:
            switch (nType)
            {
                case JsonMarkerNull:
                case JsonMarkerTrue:
                case JsonMarkerFalse:
                    nSize = 1;
                    break;
                case JsonMarkerLargeString:
                    nSize = strlen(pJson + 1) + 2;
                    break;
                case JsonMarkerLargeObject:
                case JsonMarkerLargeArray:
                    nSize = 2;
                    pJson++;
                    while (*pJson != JsonMarkerSequenceEnd)
                    {
                        uint32 nItemSize = Json_GetSize(pJson);
                        pJson += nItemSize;
                        nSize += nItemSize;
                    }
                    break;
                default:
                    if ((nType & 0b00000111) == JsonMarkerExponent)
                    {
                        nSize = 1 + Json_GetSize(pJson + 1);
                    }
                    else if ((nType & 0b00001111) == JsonMarkerDigit)
                        nSize = 1;
                    else if ((nType & 0b00011111) == JsonMarkerInt)
                    {
                        int nIntSize = nType >> 5;
                        if (nIntSize == 1)
                            nSize = 2;
                        else if (nIntSize == 2)
                            nSize = 3;
                        else if (nIntSize == 3)
                            nSize = 5;
                        else if (nIntSize == 4)
                            nSize = 9;
                    }
                    break;
            }
            break;
    }
    return nSize;
}

JsonObject Json_LoadUnkown(const byte* pJson)
{
    JsonObject oJson;
    oJson.Position = pJson;
    oJson.Type = JsonTypeInvalid;
    byte nType = *pJson;
    int nMainType = nType & 0x3;
    switch (nMainType)
    {
        case JsonMarkerSmallString:
            oJson.Type = JsonTypeString;
            oJson.StringValue = pJson + 1;
            break;
        case JsonMarkerSmallObject:
            oJson.Type = JsonTypeObject;
            break;
        case JsonMarkerSmallArray:
            oJson.Type = JsonTypeArray;
            break;
        default:
            switch (nType)
            {
                case JsonMarkerNull:
                    oJson.Type = JsonTypeNull;
                    break;
                case JsonMarkerTrue:
                    oJson.Type = JsonTypeBool;
                    oJson.BoolValue = 1;
                    break;
                case JsonMarkerFalse:
                    oJson.Type = JsonTypeBool;
                    oJson.BoolValue = 0;
                    break;
                case JsonMarkerLargeString:
                    oJson.Type = JsonTypeString;
                    oJson.StringValue = pJson + 1;
                    break;
                case JsonMarkerLargeObject:
                    oJson.Type = JsonTypeObject;
                    break;
                case JsonMarkerLargeArray:
                    oJson.Type = JsonTypeArray;
                    break;
                case JsonMarkerSequenceEnd:
                    oJson.Type = JsonTypeInvalid;
                    break;
                default:
                {
                    if ((nType & 0b00000111) == JsonMarkerExponent)
                    {
                        unsigned nEncoded = nType >> 3;
                        unsigned nMask = 0b00010000;
                        signed nExponent = (nEncoded ^ nMask) - nMask;
                        JsonObject oMantissa = Json_LoadUnkown(pJson + 1);
                        if (oMantissa.Type == JsonTypeNumber)
                        {
                            oJson.Type = JsonTypeNumber;
                            oJson.DoubleValue = oMantissa.DoubleValue * pow(10.0, nExponent);
                        }
                        else
                            oJson.Type = JsonTypeInvalid;

                    }
                    else if ((nType & 0b00001111) == JsonMarkerDigit)
                    {
                        oJson.Type = JsonTypeNumber;
                        int8 nDigit = (int8)(nType >> 4);
                        oJson.DoubleValue = (int8)nDigit;
                    }
                    else if ((nType & 0b00011111) == JsonMarkerInt)
                    {
                        oJson.Type = JsonTypeNumber;
                        byte nSize = nType >> 5;
                        if (nSize == 1)
                            oJson.DoubleValue = *((int8*)(pJson + 1));
                        else if (nSize == 2)
                            oJson.DoubleValue = *((int16*)(pJson + 1));
                        else if (nSize == 3)
                            oJson.DoubleValue = *((int32*)(pJson + 1));
                        else if (nSize == 4)
                            oJson.DoubleValue = *((int64*)(pJson + 1));
                        else
                            oJson.Type = JsonTypeInvalid;
                    }
                    else
                        oJson.Type = JsonTypeInvalid;
                }
                break;
            }
            break;
    }
    return oJson;
}

/******************************
* API Functions
*******************************/

JsonProperty Json_IterateProperties(JsonObject oJsonObject)
{
    JsonProperty oProperty;
    oProperty.Position = 0;
    oProperty.Name = 0;
    oProperty.Value.Type = JsonTypeInvalid;
    if (oJsonObject.Type != JsonTypeObject)
        return oProperty;
    oProperty.Position = oJsonObject.Position + 1;
    uint32 nKeySize = Json_GetSize(oProperty.Position);
    oProperty.Name = oProperty.Position + 1;
    oProperty.Value = Json_LoadUnkown(oProperty.Position + nKeySize);
    return oProperty;
}
JsonProperty Json_NextProperty(JsonProperty oJsonPreviousProperty)
{
    JsonProperty oProperty;
    oProperty.Position = 0;
    oProperty.Name = 0;
    oProperty.Value.Type = JsonTypeInvalid;
    if (oJsonPreviousProperty.Value.Type == JsonTypeInvalid || oJsonPreviousProperty.Position == 0)
        return oProperty;

    uint32 nPreviousKeySize = Json_GetSize(oJsonPreviousProperty.Position);
    uint32 nPreviousValueSize = Json_GetSize(oJsonPreviousProperty.Position + nPreviousKeySize);
    const  byte* pNextPosition = oJsonPreviousProperty.Position + nPreviousKeySize + nPreviousValueSize;
    if (*(pNextPosition) == JsonMarkerSequenceEnd)
        return oProperty;

    oProperty.Position = pNextPosition;
    uint32 nNextKeySize = Json_GetSize(oProperty.Position);
    oProperty.Name = oProperty.Position + 1;
    oProperty.Value = Json_LoadUnkown(oProperty.Position + nNextKeySize);
    return oProperty;
}

JsonProperty Json_GetPropertyByName(JsonObject oJsonObject, char* pName)
{
    JsonProperty oProperty;
    oProperty.Position = 0;
    oProperty.Name = 0;
    oProperty.Value.Type = JsonTypeInvalid;
    if (oJsonObject.Type != JsonTypeObject)
        return oProperty;
    oProperty.Position = oJsonObject.Position + 1;
    if (*oProperty.Position == JsonMarkerSequenceEnd)//empty object
        return oProperty;
    oProperty.Name = oProperty.Position + 1;
    while (strcmp(oProperty.Name, pName) != 0)
    {
        uint32 nKeySize = Json_GetSize(oProperty.Position);
        uint32 nValueSize = Json_GetSize(oProperty.Position + nKeySize);
        oProperty.Position += nKeySize + nValueSize;
        if (*oProperty.Position == JsonMarkerSequenceEnd)//reached end of object
            return oProperty;
        oProperty.Name = oProperty.Position + 1;
    }

    uint32 nKeySize = Json_GetSize(oProperty.Position);
    oProperty.Value = Json_LoadUnkown(oProperty.Position + nKeySize);
    return oProperty;
}
int Json_GetPropertyCount(JsonObject oJsonObject)
{
    if (oJsonObject.Type != JsonTypeObject)
        return -1;
    const byte* pPosition = oJsonObject.Position + 1;
    int nCount = 0;
    while (*pPosition != JsonMarkerSequenceEnd)
    {
        nCount++;
        uint32 nKeySize = Json_GetSize(pPosition);
        uint32 nValueSize = Json_GetSize(pPosition + nKeySize);
        pPosition += nKeySize + nValueSize;
    }
    return nCount;
}

JsonElement Json_IterateElements(JsonObject oJsonArray)
{
    JsonElement oElement;
    oElement.Position = 0;
    oElement.Index = 0;
    oElement.Value.Type = JsonTypeInvalid;
    if (oJsonArray.Type != JsonTypeArray)
        return oElement;
    oElement.Position = oJsonArray.Position + 1;
    oElement.Index = 0;
    oElement.Value = Json_LoadUnkown(oElement.Position);
    return oElement;
}
JsonElement Json_NextElement(JsonElement oJsonPreviousElement)
{
    JsonElement oElement;
    oElement.Position = 0;
    oElement.Index = 0;
    oElement.Value.Type = JsonTypeInvalid;
    if (oJsonPreviousElement.Value.Type == JsonTypeInvalid || oJsonPreviousElement.Position == 0)
        return oElement;

    uint32 nPreviousValueSize = Json_GetSize(oJsonPreviousElement.Position);
    const byte* pNextPosition = oJsonPreviousElement.Position + nPreviousValueSize;
    if (*(pNextPosition) == JsonMarkerSequenceEnd)
        return oElement;

    oElement.Position = pNextPosition;
    oElement.Index = oJsonPreviousElement.Index + 1;
    oElement.Value = Json_LoadUnkown(oElement.Position);
    return oElement;
}

JsonElement Json_GetElementAtIndex(JsonObject oJsonArray, int nIndex)
{
    JsonElement oElement;
    oElement.Position = 0;
    oElement.Index = 0;
    oElement.Value.Type = JsonTypeInvalid;
    if (oJsonArray.Type != JsonTypeArray)
        return oElement;
    oElement.Position = oJsonArray.Position + 1;
    if (*oElement.Position == JsonMarkerSequenceEnd)//empty array
        return oElement;
    while (nIndex > 0)
    {
        uint32 nValueSize = Json_GetSize(oElement.Position);
        oElement.Position += nValueSize;
        if (*oElement.Position == JsonMarkerSequenceEnd)//reached end of object
            return oElement;
        oElement.Index++;
        nIndex--;
    }
    oElement.Value = Json_LoadUnkown(oElement.Position);
    return oElement;
}

int Json_GetElementCount(JsonObject oJsonArray)
{
    if (oJsonArray.Type != JsonTypeArray)
        return -1;
    const byte* pPosition = oJsonArray.Position + 1;
    int nCount = 0;
    while (*pPosition != JsonMarkerSequenceEnd)
    {
        nCount++;
        uint32 nValueSize = Json_GetSize(pPosition);
        pPosition += nValueSize;
    }
    return nCount;
}

JsonResult Json_Parse(char* pJson)
{
    JsonResult oResult;
    oResult.InitialSize = 0;
    oResult.EndSize = 0;
    oResult.Success = 0;
    oResult.Index = -1;
    oResult.Error = 0;
    oResult.RootObject.Type = JsonTypeInvalid;

    JsonCursors oCursors;
    oCursors.pRead = pJson;
    oCursors.pWrite = pJson;
    oCursors.pError = 0;
    Json_ParseUnkown(&oCursors);
    if (oCursors.pError)
    {
        oResult.Error = oCursors.pError;
        oResult.Index = (int)(oCursors.pRead - (byte*)pJson);
        return oResult;
    }
    else
    {
        oResult.InitialSize = (int)(oCursors.pRead - (byte*)pJson);
        oResult.EndSize = (int)(oCursors.pWrite - (byte*)pJson);
        while (*(oCursors.pWrite) != 0)
            *(oCursors.pWrite++) = 0;
        oResult.RootObject = Json_LoadUnkown((unsigned char*)pJson);
        oResult.Success = 1;

    }
    return  oResult;
}
JsonObject Json_Load(const char* pJson)
{
    return Json_LoadUnkown((byte*)pJson);
}
