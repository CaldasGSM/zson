#include <stdio.h>
#include <stdlib.h>
#include "json.h"

//just to make like easer
#define bool int
#define false 0
#define true 1

int run_directory();
int run_file(const char* filename);
bool test_parse(const char* filename);
bool test_iterators(const char* filename);
bool iterate_object(JsonObject oJson);
bool test_write(const char* filename);

//helper function
char* read_content(const char* filename)
{
    FILE* hFile = fopen(filename, "rb");
    fseek(hFile, 0, SEEK_END);
    long nSize = ftell(hFile);
    fseek(hFile, 0, SEEK_SET);  /* same as rewind(f); */

    char* pBuffer = (char*)malloc(nSize + 1);
    fread(pBuffer, nSize, 1, hFile);
    pBuffer[nSize] = '\0';
    fclose(hFile);
    return pBuffer;
}
void dump_object(JsonObject oJson, int nLevel)
{
    switch (oJson.Type)
    {
        case JsonTypeArray:
            printf("%*s array(%i)\n", nLevel * 4, "", Json_GetElementCount(oJson));
            for (JsonElement oElement = Json_IterateElements(oJson); oElement.Value.Type != JsonTypeInvalid; oElement = Json_NextElement(oElement))
            {
                printf("%*s element(%i)\n", (nLevel + 1) * 4, "", oElement.Index);
                dump_object(oElement.Value, nLevel + 1);

            }
            break;
        case JsonTypeBool:
            printf("%*s bool(%s)\n", nLevel * 4, "", oJson.BoolValue ? "true" : "false");
            break;
        case JsonTypeNull:
            printf("%*s null\n", nLevel * 4, "");
            break;
        case JsonTypeNumber:
            printf("%*s number(%f)\n", nLevel * 4, "", oJson.DoubleValue);
            break;
        case JsonTypeObject:
            printf("%*s object(%i)\n", nLevel * 4, "", Json_GetPropertyCount(oJson));
            for (JsonProperty oProperty = Json_IterateProperties(oJson); oProperty.Value.Type != JsonTypeInvalid; oProperty = Json_NextProperty(oProperty))
            {
                printf("%*s property(%s)\n", (nLevel + 1) * 4, "", oProperty.Name);
                dump_object(oProperty.Value, nLevel + 1);

            }
            break;
        case JsonTypeString:
            printf("%*s string(%s)\n", nLevel * 4, "", oJson.StringValue);
            break;
        default:
        case JsonTypeInvalid:
            printf("%*s invalid\n", nLevel * 4, "");
            break;
    }
}