
# ZSON

Zero dynamic allocation JSON parser in C
-------------------- 

The purpose of this project is to implement a zero allocation JSON parser. Making it ideal for embedded systems or projects with tight memory requirements. 

Unlike other libraries that claim to do zero allocations, but actually just implement a 'push parser' or a tokenizer (leaving the responsibility of allocation to the caller), this library actually implements a parser that does reinterpret the JSON text into a iterable structure, with native types and decoded data.  

The library avoids heap allocations by parsing the JSON data into the same buffer that held the original JSON text, so it actually reuses the same memory space. This does mean the original data is destroyed, so if you want to keep the original text data, a manual copy of the buffer must be made before passing the buffer into the parser.

Many times, the resulting structure has a smaller size than the JSON text, so if there is need to persist the parsed information for long, a copy to a smaller buffer can be made (releasing the original 'larger' buffer).

# Usage

## Parsing a json object 
```c 
JsonResult oResult = Json_Parse("{\"foo\":\"bar\"}");
if (!oResult.Success)
{
    printf("Parsing failed : %s at %i\n", oResult.Error, oResult.Index);
}
else
{
    JsonProperty oProperty = Json_GetPropertyByName(oResult.RootObject, "foo");
    print("The object has property \"%s\" with value \"%s\"\n", oProperty.Name, oProperty.Value.StringValue)
}
```

## Parsing a json array 
```c 
JsonResult oResult = Json_Parse("[1,2,3]");
if (!oResult.Success)
{
    printf("Parsing failed : %s at %i\n", oResult.Error, oResult.Index);
}
else
{
    JsonElement oElement = Json_GetElementAtIndex(oResult.RootObject ,1);
    print("The array has value \"%s\" at index %i\n", oElement.Index, oElement.Value.DoubleValue)
}
```
## Parsing a json scalar

Although JSON specification does not allow scalar values as root objects, the parser is still able to produce values for them

```c 
JsonResult oResult = Json_Parse("\"this is just a string\"");
if (!oResult.Success)
{
    printf("Parsing failed : %s at %i\n", oResult.Error, oResult.Index);
}
else
{
    print("the root is not an object but a string : \"%s\"\n", oResult.RootObject.StringValue)
}
```

# Documentation

## Parsing api


Element|Description
---|---
`enum JsonType` | An enumerator that qualifies que type of data that a `JsonObject` contains, or `JsonTypeInvalid` if a parsing or enumeration error as occurred 
`struct JsonObject` | A structure that holds a parsed value, indicating it's type or `JsonTypeInvalid` if a parsing or enumeration error as occurred 
`struct JsonResult` | A structure returned by `Json_Parse` function, containing that parsing status, the error in case of failure or the root `JsonObject` in case of success  
`struct JsonProperty` | A structure returned by the object enumeration functions that holds the name of the property and its value as a `JsonObject` 
`struct JsonElement` |  A structure returned by the array enumeration functions that has its value as a `JsonObject`, and an index for the element.
`JsonResult Json_Parse(char* pJson)` | Parses a JSON text into a serialized native structure,reusing the same buffer, this is a destructive operation, if the original data is needed a copy of the text data must be made before calling this function
`JsonProperty Json_IterateProperties(JsonObject oJsonObject)` | Returns the first property of the given `JsonObject`, the given object  must be of type `JsonTypeObject`
`JsonProperty Json_NextProperty(JsonProperty oJsonProperty)` | Returns the property following of the given `JsonProperty`, if the given property was the last one the returned `JsonProperty` will have its properties zeroed and the type of the value will be `JsonTypeInvalid`
`JsonProperty Json_GetPropertyByName(JsonObject oJsonObject, char* pName)` | Iterates the object and retrieves a property of the given `JsonObject` that has the given name, if no such property is found the returned `JsonProperty` will have its properties zeroed and the type of the value will be `JsonTypeInvalid`
`int Json_GetPropertyCount(JsonObject oJsonObject)` | Return the number of properties that the given `JsonObject` has
`JsonElement Json_IterateElements(JsonObject oJsonArray)` | Returns the first value of the given JsonObject, the given object must be of type `JsonTypeArray`
`JsonElement Json_NextElement(JsonElement oJsonElement)` | Returns the value following of the given `JsonElement`, if the given element was the last one the returned `JsonElement` will have its properties zeroed and the type of the value will be `JsonTypeInvalid`
`JsonElement Json_GetElementAtIndex(JsonObject oJsonArray, int nIndex)` | Iterates the array and retrieves a value of the given `JsonObject` at the given property, if the index out of range the `JsonElement` will have its properties zeroed and the type of the value will be `JsonTypeInvalid`
`int Json_GetElementCount(JsonObject oJsonArray)` | Return the number of values that the given `JsonObject` has

