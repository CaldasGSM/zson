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

typedef struct JsonHeader
{
    int AllocatedSize;
    int StringSize;
    byte* InsertionPoint;
    char* Error;
} JsonHeader;

int Json_SizeOfString(const char* pString)
{
    char pUnescaped[] = { '"','\\','/','\b','\f','\n','\r','\t','\v' };
    int nSize = 0;
    char sChar = '\0';
    while (sChar = *pString++)
    {
        nSize++;
        for (int i = 0; i < 9;i++)
            if (sChar == pUnescaped[i])
            {
                nSize++;
                break;
            }
    }
    return nSize;
}
void Json_WriteString(char* pDest, const char* pSource)
{
    char pUnescaped[] = { '"','\\','/','\b','\f','\n','\r','\t','\v' };
    char pEscaped[] = { '"', '\\','/','b', 'f', 'n', 'r', 't', 'v' };

    char sChar = '\0';
    int i = 0;
    while (sChar = *pSource++)
    {
        for (i = 0; i < 9;i++)
            if (sChar == pUnescaped[i])
            {
                *pDest++ = '\\';
                *pDest++ = pEscaped[i];
                break;
            }
        if (i == 9)
            *pDest++ = sChar;
    }
}
int Json_SizeOfNumber(double nValue)
{
    int bNegative = nValue < 0;
    if (bNegative)
        nValue = -nValue;
    //count decimal
    uint32 nDecimalDigits = 0;
    double nEpsilon = 2.2204460492503131e-016;//DBL_EPSILON
    double nExponent = 1.0;
    while (fabs(nValue * nExponent - trunc(nValue * nExponent)) > nEpsilon)
    {
        ++nDecimalDigits;

        nEpsilon *= 10;
        nExponent *= 10;
    }
    //count it
    uint32 nIntegerDigits = (uint32)fmax(1, floor(log10(nValue)) + 1);


    return (bNegative ? 1 : 0) + nIntegerDigits + (nDecimalDigits > 0 ? 1 : 0) + nDecimalDigits;// ,-<intsize>.<decimal>
}
void Json_WriteNumber(char* pDest, double nValue)
{
    int bNegative = nValue < 0;
    if (bNegative)
        nValue = -nValue;
    uint32 nDecimalDigits = 0;
    double nEpsilon = 2.2204460492503131e-016;//DBL_EPSILON
    double nExponent = 1.0;
    while (fabs(nValue * nExponent - trunc(nValue * nExponent)) > nEpsilon)
    {
        ++nDecimalDigits;

        nEpsilon *= 10;
        nExponent *= 10;
    }
    //decimal part
    uint64 nDigits = nValue * pow(10, nDecimalDigits);//make int
    if (nDecimalDigits > 0)
    {
        for (int i = 0; i < nDecimalDigits;i++)
        {
            byte nDigit = nDigits % 10;
            *--pDest = '0' + nDigit;
            nDigits /= 10;
        }
        *--pDest = '.';
    }
    //integer part (write at least a single 0)
    do
    {
        byte nDigit = nDigits % 10;
        *--pDest = '0' + nDigit;
        nDigits /= 10;
    } while (nDigits > 0);

    if (bNegative)
        *--pDest = '-';
}

char* Json_ValidateScope(JsonHeader* pHeader, int* bFirst, int bHasName)
{
    int bIsArrayScope = (*pHeader->InsertionPoint == ']');
    int bIsObjectScope = (*pHeader->InsertionPoint == '}');
    int bIsRootScope = (*pHeader->InsertionPoint == '\0');
    if (bIsArrayScope)
        *bFirst = *(pHeader->InsertionPoint - 1) == '[';
    else if (bIsObjectScope)
        *bFirst = *(pHeader->InsertionPoint - 1) == '{';
    else if (bIsRootScope)
        *bFirst = *(pHeader->InsertionPoint - 1) == '\0';
    else
        return "Invalid insertion point";

    if (bIsRootScope)
    {
        if (*bFirst)//we can add anything
            return 0;
        else
            return "Only one value can be root of a json document";
    }
    else if (bIsObjectScope)
    {
        if (bHasName)//is ok to add named items inside a object scope
            return 0;
        else
            return "Inside an object scope you must add a named property";

    }
    else if (bIsArrayScope)
    {
        if (!bHasName)//arrays dont have named items
            return 0;
        else
            return "Inside an array scope can't add a named property";
    }
}

#define packheader(pHeader) ((char*)pHeader + sizeof(JsonHeader) + 1) 
#define unpackheader(pBuffer) ((JsonHeader*)((char*)pBuffer - (sizeof(JsonHeader) + 1)))

