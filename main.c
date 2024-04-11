#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#include "main.h"



void write_object(char** pBuffer, JsonObject oJson);
void write_property(char** pBuffer, JsonProperty oProperty);

double parse_decimal(const char* pChars)
{
    bool bIsNegative = 0;
    long long nMantissa = 0;
    long long nExponent = 0;
    long long nExplicitExponent = 0;
    bool bIsExplicitExponentNegative = 0;
    if (bIsNegative = *(pChars) == '-')
        pChars++;

    while (*(pChars) >= '0' && *(pChars) <= '9')
    {
        if (nMantissa < 9223372036854775797LL)
            nMantissa = (nMantissa * 10) + (*(pChars)++ - '0');
        else
            nExponent++;
    }

    if (*(pChars) == '.')//has decimals
    {
        pChars++;//skip the .
        while (*(pChars) >= '0' && *(pChars) <= '9')
        {
            if (nMantissa < 9223372036854775797LL)
            {
                nMantissa = (nMantissa * 10) + (*(pChars)++ - '0');
                nExponent--;
            }
        }
    }

    if (*(pChars) == 'e' || *(pChars) == 'e')//has exponent
    {
        pChars++;//skip the ^
        if (bIsExplicitExponentNegative = *(pChars) == '-')
            pChars++;

        while (*(pChars) >= '0' && *(pChars) <= '9')
        {
            nExplicitExponent = (nExplicitExponent * 10) + (*(pChars)++ - '0');
        }
        if (bIsExplicitExponentNegative)
            nExplicitExponent = -nExplicitExponent;

        nExponent += nExplicitExponent;
    }


    if (bIsNegative)
        nMantissa = -nMantissa;
    double nValue = nMantissa * pow(10.0, nExponent);
    return nValue;
}

int main()
{
    if (run_directory())
        printf("Tests run successfully.");
}


/// @brief  Run tests for all files in folder
bool run_directory()
{

    DIR* hDir = opendir("tests");

    if (hDir == NULL)
    {
        printf("Could not open current directory");
        return false;
    }

    struct dirent* pFile;
    char sPath[255] = "";
    while ((pFile = readdir(hDir)) != NULL)
    {
        if (pFile->d_name[0] == '.')
            continue;

        sPath[0] = '\0';
        strcat(sPath, "tests\\");
        strcat(sPath, pFile->d_name);
        if (!run_file(sPath))
        {
            closedir(hDir);
            return false;
        }
    }
    closedir(hDir);
    return true;
}


/// @brief  Run tests for all files in folder
/// @param filename The path to the file
bool run_file(const char* filename)
{
    printf("Testing : %s\n", filename);
    if (!test_parse(filename))
        return false;
    if (!test_iterators(filename))
        return false;
    if (!test_write(filename))
        return false;
    return true;
}

/// @brief runs the parser on the file
/// @param filename 
bool test_parse(const char* filename)
{
    char* pContent = read_content(filename);
    JsonResult oResult = Json_Parse(pContent);
    // dump_object(oResult.RootObject, 0);
    // exit(0);
    if (!oResult.Success)
    {
        printf("parsing failed : %s at %i\n", oResult.Error, oResult.Index);
        free(pContent);
        return false;
    }
    else
    {
        printf("parsing successful : %i > %i = %f%%\n", oResult.InitialSize, oResult.EndSize, ((float)oResult.EndSize / (float)oResult.InitialSize));
        free(pContent);
        return true;
    }
}

/// @brief runs the iterators on the parsed on the file
/// @param filename 
bool test_iterators(const char* filename)
{
    char* pContent = read_content(filename);
    JsonResult oResult = Json_Parse(pContent);
    bool bResult = iterate_object(oResult.RootObject);
    free(pContent);
    if (bResult == true)
        printf("Iterated without errors.\n");
    return bResult;
}