### enum `JsonType`
The enumerator is used to reflect the type of data found in the JSON text, a special `JsonTypeInvalid` is included to allow the parsing or enumeration functions to return a failure
```c
enumeration JsonType
{
    JsonTypeInvalid //returned if the parsing or enumeration failed
    JsonTypeNull    //a literal "null" in the JSON
    JsonTypeBool    //a literal "true" or "false" in the JSON, the value will be parsed into JsonObject.BoolValue
    JsonTypeNumber  //an integer or decimal value in the JSON, the value will be parsed into JsonObject.DoubleValue
    JsonTypeString  //a double quote delimited string JSON, the value will be parsed into JsonObject.StringValue
    JsonTypeObject  //a {} delimited object JSON
    JsonTypeArray   //a [] delimited array JSON
}
```

### structure `JsonResult`

A structure that holds a parsed value, indicating it's type or `JsonTypeInvalid` if a parsing or enumeration error as occurred.

if `JsonResult.Success` equals 0  The `Error` element will point to a string containing a description and `Index` will point to the index of the input at which the parsing failed.
if `JsonResult.Success` is any other value the `InitialSize` and `EndSize` will have the start and end buffer sizes, which will give you an idea of the "compression" achieved 
and `EndSize` can be used to allocate a new buffer for long time persistance.
The `RootObject` will have the parsed value, the should be checked against its type before usage.

>After parsing the JSON text, the parser will fill the remaining buffer with `\0`
```c
struct JsonResult
{
    int Success             //0 in case of failure
    //if Success == 0
    char* Error             //a description in case of error, undefined otherwise
    int Index               //the index at which the parser failed, undefined otherwise
    //if Success != 0
    int InitialSize         //the size of the parsed json text
    int EndSize             //the size of buffer used to hold the parsed data ( will be <= than InitialSize )
    JsonObject RootObject   //the object that was parsed out of the JSON text
}
```

### structure `JsonObject`

This struct represents a json values ou json structure, JSON definition supports two types of structure.
An object which is an ordered set of name/value pairs delimited by `{}`, or a ordered list of values delimited by `[]` 
These types are set as  `JsonTypeObject` and `JsonTypeArray` respectively. In such case the `JsonObject` as no value and the iteration methods should be used

For other types, the other properties contain the parsed value, except `JsonTypeNull` that represents a literal null in the JSON, or `JsonTypeInvalid` that is used to represent failure. 

```c
struct JsonObject
{
    char* Position      //used internally, by the iteration functions to locate a referenced object
    JsonType Type       //the type of the JSON value/structure that this object represents 
    char* StringValue   //a null terminated string in case of "JsonTypeString", undefined otherwise
    double DoubleValue  //a decimal numbers in case of "JsonTypeNumber", undefined otherwise
    char BoolValue      //0 or 1 case of "JsonTypeBool", undefined otherwise
}
```

### structure `JsonProperty`

This structure is return by `Json_IterateProperties` and `Json_GetPropertyByName`, it also must be passed to the `Json_NextProperty` for a continuation of the iteration

