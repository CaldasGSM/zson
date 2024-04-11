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

int Json_MeasureSize(char* pChar)
{
    char sChar = 0;
    int nDepth = 0;
    int nDepthIncrement = 4;
    int nOutputSizeSize = 0;
    while (sChar = *pChar++)
    {
        switch (sChar)
        {
            case '{':
            case '[':
                nOutputSizeSize += 1;//char
                nOutputSizeSize += 1;//\n
                nDepth += nDepthIncrement;
                nOutputSizeSize += nDepth;//ident

                break;
            case '}':
            case ']':
                nOutputSizeSize += 1;//\n
                nDepth -= nDepthIncrement;
                if (nDepth < 0)//bad json text
                    return -1;
                nOutputSizeSize += nDepth;//ident
                nOutputSizeSize += 1;//char
                break;
            case '"':
                int nStringSize = 1;//open "
                while (sChar && (sChar = *pChar++) != '"')
                {
                    if (sChar == '\\')//escaped pair
                    {
                        pChar++;
                        nStringSize++;
                    }
                    nStringSize++;
                }
                nStringSize++;//close "
                nOutputSizeSize += nStringSize;
            case ',':
                nOutputSizeSize += 1;//char
                nOutputSizeSize += 1;//\n
                nOutputSizeSize += nDepth;//ident
                break;
            case ':':
                nOutputSizeSize += 3;// : 
                break;
            default:
                nOutputSizeSize += 1;// char 
                break;
        }
    }
    return nOutputSizeSize;
}

char* Json_Indent(char* pChar)
{
    int nOutputSizeSize = Json_MeasureSize(pChar);
    if (nOutputSizeSize < 0)
        return 0;

    char* pOut = malloc(nOutputSizeSize + 1);
    char* pReturn = pOut;
    //memset(pOut, '-', nOutputSizeSize + 1);
    *(pOut + nOutputSizeSize) = '\0';
    char sChar = 0;
    int nDepth = 0;
    int nDepthIncrement = 4;
    char sDepthChar = ' ';
    int bIsScopeOpening = 0;
    while (sChar = *pChar++)
    {
        if (sChar <= 32)//skip spaces and white chars
            continue;
        if (bIsScopeOpening && sChar != '}' && sChar != '}')
        {
            nDepth += nDepthIncrement;
            *pOut++ = '\n';
            for (int i = 0;i < nDepth;i++)
                *pOut++ = sDepthChar;
        }
        switch (sChar)
        {
            case '{':
            case '[':
                bIsScopeOpening = 1;
                *pOut++ = sChar;

                break;

            case '}':
            case ']':
                if (!bIsScopeOpening)
                {
                    *pOut++ = '\n';
                    nDepth -= nDepthIncrement;
                    for (int i = 0;i < nDepth;i++)
                        *pOut++ = sDepthChar;
                }
                *pOut++ = sChar;
                bIsScopeOpening = 0;
                break;

            case '"':
                bIsScopeOpening = 0;
                *pOut++ = '"';
                while (sChar && (sChar = *pChar++) != '"')
                {
                    if (sChar == '\\')//escaped pair
                    {
                        *pOut++ = sChar;
                        *pOut++ = *pChar++;
                    }
                    else
                        *pOut++ = sChar;
                }
                *pOut++ = '"';
                break;
            case ',':
                bIsScopeOpening = 0;
                *pOut++ = ',';
                *pOut++ = '\n';
                for (int i = 0;i < nDepth;i++)
                    *pOut++ = sDepthChar;
                break;
            case ':':
                bIsScopeOpening = 0;
                *pOut++ = ' ';
                *pOut++ = ':';
                *pOut++ = ' ';
                break;
            default:
                bIsScopeOpening = 0;
                *pOut++ = sChar;// char
                break;
        }
    }

    *pOut = '\0';
    return pReturn;
}

char* Json_Compress(char* pChar)
{
    char* pRead = pChar;
    char* pWrite = pChar;
    char sChar = 0;
    while (sChar = *pRead++)
    {
        if (sChar <= 32)
            continue;
        if (sChar == '"')
        {
            *pWrite++ = '"';
            while (sChar && (sChar = *pRead++) != '"')
            {
                if (sChar == '\\')//escaped pair
                {
                    *pWrite++ = sChar;//write the backslash 
                    sChar = *pRead++;//read next
                }
                *pWrite++ = sChar;
            }
            *pWrite++ = '"';
            continue;
        }
        *pWrite++ = sChar;
    }
    *pWrite = '\0';
    return pChar;
}