bool iterate_object(JsonObject oJsonObject)
{
    switch (oJsonObject.Type)
    {
        case JsonTypeArray:
            int nElements = Json_GetElementCount(oJsonObject);
            for (JsonElement oElement = Json_IterateElements(oJsonObject); oElement.Value.Type != JsonTypeInvalid; oElement = Json_NextElement(oElement))
            {
                int nIndex = oElement.Index;
                JsonObject oElementValue = oElement.Value;
                if (!iterate_object(oElementValue))
                    return false;

            }
            break;
        case JsonTypeBool:
            int bValue = oJsonObject.BoolValue;
            break;
        case JsonTypeNull:
            //NOP
            break;
        case JsonTypeNumber:
            double nValue = oJsonObject.DoubleValue;
            break;
        case JsonTypeObject:
            int nProperties = Json_GetPropertyCount(oJsonObject);
            for (JsonProperty oProperty = Json_IterateProperties(oJsonObject); oProperty.Value.Type != JsonTypeInvalid; oProperty = Json_NextProperty(oProperty))
            {
                const char* sName = oProperty.Name;
                JsonObject oPropertyValue = oProperty.Value;
                if (!iterate_object(oPropertyValue))
                    return false;
            }
            break;
        case JsonTypeString:
            const char* sName = oJsonObject.StringValue;
            break;
        default:
        case JsonTypeInvalid:
            printf("JsonTypeInvalid found at %i", oJsonObject.Position);
            return false;
            break;
    }
    return true;
}

/// @brief runs the iterators on the parsed on the file
/// @param filename 
bool test_write(const char* filename)
{
    char* pContent = read_content(filename);
    char* pOriginal = (char*)malloc(strlen(pContent) + 1);
    strcpy(pOriginal, pContent);
    JsonResult oResult = Json_Parse(pContent);

    char* pOutput = Json_CreateBuffer();
    write_object(&pOutput, oResult.RootObject);

    Json_Compress(pOriginal);
    int nComparison = strcmp(pOriginal, pOutput);
    if (nComparison == 0)
    {
        printf("JSON Generated without errors\n");
    }
    else
    {
        printf("Generated JSON text does not match original\n");
        printf("%s\n\n%s", pOriginal, pOutput);
    }
    free(pContent);
    free(pOriginal);
    Json_ReleaseBuffer(pOutput);
    return nComparison == 0;
}

void write_object(char** pBuffer, JsonObject oJson)
{
    switch (oJson.Type)
    {
        case JsonTypeArray:
            Json_AddArray(pBuffer);
            for (JsonElement oElement = Json_IterateElements(oJson); oElement.Value.Type != JsonTypeInvalid; oElement = Json_NextElement(oElement))
                write_object(pBuffer, oElement.Value);
            Json_ExitScope(pBuffer);
            break;
        case JsonTypeBool:
            Json_AddBool(pBuffer, oJson.BoolValue);
            break;
        case JsonTypeNull:
            Json_AddNull(pBuffer);
            break;
        case JsonTypeNumber:
            Json_AddNumber(pBuffer, oJson.DoubleValue);
            break;

        case JsonTypeString:
            Json_AddString(pBuffer, oJson.StringValue);
            break;
        default:
        case JsonTypeInvalid:
            break;
        case JsonTypeObject:
            Json_AddObject(pBuffer);
            for (JsonProperty oProperty = Json_IterateProperties(oJson); oProperty.Value.Type != JsonTypeInvalid; oProperty = Json_NextProperty(oProperty))
                write_property(pBuffer, oProperty);
            Json_ExitScope(pBuffer);
            break;
    }
}

void write_property(char** pBuffer, JsonProperty oProperty)
{
    switch (oProperty.Value.Type)
    {
        case JsonTypeArray:
            Json_AddPropertyArray(pBuffer, oProperty.Name);
            for (JsonElement oElement = Json_IterateElements(oProperty.Value); oElement.Value.Type != JsonTypeInvalid; oElement = Json_NextElement(oElement))
                write_object(pBuffer, oElement.Value);
            Json_ExitScope(pBuffer);
            break;
        case JsonTypeBool:
            Json_AddPropertyBool(pBuffer, oProperty.Name, oProperty.Value.BoolValue);
            break;
        case JsonTypeNull:
            Json_AddPropertyNull(pBuffer, oProperty.Name);
            break;
        case JsonTypeNumber:
            Json_AddPropertyNumber(pBuffer, oProperty.Name, oProperty.Value.DoubleValue);
            break;
        case JsonTypeString:
            Json_AddPropertyString(pBuffer, oProperty.Name, oProperty.Value.StringValue);
            break;
        default:
        case JsonTypeInvalid:
            break;
        case JsonTypeObject:
            Json_AddPropertyObject(pBuffer, oProperty.Name);
            for (JsonProperty oSubProperty = Json_IterateProperties(oProperty.Value); oSubProperty.Value.Type != JsonTypeInvalid; oSubProperty = Json_NextProperty(oSubProperty))
                write_property(pBuffer, oSubProperty);
            Json_ExitScope(pBuffer);
            break;
    }
}