```c
struct JsonProperty
{
    char* Position      //used internally, by the iteration functions to locate a referenced object
    char* Name          //the null terminated string that holds the name of the property 
    JsonObject Value    //an object that holds that property value
}
```

### structure `JsonElement`

This structure is return by `Json_IterateElements` and `Json_GetElementAtIndex`, it also must be passed to the `Json_NextElement` for a continuation of the iteration

```c
struct JsonElement
{
    char* Position      //used internally, by the iteration functions to locate a referenced object
    int Index           //index of the value in the parent array
    JsonObject Value    //the value at the index value
}
```


### function `Json_Parse`

This function accept a writable null terminated string, containing a valid JSON text and parses 
the text into the same buffer memory space.You must be aware that this is a destructive operation.

The `JsonResult` is returned and used only by this function, to easily allow catch of parsing failures. 

```c
JsonResult Json_Parse(char* pJson);
```

#### Usage
```c
JsonResult oResult = Json_Parse("{\"foo\":\"bar\"}");
if(oResult.Success)
    //do something with oResult.RootObject
```

### functions `Json_IterateProperties` & `Json_NextProperty`

These functions can be used to "explore" JSON object with an unknown structure, by enumerating all of it's properties
Their behavior and result is similar, the only  major difference being that `Json_IterateProperties` receives a `JsonObject` retrieves the first `JsonProperty`.
While `Json_NextProperty` retrieves the next `JsonProperty` following the given one.


#### Usage
```c
JsonProperty oFirstProperty = Json_IterateProperties(oObject);
if(oFirstProperty.Value.Type != JsonTypeInvalid)
    //the object has at least 1 property 

JsonProperty oSecondProperty = Json_NextProperty(oFirstProperty);
if(oSecondProperty.Value.Type != JsonTypeInvalid)
    //the object has at least 2 properties
```
```c
//lets output them all
for (JsonProperty oProperty = Json_IterateProperties(oObject); oProperty.Value.Type != JsonTypeInvalid; oProperty = Json_NextProperty(oProperty))
   print("The object has property named \"%s\" \n", oProperty.Name)
```

### function `Json_GetPropertyByName`

If the JSON data as a known structure you can use the `Json_GetPropertyByName` to access the property value directly.
Be aware that if the property is optional or may not be present, the returned type should be validated to prevent errors 


#### Usage
```c
JsonProperty oUnsureProperty = Json_GetPropertyByName(oObject,"user_name");
if(oUnsureProperty.Value.Type != JsonTypeInvalid)
    //the property is present
```
```c
//get values from a known structure disregarding validations
double nPrice = Json_GetPropertyByName(oObject,"product_price").Value.DoubleValue;
int bActive = Json_GetPropertyByName(oObject,"is_active").Value.BoolValue == 1;
```

### function `Json_GetPropertyCount`

This is just a helper function that returns the count of properties of a `JsonObject`.
Since properties can't be get by index, there is not much use for this functions. but it may help if you need to make allocations proportional do the object "size".

>If the JsonObject passed is not of type `JsonTypeObject` -1 is returned

#### Usage
```c
JsonObject oObject; //gotten from a previous parsing

int nTotalProperties = Json_GetPropertyCount(oObject);

```

### functions `Json_IterateElements` & `Json_NextElement`

These functions are be used to iterate over a JSON array.
Their behavior and result is similar, the difference being that `Json_IterateElements` receives a `JsonObject` retrieves the first `JsonElement`.
While `Json_NextElement` retrieves the next `JsonElement` following the given one.


#### Usage
```c
JsonElement oFirstElement = Json_IterateElements(oObject);
if(oFirstElement.Value.Type == JsonTypeInvalid)
    print("array is empty");
```
```c
//lets output them all
for (JsonElement oElement = Json_IterateElements(oObject); oElement.Value.Type != JsonTypeInvalid; oElement = Json_NextElement(oElement))
    print("The array contains \"%s\" \n", oElement.Value.StringValue)
```


