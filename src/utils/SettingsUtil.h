/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

/*
Since version 2.3, settings are serialized in a structure of the following form:

# comment linking to more information about the following values
SettingName = string value (might also be a number, etc.)
SubSettings [
    Boolean = true
    Rectangle = 0 0 40 50
    Escaped = $ (leading space), $r$n (newline), $$ (escape character) and trailing space: $
    ValueArray [
        [
            ItemNo = 1
        ]
        [
            ItemNo = 2
        ]
    ]
]

See SquareTreeParser.cpp for further details on variations allowed during
the deserialization of such a settings file.
*/

enum SettingType {
    Type_Struct, Type_Array, Type_Compact,
    Type_Bool, Type_Color, Type_Float, Type_Int, Type_String, Type_Utf8String,
    Type_ColorArray, Type_FloatArray, Type_IntArray, Type_StringArray,
    Type_Comment,
    Type_Prerelease, // same as Type_Struct but won't be written out in release builds
};

struct FieldInfo {
    size_t offset; // offset of the field in the struct
    SettingType type;
    intptr_t value; // default value for primitive types and pointer to StructInfo for complex ones
};

struct StructInfo {
    uint16_t structSize;
    uint16_t fieldCount;
    const FieldInfo *fields;
    // one string of fieldCount zero-terminated names of all fields in the order of fields
    const char *fieldNames;
};

char *SerializeStruct(const StructInfo *info, const void *strct, const char *prevData=nullptr, size_t *sizeOut=nullptr);
void *DeserializeStruct(const StructInfo *info, const char *data, void *strct=nullptr);
void FreeStruct(const StructInfo *info, void *strct);