char* Json_CreateBuffer()
{
    int nHeaderSize = sizeof(JsonHeader) + 2;
    JsonHeader* pHeader = malloc(nHeaderSize);
    memset(pHeader, 0, nHeaderSize);
    pHeader->AllocatedSize = nHeaderSize;
    pHeader->StringSize = 0;
    pHeader->InsertionPoint = ((byte*)pHeader) + nHeaderSize - 1;
    return packheader(pHeader);
}

void Json_ReleaseBuffer(char* pBuffer)
{
    int nHeaderSize = sizeof(JsonHeader) + 1;
    pBuffer -= sizeof(JsonHeader) + 1;
    free(pBuffer);
}
JsonHeader* Json_CreateGap(JsonHeader* pHeader, int nSize)
{
    if (nSize <= 0)//no space requested
        return pHeader;
    int nSizeOfHeader = sizeof(JsonHeader) + 2;//the header is and string is implicitly null terminated
    int nStringAllocated = pHeader->AllocatedSize - nSizeOfHeader;
    int nStringAvailable = nStringAllocated - pHeader->StringSize;
    if (nStringAvailable < nSize)//need to reallocate
    {
        //get a powerof 2 size
        int nMinTotalSize = nSizeOfHeader + pHeader->StringSize + nSize;
        int nIdealSize = 32;//minimum start size because we already have  26 at the start
        while (nIdealSize < nMinTotalSize)
            if (nIdealSize >= 4096)
                nIdealSize += 4096;
            else
                nIdealSize *= 2;
        //update info
        int nInsertionOffset = pHeader->InsertionPoint - (byte*)pHeader;
        pHeader = realloc(pHeader, nIdealSize);
        pHeader->AllocatedSize = nIdealSize;
        pHeader->InsertionPoint = ((byte*)pHeader) + nInsertionOffset;

#ifdef DEBUG
        memset(((byte*)pHeader) + nSizeOfHeader + pHeader->StringSize, '-', nIdealSize - (nSizeOfHeader + pHeader->StringSize));
#endif        
    }
    byte* pStringStart = ((byte*)pHeader) + sizeof(JsonHeader) + 1;
    byte* pStringEnd = pStringStart + pHeader->StringSize + 1; //include terminating null
    int nInsertionSuffix = pStringEnd - pHeader->InsertionPoint;
    memmove(pHeader->InsertionPoint + nSize, pHeader->InsertionPoint, nInsertionSuffix);
#ifdef DEBUG
    memset(pHeader->InsertionPoint, '-', nSize);
#endif
    return pHeader;
}

int Json_AddObject(char** pBuffer)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 0))
        return 0;

    int nGapSize = 2 + (bFirst ? 0 : 1);
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '{';
    *(pHeader->InsertionPoint) = '}';

    pHeader->StringSize += nGapSize;
    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_AddArray(char** pBuffer)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 0))
        return 0;

    int nGapSize = 2 + (bFirst ? 0 : 1);
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '[';
    *(pHeader->InsertionPoint) = ']';

    pHeader->StringSize += nGapSize;
    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_ExitScope(char** pBuffer)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    if (*pHeader->InsertionPoint != '}' && *pHeader->InsertionPoint != ']' && *pHeader->InsertionPoint != ']')
    {
        pHeader->Error = "Append cursor is not inside a valid scope";
        return 0;
    }
    pHeader->InsertionPoint++;
    return 1;
}

const char* Json_GetError(char* pBuffer)
{
    JsonHeader* pHeader = unpackheader(pBuffer);
    return pHeader->Error;
}

int Json_AddNull(char** pBuffer)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 0))
        return 0;

    int nGapSize = 4 + (bFirst ? 0 : 1);
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    memcpy(pHeader->InsertionPoint, "null", 4);
    pHeader->InsertionPoint += 4;

    pHeader->StringSize += nGapSize;
    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_AddBool(char** pBuffer, int bValue)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 0))
        return 0;

    int nValueSize = bValue ? 4 : 5;
    int nGapSize = (bFirst ? 0 : 1) + nValueSize;

    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    memcpy(pHeader->InsertionPoint, bValue ? "true" : "false", nValueSize);
    pHeader->InsertionPoint += nValueSize;
    pHeader->StringSize += nGapSize;

    *pBuffer = packheader(pHeader);
    return 1;
}