### function `Json_GetElementCount`

This is just a helper function that returns the number of elements in a `JsonObject` array.
>Be aware that this operation is O(n)

#### Usage
```c
int nTotalElements = Json_GetElementCount(oObject);

```

### function `Json_GetElementAtIndex`
Elements in a array items can be accessed in random order by their index. The given index must be in valid range.
To get the total number of items use the `Json_GetElementCount` function
>Be aware that this operation is O(n)

#### Usage
```c
//if iteration of the full array is needed you should use "IterateElements" and "NextElement"
//as this takes O(n) + O(n log n)
int nTotalElements = Json_GetElementCount(oObject);
for(int i = 0 ; i < nTotalElements ; i++)
    Json_GetElementAtIndex(oObject,i).Value.DoubleValue;

```



### Examples
An example file as reference for the below code
```json
{
    "name": "my house",
     "location":{
        "lat":123.123,
        "lon":0.9999,
     },
     "mixed_array":[
        true,
        false,
        123465,
        "hello",
        {
            "child_object":"inside"
        }
     ]
}
```
Parsing the file, trying to validade every step to avoid any run time errors.
```c
#include "json.h"

//load from a file to a "big enough" buffer (this is lazy , bad code, DON'T COPY) 
char pBuffer[1024];
hFile = fopen("sample.json", "r");
fgets(pBuffer, 1024, hFile);

//this first example will show some defensive code and how to process the data with the correct validations to avoid runtime errors
JsonResult oResult = Json_Parse(pBuffer);
if (!oResult.Success)
{
    printf("Parsing failed : %s at %i\n", oResult.Error, oResult.Index);
}
else if(oResult.RootObject.Type == JsonTypeObject)
{
    //get the name property
    JsonProperty oName = Json_GetPropertyByName(oResult.RootObject, "name");
    if(oName.Value.Type == JsonTypeString)
        printf("the name of my object : %s \n", oName.Value.StringValue);

    //get the sub object of the "location" property 
    JsonProperty oLocation = Json_GetPropertyByName(oResult.RootObject, "location");
    if(oLocation.Value.Type == JsonTypeObject)
    {
        //iterate its properties
       JsonObject oCoordinates = oLocation.Value;
       for (JsonProperty oDimension = Json_IterateProperties(oCoordinates); oDimension.Value.Type != JsonTypeInvalid; oDimension = Json_NextProperty(oDimension))
       {
           if(oDimension.Value.Type == JsonTypeNumber)
                print("coordinate \"%s\" is %f \n", oDimension.Name,oDimension.Value.DoubleValue)
            else
                print("coordinate \"%s\" is not of type number\n", oDimension.Name)
       }
    }

    JsonProperty oMixed = Json_GetPropertyByName(oResult.RootObject, "mixed_array");
    if(oMixed.Value.Type == JsonTypeArray)
    {
        //iterate the items of the array
       JsonObject oTheArray = oMixed.Value;
       for (JsonProperty oEntry = Json_IterateElements(oTheArray); oEntry.Value.Type != JsonTypeInvalid; oEntry = Json_NextElement(oEntry))
       {
         switch (oEntry.Type)
            {
                case JsonTypeArray:
                    printf("an array with %i items\n", Json_GetElementCount(oEntry));
                    break;
                case JsonTypeBool:
                    printf("a boolean literal: %s \n", oEntry.BoolValue ? "true" : "false");
                    break;
                case JsonTypeNull:
                    printf("literally just a null");
                    break;
                case JsonTypeNumber:
                    printf("a number: %f\n", oEntry.DoubleValue);
                    break;
                case JsonTypeObject:
                    printf("an inner object with %i properties\n", Json_GetPropertyCount(oEntry));
                    break;
                case JsonTypeString:
                    printf("a string containing: %s\n", oEntry.StringValue);
                    break;
                default:
                case JsonTypeInvalid:
                    printf("but it worked on by machine!!!");
                    break;
            }
       }
    }
}
```
Short and simple code, I wish everything could be this simple, but honestly, this one will probably blow up in your face 
```c
#include "json.h"

//If the previous example seems to have to much code... well...
//If you know the structure of the object and is guarantied that it is properly constructed 
//You can go straight for the prize (at you own risk)

JsonResult oObject = Json_Parse(pBuffer).RootObject;

printf("the name of my object : %s \n", Json_GetPropertyByName(oObject, "name").Value.StringValue);
JsonObject oCoordinates = Json_GetPropertyByName(oObject, "location").Value
print("coordinate \"lat\" is %f \n", Json_GetPropertyByName(oCoordinates, "lat").Value.DoubleValue)
print("coordinate \"lon\" is %f \n", Json_GetPropertyByName(oCoordinates, "lon").Value.DoubleValue)
JsonObject oArray = Json_GetPropertyByName(oObject, "mixed_array").Value
 
printf("a string containing: %s\n", Json_GetElementAtIndex(oObject, 3).Value.StringValue);
printf("a boolean literal: %s \n", Json_GetElementAtIndex(oObject, 1).Value.BoolValue ? "true" : "false");
```
Finally some code to "explore" and print an unknown JSON text
```c

void dump_object(int nLevel, JsonObject oJson)
{
    switch (oJson.Type)
    {
        case JsonTypeArray:
            printf("%*s array(%i)\n", nLevel * 4, "", Json_GetElementCount(oJson));
            for (JsonElement oElement = Json_IterateElements(oJson); oElement.Value.Type != JsonTypeInvalid; oElement = Json_NextElement(oElement))
            {
                printf("%*s element(%i)\n", (nLevel + 1) * 4, "", oElement.Index);
                dump_object(nLevel + 1, oElement.Value);

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
                dump_object(nLevel + 1, oProperty.Value);

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



//call the "dump" function that will print everything it finds 
JsonResult oResult = Json_Parse(pBuffer);
if (oResult.Success && oResult.RootObject.Type != JsonTypeInvalid)
    dumpObject(0, oResult.RootObject);
``` 

