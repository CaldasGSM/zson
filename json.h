/********************************
json reading
*********************************/
typedef enum
{
    JsonTypeInvalid = 0,
    JsonTypeNull = 1,
    JsonTypeBool = 2,
    JsonTypeNumber = 3,
    JsonTypeString = 4,
    JsonTypeObject = 5,
    JsonTypeArray = 6,
} JsonType;

typedef struct JsonObject
{
    const unsigned char* Position;
    JsonType Type;
    union
    {
        const char* StringValue;
        double DoubleValue;
        unsigned char BoolValue;
    };
} JsonObject;

typedef struct JsonResult
{
    int Success;
    const char* Error;
    int Index;
    int InitialSize;
    int EndSize;
    JsonObject RootObject;
} JsonResult;

JsonResult Json_Parse(char* pJson);

typedef struct JsonProperty
{
    const unsigned char* Position;
    const char* Name;
    JsonObject Value;
} JsonProperty;

JsonProperty Json_IterateProperties(JsonObject oJsonObject);
JsonProperty Json_NextProperty(JsonProperty oJsonProperty);
JsonProperty Json_GetPropertyByName(JsonObject oJsonObject, char* pName);
int Json_GetPropertyCount(JsonObject oJsonObject);

typedef struct JsonElement
{
    const unsigned char* Position;
    unsigned int Index;
    JsonObject Value;
} JsonElement;

JsonElement Json_IterateElements(JsonObject oJsonArray);
JsonElement Json_NextElement(JsonElement oJsonElement);
JsonElement Json_GetElementAtIndex(JsonObject oJsonArray, int nIndex);
int Json_GetElementCount(JsonObject oJsonArray);



/********************************
json writing
*********************************/

char* Json_CreateBuffer();
void Json_ReleaseBuffer(char*);
int Json_AddArray(char** pBuffer);
int Json_AddObject(char** pBuffer);
int Json_AddNull(char** pBuffer);
int Json_AddBool(char** pBuffer, int bValue);
int Json_AddString(char** pBuffer, const char* sValue);
int Json_AddNumber(char** pBuffer, double nValue);

int Json_AddPropertyArray(char** pBuffer, const char* sName);
int Json_AddPropertyObject(char** pBuffer, const char* sName);
int Json_AddPropertyNull(char** pBuffer, const char* sName);
int Json_AddPropertyBool(char** pBuffer, const char* sName, int bValue);
int Json_AddPropertyString(char** pBuffer, const char* sName, const char* sValue);
int Json_AddPropertyNumber(char** pBuffer, const char* sName, double nValue);

int Json_ExitScope(char** pBuffer);
const char* Json_GetError(char* pBuffer);

/********************************
json format
*********************************/
char* Json_Indent(char*);
char* Json_Compress(char* pChar);