int Json_AddString(char** pBuffer, const char* sValue)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 0))
        return 0;

    int nValueSize = Json_SizeOfString(sValue);
    int nGapSize = (bFirst ? 0 : 1) + nValueSize + 2;// ,"<sname>"
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sValue);
    pHeader->InsertionPoint += nValueSize;
    *(pHeader->InsertionPoint)++ = '"';
    pHeader->StringSize += nGapSize;

    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_AddNumber(char** pBuffer, double nValue)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 0))
        return 0;

    int nValueSize = Json_SizeOfNumber(nValue);
    int nGapSize = (bFirst ? 0 : 1) + nValueSize;
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint) = ',';
    pHeader->InsertionPoint += nGapSize;
    Json_WriteNumber(pHeader->InsertionPoint, nValue);
    pHeader->StringSize += nGapSize;
    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_AddPropertyNull(char** pBuffer, const char* sName)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 1))
        return 0;

    int nNameSize = Json_SizeOfString(sName);
    int nGapSize = (bFirst ? 0 : 1) + nNameSize + 7;// ,"<sname>":null
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sName);
    pHeader->InsertionPoint += nNameSize;
    memcpy(pHeader->InsertionPoint, "\":null", 6);
    pHeader->InsertionPoint += 6;
    pHeader->StringSize += nGapSize;

    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_AddPropertyBool(char** pBuffer, const char* sName, int bValue)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 1))
        return 0;

    int nNameSize = Json_SizeOfString(sName);
    int nValueSize = bValue ? 4 : 5;
    int nGapSize = (bFirst ? 0 : 1) + nNameSize + 3 + nValueSize;// ,"<sname>":<value>
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sName);
    pHeader->InsertionPoint += nNameSize;
    *(pHeader->InsertionPoint)++ = '"';
    *(pHeader->InsertionPoint)++ = ':';
    memcpy(pHeader->InsertionPoint, bValue ? "true" : "false", nValueSize);
    pHeader->InsertionPoint += nValueSize;
    pHeader->StringSize += nGapSize;

    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_AddPropertyString(char** pBuffer, const char* sName, const char* sValue)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 1))
        return 0;

    int nNameSize = Json_SizeOfString(sName);
    int nValueSize = Json_SizeOfString(sValue);
    int nGapSize = (bFirst ? 0 : 1) + nNameSize + 5 + nValueSize;// ,"<sname>":"<svalue>"
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sName);
    pHeader->InsertionPoint += nNameSize;
    *(pHeader->InsertionPoint)++ = '"';
    *(pHeader->InsertionPoint)++ = ':';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sValue);
    pHeader->InsertionPoint += nValueSize;
    *(pHeader->InsertionPoint)++ = '"';
    pHeader->StringSize += nGapSize;

    *pBuffer = packheader(pHeader);
    return 1;
}
int Json_AddPropertyNumber(char** pBuffer, const char* sName, double nValue)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 1))
        return 0;

    int nNameSize = Json_SizeOfString(sName);
    int nValueSize = Json_SizeOfNumber(nValue);
    int nGapSize = (bFirst ? 0 : 1) + nNameSize + 3 + nValueSize;// ,"<sname>":<svalue>
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sName);
    pHeader->InsertionPoint += nNameSize;
    *(pHeader->InsertionPoint)++ = '"';
    *(pHeader->InsertionPoint)++ = ':';

    pHeader->InsertionPoint += nValueSize;
    Json_WriteNumber(pHeader->InsertionPoint, nValue);

    pHeader->StringSize += nGapSize;

    *pBuffer = packheader(pHeader);
    return 1;
}

int Json_AddPropertyArray(char** pBuffer, const char* sName)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 1))
        return 0;

    int nNameSize = Json_SizeOfString(sName);
    int nGapSize = (bFirst ? 0 : 1) + nNameSize + 5;// ,"<sname>":[]
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sName);
    pHeader->InsertionPoint += nNameSize;
    memcpy(pHeader->InsertionPoint, "\":[]", 4);
    pHeader->InsertionPoint += 3;
    pHeader->StringSize += nGapSize;
    *pBuffer = packheader(pHeader);
    return 1;
}
int Json_AddPropertyObject(char** pBuffer, const char* sName)
{
    JsonHeader* pHeader = unpackheader(*pBuffer);
    int bFirst = 0;
    if (pHeader->Error = Json_ValidateScope(pHeader, &bFirst, 1))
        return 0;

    int nNameSize = Json_SizeOfString(sName);
    int nGapSize = (bFirst ? 0 : 1) + nNameSize + 5;// ,"<sname>":[]
    pHeader = Json_CreateGap(pHeader, nGapSize);
    if (!bFirst)
        *(pHeader->InsertionPoint)++ = ',';
    *(pHeader->InsertionPoint)++ = '"';
    Json_WriteString(pHeader->InsertionPoint, sName);
    pHeader->InsertionPoint += nNameSize;
    memcpy(pHeader->InsertionPoint, "\":{}", 4);
    pHeader->InsertionPoint += 3;
    pHeader->StringSize += nGapSize;
    *pBuffer = packheader(pHeader);
    return 1;
}