## Construction api
Since many times parsing a JSON text also involves creating it, the library also includes functions to write a JSON text.
In this case dynamic memory allocation is unavoidable, bit the allocation and reallocation of the buffer is managed by the library itself.
The only remarkable thing about the way it is done, is that at every step the buffer contains a valid JSON text. This does mean a some extra memory copying is 
done at every interaction. 

> The construction functions receive a pointer to a pointer, because as they reallocate the buffer, the buffer address may change 

Element|Description
---|---
`char* Json_CreateBuffer()` | Allocates a buffer for the construction of the JSON text, it can be used wherever a string can be used. But it can't reallocated/freed by normal functions
`void Json_ReleaseBuffer(char*)` | Releases the memory of a previous allocated buffer to the OS.
`int Json_AddNull(char** pBuffer)` | Write a literal `null` value at the insertion point of the current scope
`int Json_AddBool(char** pBuffer, int bValue)` | Adds a literal `true` or `false` value at the insertion point of the current scope
`int Json_AddString(char** pBuffer, const char* sValue)` | Adds a literal `true` or `false` value at the insertion point of the current scope
`int Json_AddNumber(char** pBuffer, double nValue)`  | Adds a number value at the insertion point of the current scope
`int Json_AddArray(char** pBuffer)` | Opens a array scope at the current insertion point and moves the insertion point into the scope of the array
`int Json_AddObject(char** pBuffer)` | Opens a object scope at the current insertion point and moves the insertion point into the scope of the array
`int Json_AddPropertyNull(char** pBuffer, const char* sName)` | Write property with `null` value at the insertion point of the current object scope
`int Json_AddPropertyBool(char** pBuffer, const char* sName, int bValue)` | Write property with `true` or `false` value at the insertion point of the current object scope
`int Json_AddPropertyString(char** pBuffer, const char* sName, const char* sValue)` | Write property with a string value at the insertion point of the current object scope
`int Json_AddPropertyNumber(char** pBuffer, const char* sName, double nValue)` | Write property with a number value at the insertion point of the current object scope
`int Json_AddPropertyArray(char** pBuffer, const char* sName)` | Write a property with the value of an empty array and moves the insertion point into the scope of that array
`int Json_AddPropertyObject(char** pBuffer, const char* sName)` | Write a property with the value of an empty object and moves the insertion point into the scope of that object
`int Json_ExitScope(char** pBuffer)` | Moves the insertion pointer back to the parent scope
`const char* Json_GetError(char* pBuffer)` | Returns the error message in the case that any of the previous functions have returned `0`
`char* Json_Indent(char*)` | Returns a __allocated__ buffer with a indented version of the passes JSON text.(__the buffer must be de-allocated with free()__)
`char* Json_Compress(char* pChar)` | Removes non significant white-spaces from the JSON text(The modification is done in place)

