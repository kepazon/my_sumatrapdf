// DON'T EDIT MANUALLY !!!!
// auto-generated by gen_txt.py !!!!

#include "BaseUtil.h"
#include "SerializeTxt.h"
#include "PagesLayoutDef.h"

using namespace sertxt;

#define of offsetof
const FieldMetadata gPagesLayoutDefFieldMetadata[] = {
    { of(PagesLayoutDef, name),    TYPE_STR, 0         },
    { of(PagesLayoutDef, page1),   TYPE_STR, 0         },
    { of(PagesLayoutDef, page2),   TYPE_STR, 0         },
    { of(PagesLayoutDef, spaceDx), TYPE_I32, (uintptr_t)4 },
};

const StructMetadata gPagesLayoutDefMetadata = {
    sizeof(PagesLayoutDef),
    4,
    "name\0page1\0page2\0space_dx\0\0",
    &gPagesLayoutDefFieldMetadata[0]
};

#undef of