### Examples
An example file as reference for the below code
```c

char* pBuffer = Json_CreateBuffer();
//the root object will be an array (brackets are used only for indentation)
Json_AddArray(&pBuffer);
{
    Json_AddObject(&pBuffer);//this opens a new scope and moves the insertion point to that scope
    {
        //so now we are adding properties to the previous object
        Json_AddPropertyNull(&pBuffer, "null_property");
        Json_AddPropertyBool(&pBuffer, "bool_property", 1);
        Json_AddPropertyString(&pBuffer, "string_property", "foo\nbar");
        Json_AddPropertyNumber(&pBuffer, "number_property", -111.222);
        Json_AddPropertyObject(&pBuffer, "child_object");
        {
            Json_AddPropertyArray(&pBuffer, "empty_array");
            Json_ExitScope(&pBuffer);//exit the newly created array scope
        }
        Json_ExitScope(&pBuffer);
        Json_AddPropertyString(&pBuffer, "parent_scope", "after the child object");
        
    }
    Json_ExitScope(&pBuffer);
    //back to the root array
    Json_AddNull(&pBuffer);
    Json_AddBool(&pBuffer, 0);
    Json_AddString(&pBuffer, "hello world");
    Json_AddNumber(&pBuffer, 321.0123);
    Json_AddNumber(&pBuffer, .1);
    Json_AddNumber(&pBuffer, -0.5);
}
//the text was printable at any time, but here is the end result
printf("%s", pBuffer);

char* pPretty = Json_Indent(pBuffer);
//and here it is formatted and indented
printf("%s", pBuffer);


free(pPretty)
Json_ReleaseBuffer(pBuffer);


```


# How it works

#### Scoped data
To understand a bit about how this technic works we must realize that JSON has extra information that occupies
space that is not needed for a native manipulation of the data. Take the following string `"foo"` for example.
It takes up 5 bytes with the following information 

```
|0x01|0x02|0x03|0x04|0x05|
|   "|   f|   o|   o|   "|
```

To turn it into a native null terminated string, all we have to do is put a null after the last `o` of the string.
It comes in handy that, there is a byte already there with a `"` that we don't need. so we can just change the memory 
content to be :

```
|0x01|0x02|0x03|0x04|0x05|
|   "|   f|   o|   o|  \0|
```
And returning a pointer to the address `0x02`, will point to a valid null terminated c string.
And since we are at it, there is a extra byte in the beginning of the string with another `"` 
that as no purpose. We can use it to put a specific byte value to "mark" the beginning of a string.
So the actual data in memory will become something like:

```
|0x01|0x02|0x03|0x04|0x05|
| str|   f|   o|   o|  \0|
```
Of course a string may need to be unescaped, but that works in our favor since it means it will occupy even less space.
Here is another example for the string `"foo\nbar"`:

```
        |0x01|0x02|0x03|0x04|0x05|0x06|0x07|0x08|0x09|0x0A|0x0B|0x0C|0x0D|0x0E|0x0F|
original|   "|   f|   o|   o|   \|   n|   b|   a|   r|   "|    |    |    |    |    |
  parsed| str|   f|   o|   o|  \n|   b|   a|   r|  \0|    |    |    |    |    |    |
```

The same happens for a JSON object, it provides us with 2 extra bytes in the form of `{}` 
and the arrays in the form of `[]`

Objects always consist of name/value pairs, so their content can be parsed and iterated em pairs, 
allowing us to not need any type of separator and discard the `:` and `,` characters

And the same thing happens for arrays, until we reach the end of the array, every consecutive value is 
an item of the array:

So an example of the object `{"foo":"bar"}` can be parsed like this:

```
        |0x01|0x02|0x03|0x04|0x05|0x06|0x07|0x08|0x09|0x0A|0x0B|0x0C|0x0D|0x0E|0x0F|
original|   {|   "|   f|   o|   o|   "|   :|   "|   b|   a|   r|   "|   }|    |    |
  parsed| obj| str|   f|   o|   o|  \0| str|   b|   a|   r|  \0| end|    |    |    |
```
While an array like `["foo","bar"]` will be very similar the diference will be only in the type "marker"
and out we end up iterating the data:

```
        |0x01|0x02|0x03|0x04|0x05|0x06|0x07|0x08|0x09|0x0A|0x0B|0x0C|0x0D|0x0E|0x0F|
original|   [|   "|   f|   o|   o|   "|   ,|   "|   b|   a|   r|   "|   ]|    |    |
  parsed| arr| str|   f|   o|   o|  \0| str|   b|   a|   r|  \0| end|    |    |    |
```

#### literals

Having understood the replacements done in memory, it becomes obvious that for the literals `null`, `true` and `false`
is even easier, because they provide us 4 and 5 bytes when all we need is single byte to place a "marker" 
with a byte value that signals to the corresponding type, so here is a simple example os the JSON `[true,false,null]`:

```
        |0x01|0x02|0x03|0x04|0x05|0x06|0x07|0x08|0x09|0x0A|0x0B|0x0C|0x0D|0x0E|0x0F|0x10|0x11|
original|   [|   t|   r|   u|   e|   ,|   f|   a|   l|   s|   e|   ,|   n|   u|   l|   l|   ]|
  parsed| arr| tru| fal| nul| end|    |    |    |    |    |    |    |    |    |    |    |    |
```

#### numbers

Here is where our real problem starts, unlike the scoped data there is not extra bytes for us to put our "marker", 
and also unlike the literals there is not a fixed size used.

There are easy situations like the number `1234` that occupies 4 bytes allowing us to store a 32bit int, 
but that still leaves situations like `9`, where we have a single byte available. If we use it to put our number "marker" 
we will have no space for the data. If we store the data we won't know how to interpret that data without a "marker" signaling the type.

But if we thing about that "marker" byte and it's special values, we can see that up until now 
we have only defined a few unique values (7 to be exact), and to store 8 different values we only need 3 bits.
That leaves us 5 bits of the "marker" byte that we will never use, maybe, we can use those bits to store
actual data.

This puts us on the right track. Not only that, but fiddling with the "marker" bits we can take care of another problem...

Lets take the JSON `["One","Two"]` for example.
If we have a pointer to the start of the array, and we wish to get the item at position `1` (that being the second string).
We need to "skip" the first item, but the only way to go to the end of a null terminated string is to read __all bytes__ until we get a `\0`. 
If we knew that the string occupied 3 bytes, we could actually jump to its end, and fetch the following value. 

So if we are able to store the size of our objects, it will make random access faster. 
Of course JSON allows for arbitrarily big strings, arrays or objects. 
A 32bit int could handle all of that but we have no place to put it so a compromise must be made.
We will store the size of small strings, arrays or object in the available bits of our "marker", 
and have a different value "marker" for the ones that need to be iterated to find the actual size.

This leads us to our final version of our "marker" values, that are actually an encoding of flags and 
data about the values that we have parsed.

```
6bit size [0-63]        0   1   JsonMarkerSmallString
6bit size [0-63]        1   0   JsonMarkerSmallObject
6bit size [0-63]        1   1   JsonMarkerSmallArray
5bit int [0-31]     1   0   0   JsonMarkerDecimal
4bit int [0-15] 1   0   0   0   JsonMarkerDigit
3bit [0-7]  1   0   0   0   0   JsonMarkerInt
1   0   0   0   0   0   0   0   JsonMarkerLargeString
1   0   1   0   0   0   0   0   JsonMarkerLargeObject
1   1   0   0   0   0   0   0   JsonMarkerLargeArray
1   1   1   0   0   0   0   0   JsonMarkerSequenceEnd
0   0   1   0   0   0   0   0   JsonMarkerNull
0   1   0   0   0   0   0   0   JsonMarkerTrue
0   1   1   0   0   0   0   0   JsonMarkerFalse
0   0   0   0   0   0   0   0   unused null

*/
```

If any of the first 2 bits of our "marker" are signaled, we know we are dealing with a scope and we can
use the other 6 bits to hold a value from 0 to 63 indicating the byte size of the scope. 
This allows us to easily skip small strings (and 63 bytes is nothing to be shameful of),
which I bet will be 100% of the cases of the names of the properties of an object.
And other small objects like `{"x":123,"y":321}` that may occur in bulk will also be easily skipped.

If the first 2 bits are 0, then we check the next one. If signaled it means it will be followed by an integer
but the decimal must be shifted n places. The n is stored in the next 5 bits of the maker, allowing us to have up to
15 decimal places. And if you are wondering if the have "space" to store this "marker", just remember that to define
anything with a decimal place in JSON you need to put a `.` so an extra byte will always be available for the decimal marker.

If the 3rd bit is also 0 we check the 4th bit. If is 1, this is the marker for a single digit the next 
4 bits allow us to have values from 0 to 16 which is more that enough to store values from '0' to '9', being these 
the worst case cenarios for numbers.

If the 4th bit is also 0 but the 5th is 1 we have in the remaining 3 bits a enumerator (with room to spare) that tells us the int size that follows 
- 1 = 8bit signed int
- 2 = 16bit signed int
- 3 = 32bit signed int
- 4 = 64bit signed int

And the bytes for that int are guaranteed by the digits placed in the JSON text, and we can check the worst case 
cenarios to be sure.

- `9`     1 byte  - special case in witch we use `JsonMarkerDigit`
- `99`    2 bytes - 1 byte "marker" to signal a 8bit int,  1 byte to store values from  -128 to 127 
- `999`   3 bytes - 1 byte "marker" to signal a 16bit int, 2 bytes to store values from  -32768 to 32767
- `9999`  4 bytes - 1 byte "marker" to signal a 16bit int, 2 bytes to store values from  -32768 to 32767
- `99999` 5 bytes - 1 byte "marker" to signal a 32bit int, 4 bytes to store values from  -2147483648 to 2147483647
- and so on...

And if at any of these we add a `-` minus sign, we get even more space.

Finally the combination of the last 3 bits of our marker, gives us 8 combinations that are just enough to
define the markers we need for our other types of values.

>The all 0 combination was left unused on purpose, so mistakes are not made with the termination of string, or with the passed buffer. A 0 encountered will always mean error.


#### Final thoughts

This technic and encoding was created by myself without any inspiration from other sources. This does not mean
I think it is unique or novel. There are many smart people out there, I wouldn't be surprized if some
other algorithm used a similar encoding technic. Any way, I think that this code can be of use for project that
have very tight memory requirements, as is as a very small memory footprint, while not compromising performance too much. 

The code is provided AS IS, you can use it as you wish. (just don't blame me if you take down the internet)

Happy coding.
