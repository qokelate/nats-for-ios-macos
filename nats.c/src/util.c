// Copyright 2015-2019 The NATS Authors
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "natsp.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "mem.h"

#define ASCII_0 (48)
#define ASCII_9 (57)

static char base32DecodeMap[256];

static const char *base64EncodeURL= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// An implementation of crc16 according to CCITT standards for XMODEM.
static uint16_t crc16tab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

// parseInt64 expects decimal positive numbers. We
// return -1 to signal error
int64_t
nats_ParseInt64(const char *d, int dLen)
{
    int     i;
    char    dec;
    int64_t n = 0;

    if (dLen == 0)
        return -1;

    for (i=0; i<dLen; i++)
    {
        dec = d[i];
        if ((dec < ASCII_0) || (dec > ASCII_9))
            return -1;

        n = (n * 10) + ((int64_t)dec - ASCII_0);
    }

    return n;
}

natsStatus
nats_ParseControl(natsControl *control, const char *line)
{
    natsStatus  s           = NATS_OK;
    char        *tok        = NULL;
    int         len         = 0;

    if ((line == NULL) || (line[0] == '\0'))
        return nats_setDefaultError(NATS_PROTOCOL_ERROR);

    tok = strchr(line, (int) ' ');
    if (tok == NULL)
    {
        control->op = NATS_STRDUP(line);
        if (control->op == NULL)
            return nats_setDefaultError(NATS_NO_MEMORY);

        return NATS_OK;
    }

    len = (int) (tok - line);
    control->op = NATS_MALLOC(len + 1);
    if (control->op == NULL)
    {
        s = nats_setDefaultError(NATS_NO_MEMORY);
    }
    else
    {
        memcpy(control->op, line, len);
        control->op[len] = '\0';
    }

    if (s == NATS_OK)
    {
        // Discard all spaces and the like in between the next token
        while ((tok[0] != '\0')
               && ((tok[0] == ' ')
                   || (tok[0] == '\r')
                   || (tok[0] == '\n')
                   || (tok[0] == '\t')))
        {
            tok++;
        }
    }

    // If there is a token...
    if (tok[0] != '\0')
    {
        char *tmp;

        len = (int) strlen(tok);
        tmp = &(tok[len - 1]);

        // Remove trailing spaces and the like.
        while ((tmp[0] != '\0')
                && ((tmp[0] == ' ')
                    || (tmp[0] == '\r')
                    || (tmp[0] == '\n')
                    || (tmp[0] == '\t')))
        {
            tmp--;
            len--;
        }

        // We are sure that len is > 0 because of the first while() loop.

        control->args = NATS_MALLOC(len + 1);
        if (control->args == NULL)
        {
            s = nats_setDefaultError(NATS_NO_MEMORY);
        }
        else
        {
            memcpy(control->args, tok, len);
            control->args[len] = '\0';
        }
    }

    if (s != NATS_OK)
    {
        NATS_FREE(control->op);
        control->op = NULL;

        NATS_FREE(control->args);
        control->args = NULL;
    }

    return NATS_UPDATE_ERR_STACK(s);
}

natsStatus
nats_CreateStringFromBuffer(char **newStr, natsBuffer *buf)
{
    char    *str = NULL;
    int     len  = 0;

    if ((buf == NULL) || ((len = natsBuf_Len(buf)) == 0))
        return NATS_OK;

    str = NATS_MALLOC(len + 1);
    if (str == NULL)
        return nats_setDefaultError(NATS_NO_MEMORY);

    memcpy(str, natsBuf_Data(buf), len);
    str[len] = '\0';

    *newStr = str;

    return NATS_OK;
}

void
nats_Sleep(int64_t millisec)
{
#ifdef _WIN32
    Sleep((DWORD) millisec);
#else
    usleep(millisec * 1000);
#endif
}

const char*
nats_GetBoolStr(bool value)
{
    if (value)
        return "true";

    return "false";
}

void
nats_NormalizeErr(char *error)
{
    int start = 0;
    int end   = 0;
    int len   = (int) strlen(error);
    int i;

    if (strncmp(error, _ERR_OP_, _ERR_OP_LEN_) == 0)
        start = _ERR_OP_LEN_;

    for (i=start; i<len; i++)
    {
        if ((error[i] != ' ') && (error[i] != '\''))
            break;
    }

    start = i;
    if (start == len)
    {
        error[0] = '\0';
        return;
    }

    for (end=len-1; end>0; end--)
    {
        char c = error[end];
        if ((c == '\r') || (c == '\n') || (c == '\'') || (c == ' '))
            continue;
        break;
    }

    if (end <= start)
    {
        error[0] = '\0';
        return;
    }

    len = end - start + 1;
    memmove(error, error + start, len);
    error[len] = '\0';
}

static natsStatus
_jsonCreateField(nats_JSONField **newField, char *fieldName)
{
    nats_JSONField *field = NULL;

    field = (nats_JSONField*) NATS_CALLOC(1, sizeof(nats_JSONField));
    if (field == NULL)
        return nats_setDefaultError(NATS_NO_MEMORY);

    field->name = fieldName;
    field->typ  = TYPE_NOT_SET;

    *newField = field;

    return NATS_OK;
}

static void
_jsonFreeField(nats_JSONField *field)
{
    if (field->typ == TYPE_ARRAY)
    {
        NATS_FREE(field->value.varr->values);
        NATS_FREE(field->value.varr);
    }
    NATS_FREE(field);
}

static char*
_jsonTrimSpace(char *ptr)
{
    while ((*ptr != '\0')
            && ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\r') || (*ptr == '\n')))
    {
        ptr += 1;
    }
    return ptr;
}

static natsStatus
_jsonGetStr(char **ptr, char **value)
{
    char *p = *ptr;

    while ((*p != '\0') && (*p != '"'))
    {
        if ((*p == '\\') && (*(p + 1) != '\0'))
        {
            p++;
            // based on what http://www.json.org/ says a string should be
            switch (*p)
            {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'n':
                case 'r':
                case 't':
                    break;
                case 'u':
                {
                    int i;

                    // Needs to be 4 hex. A hex is a digit or AF, af
                    p++;
                    for (i=0; i<4; i++)
                    {
                        // digit range
                        if (((*p >= '0') && (*p <= '9'))
                                || ((*p >= 'A') && (*p <= 'F'))
                                || ((*p >= 'a') && (*p <= 'f')))
                        {
                            p++;
                        }
                        else
                        {
                            return nats_setError(NATS_INVALID_ARG, "%s", "error parsing string: invalid unicode character");
                        }
                    }
                    p--;
                    break;
                }
                default:
                    return nats_setError(NATS_INVALID_ARG, "%s", "error parsing string: invalid control character");
            }
        }
        p++;
    }

    if (*p != '\0')
    {
        *value = *ptr;
        *p = '\0';
        *ptr = (char*) (p + 1);
        return NATS_OK;
    }
    return nats_setError(NATS_INVALID_ARG, "%s",
                         "error parsing string: unexpected end of JSON input");
}

static natsStatus
_jsonGetNum(char **ptr, long double *val)
{
    char        *p             = *ptr;
    bool        expIsNegative  = false;
    int64_t     intVal         = 0;
    int64_t     decVal         = 0;
    int64_t     decPower       = 1;
    int64_t     sign           = 1;
    long double ePower         = 1.0;
    long double res            = 0.0;
    int         decPCount      = 0;

    while (isspace(*p))
        p++;

    sign = (*p == '-' ? -1 : 1);

    if ((*p == '-') || (*p == '+'))
        p++;

    while (isdigit(*p))
        intVal = intVal * 10 + (*p++ - '0');

    if (*p == '.')
        p++;

    while (isdigit(*p))
    {
        decVal = decVal * 10 + (*p++ - '0');
        decPower *= 10;
        decPCount++;
    }

    if ((*p == 'e') || (*p == 'E'))
    {
        int64_t eVal = 0;

        p++;

        expIsNegative = (*p == '-' ? true : false);

        if ((*p == '-') || (*p == '+'))
            p++;

        while (isdigit(*p))
            eVal = eVal * 10 + (*p++ - '0');

        if (expIsNegative)
        {
            if (decPower > 0)
                ePower = (long double) decPower;
        }
        else
        {
            if (decPCount > eVal)
            {
                eVal = decPCount - eVal;
                expIsNegative = true;
            }
            else
            {
                eVal -= decPCount;
            }
        }
        while (eVal != 0)
        {
            ePower *= 10;
            eVal--;
        }
    }

    // If we don't end with a ' ', ',' or '}', this is syntax error.
    if ((*p != ' ') && (*p != ',') && (*p != '}'))
        return NATS_ERR;

    if (decVal > 0)
        res = (long double) (sign * (intVal * decPower + decVal));
    else
        res = (long double) (sign * intVal);

    if (ePower > 1)
    {
        if (expIsNegative)
            res /= ePower;
        else
            res *= ePower;
    }
    else if (decVal > 0)
    {
        res /= decPower;
    }
    *ptr = p;
    *val = res;
    return NATS_OK;
}

static natsStatus
_jsonGetBool(char **ptr, bool *val)
{
    if (strncmp(*ptr, "true", 4) == 0)
    {
        *val = true;
        *ptr += 4;
        return NATS_OK;
    }
    else if (strncmp(*ptr, "false", 5) == 0)
    {
        *val = false;
        *ptr += 5;
        return NATS_OK;
    }
    return nats_setError(NATS_INVALID_ARG,
                         "error parsing boolean, got: '%s'", *ptr);
}

static natsStatus
_jsonGetArray(char **ptr, nats_JSONArray **newArray)
{
    natsStatus      s       = NATS_OK;
    char            *p      = *ptr;
    char            *val    = NULL;
    bool            end     = false;
    nats_JSONArray  array;

    // Initialize our stack variable
    memset(&array, 0, sizeof(nats_JSONArray));

    // We support only string array for now
    array.typ     = TYPE_STR;
    array.eltSize = sizeof(char*);
    array.size    = 0;
    array.cap     = 4;
    array.values  = NATS_CALLOC(array.cap, array.eltSize);

    while ((s == NATS_OK) && (*p != '\0'))
    {
        p = _jsonTrimSpace(p);

        // We support only array of strings for now
        if (*p != '"')
        {
            s = nats_setError(NATS_NOT_PERMITTED,
                              "only string arrays supported, got '%s'", p);
            break;
        }

        p += 1;

        s = _jsonGetStr(&p, &val);
        if (s != NATS_OK)
            break;

        if (array.size + 1 > array.cap)
        {
            char **newValues  = NULL;
            int newCap      = 2 * array.cap;

            newValues = (char**) NATS_REALLOC(array.values, newCap * sizeof(char*));
            if (newValues == NULL)
            {
                s = nats_setDefaultError(NATS_NO_MEMORY);
                break;
            }
            array.values = (void**) newValues;
            array.cap    = newCap;
        }
        ((char**)array.values)[array.size++] = val;

        p = _jsonTrimSpace(p);
        if (*p == '\0')
            break;

        if (*p == ']')
        {
            end = true;
            break;
        }
        else if (*p == ',')
        {
            p += 1;
        }
        else
        {
            s = nats_setError(NATS_ERR, "expected ',' got '%s'", p);
        }
    }
    if ((s == NATS_OK) && !end)
    {
        s = nats_setError(NATS_ERR,
                          "unexpected end of array: '%s'",
                          (*p != '\0' ? p : "NULL"));
    }
    if (s == NATS_OK)
    {
        *newArray = NATS_MALLOC(sizeof(nats_JSONArray));
        if (*newArray == NULL)
        {
            s = nats_setDefaultError(NATS_NO_MEMORY);
        }
        else
        {
            memcpy(*newArray, &array, sizeof(nats_JSONArray));
            *ptr = (char*) (p + 1);
        }
    }
    if (s != NATS_OK)
    {
        int i;
        for (i=0; i<array.size; i++)
        {
            p = ((char**)array.values)[i];
            *(p + strlen(p)) = '"';
        }
        NATS_FREE(array.values);
    }

    return NATS_UPDATE_ERR_STACK(s);
}

static char*
_jsonSkipUnknownType(char *ptr)
{
    char    *p = ptr;
    int     skip = 0;
    bool    quoteOpen = false;

    while (*p != '\0')
    {
        if (((*p == ',') || (*p == '}')) && (skip == 0))
            break;
        else if ((*p == '{') || (*p == '['))
            skip++;
        else if ((*p == '}') || (*p == ']'))
            skip--;
        else if ((*p == '"') && (*(p-1) != '\\'))
        {
            if (quoteOpen)
            {
                quoteOpen = false;
                skip--;
            }
            else
            {
                quoteOpen = true;
                skip++;
            }
        }
        p += 1;
    }
    return p;
}

#define JSON_STATE_START        (0)
#define JSON_STATE_NO_FIELD_YET (1)
#define JSON_STATE_FIELD        (2)
#define JSON_STATE_SEPARATOR    (3)
#define JSON_STATE_VALUE        (4)
#define JSON_STATE_NEXT_FIELD   (5)
#define JSON_STATE_END          (6)

natsStatus
nats_JSONParse(nats_JSON **newJSON, const char *jsonStr, int jsonLen)
{
    natsStatus      s         = NATS_OK;
    nats_JSON       *json     = NULL;
    nats_JSONField  *field    = NULL;
    nats_JSONField  *oldField = NULL;
    char            *ptr;
    char            *fieldName = NULL;
    int             state;
    bool            gotEnd    = false;
    char            *copyStr  = NULL;

    if (jsonLen < 0)
    {
        if (jsonStr == NULL)
            return nats_setDefaultError(NATS_INVALID_ARG);

        jsonLen = (int) strlen(jsonStr);
    }

    json = NATS_CALLOC(1, sizeof(nats_JSON));
    if (json == NULL)
        return nats_setDefaultError(NATS_NO_MEMORY);

    s = natsStrHash_Create(&(json->fields), 4);
    if (s == NATS_OK)
    {
        json->str = NATS_MALLOC(jsonLen + 1);
        if (json->str == NULL)
            s = nats_setDefaultError(NATS_NO_MEMORY);

        if (s == NATS_OK)
        {
            memcpy(json->str, jsonStr, jsonLen);
            json->str[jsonLen] = '\0';
        }
    }
    if (s != NATS_OK)
    {
        nats_JSONDestroy(json);
        return NATS_UPDATE_ERR_STACK(s);
    }

    ptr = json->str;
    copyStr = NATS_STRDUP(ptr);
    if (copyStr == NULL)
    {
        nats_JSONDestroy(json);
        return nats_setDefaultError(NATS_NO_MEMORY);
    }
    state = JSON_STATE_START;

    while ((s == NATS_OK) && (*ptr != '\0'))
    {
        ptr = _jsonTrimSpace(ptr);
        if (*ptr == '\0')
            break;
        switch (state)
        {
            case JSON_STATE_START:
            {
                // Should be the start of the JSON string
                if (*ptr != '{')
                {
                    s = nats_setError(NATS_ERR, "incorrect JSON string: '%s'", ptr);
                    break;
                }
                ptr += 1;
                state = JSON_STATE_NO_FIELD_YET;
                break;
            }
            case JSON_STATE_NO_FIELD_YET:
            case JSON_STATE_FIELD:
            {
                // Check for end, which is valid only in state == JSON_STATE_NO_FIELD_YET
                if (*ptr == '}')
                {
                    if (state == JSON_STATE_NO_FIELD_YET)
                    {
                        ptr += 1;
                        state = JSON_STATE_END;
                        break;
                    }
                    s = nats_setError(NATS_ERR,
                                      "expected beginning of field, got: '%s'",
                                      ptr);
                    break;
                }
                // Check for
                // Should be the first quote of a field name
                if (*ptr != '"')
                {
                    s = nats_setError(NATS_ERR, "missing quote: '%s'", ptr);
                    break;
                }
                ptr += 1;
                s = _jsonGetStr(&ptr, &fieldName);
                if (s != NATS_OK)
                {
                    s = nats_setError(NATS_ERR, "invalid field name: '%s'", ptr);
                    break;
                }
                s = _jsonCreateField(&field, fieldName);
                if (s != NATS_OK)
                {
                    NATS_UPDATE_ERR_STACK(s);
                    break;
                }
                s = natsStrHash_Set(json->fields, fieldName, false, (void*) field, (void**)&oldField);
                if (s != NATS_OK)
                {
                    NATS_UPDATE_ERR_STACK(s);
                    break;
                }
                if (oldField != NULL)
                {
                    NATS_FREE(oldField);
                    oldField = NULL;
                }
                state = JSON_STATE_SEPARATOR;
                break;
            }
            case JSON_STATE_SEPARATOR:
            {
                // Should be the separation between field name and value.
                if (*ptr != ':')
                {
                    s = nats_setError(NATS_ERR, "missing value for field '%s': '%s'", fieldName, ptr);
                    break;
                }
                ptr += 1;
                state = JSON_STATE_VALUE;
                break;
            }
            case JSON_STATE_VALUE:
            {
                // Parsing value here. Determine the type based on first character.
                if (*ptr == '"')
                {
                    field->typ = TYPE_STR;
                    ptr += 1;
                    s = _jsonGetStr(&ptr, &field->value.vstr);
                    if (s != NATS_OK)
                        s = nats_setError(NATS_ERR,
                                          "invalid string value for field '%s': '%s'",
                                          fieldName, ptr);
                }
                else if ((*ptr == 't') || (*ptr == 'f'))
                {
                    field->typ = TYPE_BOOL;
                    s = _jsonGetBool(&ptr, &field->value.vbool);
                    if (s != NATS_OK)
                        s = nats_setError(NATS_ERR,
                                          "invalid boolean value for field '%s': '%s'",
                                          fieldName, ptr);
                }
                else if (isdigit(*ptr) || (*ptr == '-'))
                {
                    field->typ = TYPE_NUM;
                    s = _jsonGetNum(&ptr, &field->value.vdec);
                    if (s != NATS_OK)
                        s = nats_setError(NATS_ERR,
                                          "invalid numeric value for field '%s': '%s'",
                                          fieldName, ptr);
                }
                else if ((*ptr == '[') || (*ptr == '{'))
                {
                    bool doSkip = true;

                    if (*ptr == '[')
                    {
                        ptr += 1;
                        s = _jsonGetArray(&ptr, &field->value.varr);
                        if (s == NATS_OK)
                        {
                            field->typ = TYPE_ARRAY;
                            doSkip = false;
                        }
                        else  if (s == NATS_NOT_PERMITTED)
                        {
                            // This is an array but we don't support the
                            // type of elements, so skip.
                            s = NATS_OK;
                            // Clear error stack
                            nats_clearLastError();
                            // Need to go back to the '[' character.
                            ptr -= 1;
                        }
                    }
                    if ((s == NATS_OK) && doSkip)
                    {
                        // Don't support, skip until next field.
                        ptr = _jsonSkipUnknownType(ptr);
                        // Destroy the field that we have created
                        natsStrHash_Remove(json->fields, fieldName);
                        _jsonFreeField(field);
                        field = NULL;
                    }
                }
                else
                {
                    s = nats_setError(NATS_ERR,
                                      "looking for value, got: '%s'", ptr);
                }
                if (s == NATS_OK)
                    state = JSON_STATE_NEXT_FIELD;
                break;
            }
            case JSON_STATE_NEXT_FIELD:
            {
                // We should have a ',' separator or be at the end of the string
                if ((*ptr != ',') && (*ptr != '}'))
                {
                    s =  nats_setError(NATS_ERR, "missing separator: '%s' (%s)", ptr, copyStr);
                    break;
                }
                if (*ptr == ',')
                    state = JSON_STATE_FIELD;
                else
                    state = JSON_STATE_END;
                ptr += 1;
                break;
            }
            case JSON_STATE_END:
            {
                // If we are here it means that there was a character after the '}'
                // so that's considered a failure.
                s = nats_setError(NATS_ERR,
                                  "invalid characters after end of JSON: '%s'",
                                  ptr);
                break;
            }
        }
    }
    if (s == NATS_OK)
    {
        if (state != JSON_STATE_END)
            s = nats_setError(NATS_ERR, "%s", "JSON string not properly closed");
    }
    if (s == NATS_OK)
        *newJSON = json;
    else
        nats_JSONDestroy(json);

    NATS_FREE(copyStr);

    return NATS_UPDATE_ERR_STACK(s);
}

natsStatus
nats_JSONGetValue(nats_JSON *json, const char *fieldName, int fieldType, void **addr)
{
    nats_JSONField *field = NULL;

    field = (nats_JSONField*) natsStrHash_Get(json->fields, (char*) fieldName);
    // If unknown field, just ignore
    if (field == NULL)
        return NATS_OK;

    // Check parsed type matches what is being asked.
    switch (fieldType)
    {
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_ULONG:
        case TYPE_DOUBLE:
            if (field->typ != TYPE_NUM)
                return nats_setError(NATS_INVALID_ARG,
                                     "Asked for field '%s' as type %d, but got type %d when parsing",
                                     field->name, fieldType, field->typ);
            break;
        case TYPE_BOOL:
        case TYPE_STR:
            if (field->typ != fieldType)
                return nats_setError(NATS_INVALID_ARG,
                                     "Asked for field '%s' as type %d, but got type %d when parsing",
                                     field->name, fieldType, field->typ);
            break;
        default:
            return nats_setError(NATS_INVALID_ARG,
                                 "Asked for field '%s' as type %d, but this type does not exist",
                                 field->name, fieldType);
    }
    // We have proper type, return value
    switch (fieldType)
    {
        case TYPE_STR:
        {
            if (field->value.vstr == NULL)
            {
                (*(char**)addr) = NULL;
            }
            else
            {
                char *tmp = NATS_STRDUP(field->value.vstr);
                if (tmp == NULL)
                    return nats_setDefaultError(NATS_NO_MEMORY);
                (*(char**)addr) = tmp;
            }
            break;
        }
        case TYPE_BOOL:     (*(bool*)addr) = field->value.vbool;                break;
        case TYPE_INT:      (*(int*)addr) = (int)field->value.vdec;             break;
        case TYPE_LONG:     (*(int64_t*)addr) = (int64_t) field->value.vdec;    break;
        case TYPE_ULONG:    (*(uint64_t*)addr) = (uint64_t) field->value.vdec;  break;
        case TYPE_DOUBLE:   (*(long double*)addr) = field->value.vdec;          break;
        default:
        {
            return nats_setError(NATS_NOT_FOUND,
                                 "Unknown field type for field '%s': %d",
                                 field->name, fieldType);
        }
    }
    return NATS_OK;
}

natsStatus
nats_JSONGetArrayValue(nats_JSON *json, const char *fieldName, int fieldType, void ***array, int *arraySize)
{
    natsStatus      s        = NATS_OK;
    nats_JSONField  *field   = NULL;
    void            **values = NULL;

    field = (nats_JSONField*) natsStrHash_Get(json->fields, (char*) fieldName);
    // If unknown field, just ignore
    if (field == NULL)
        return NATS_OK;

    // Check parsed type matches what is being asked.
    if (field->typ != TYPE_ARRAY)
        return nats_setError(NATS_INVALID_ARG,
                             "Field '%s' is not an array, it has type: %d",
                             field->name, field->typ);
    if (fieldType != field->value.varr->typ)
        return nats_setError(NATS_INVALID_ARG,
                             "Asked for field '%s' as an array of type: %d, but it is an array of type: %d",
                             field->name, fieldType, field->typ);

    values = NATS_CALLOC(field->value.varr->size, field->value.varr->eltSize);
    if (values == NULL)
        return nats_setDefaultError(NATS_NO_MEMORY);

    if (fieldType == TYPE_STR)
    {
        int i;

        for (i=0; i<field->value.varr->size; i++)
        {
            values[i] = NATS_STRDUP((char*)(field->value.varr->values[i]));
            if (values[i] == NULL)
            {
                s = nats_setDefaultError(NATS_NO_MEMORY);
                break;
            }
        }
        if (s != NATS_OK)
        {
            int j;

            for (j=0; j<i; j++)
                NATS_FREE(values[i]);

            NATS_FREE(values);
        }
    }
    else
    {
        s = nats_setError(NATS_INVALID_ARG, "%s",
                          "Only string arrays are supported");
    }
    if (s == NATS_OK)
    {
        *array     = values;
        *arraySize = field->value.varr->size;
    }

    return NATS_UPDATE_ERR_STACK(s);
}

void
nats_JSONDestroy(nats_JSON *json)
{
    natsStrHashIter iter;
    nats_JSONField  *field;

    if (json == NULL)
        return;

    natsStrHashIter_Init(&iter, json->fields);
    while (natsStrHashIter_Next(&iter, NULL, (void**)&field))
    {
        natsStrHashIter_RemoveCurrent(&iter);
        _jsonFreeField(field);
    }
    natsStrHash_Destroy(json->fields);
    NATS_FREE(json->str);
    NATS_FREE(json);
}

void
nats_Base32_Init(void)
{
    const char  *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int         alphaLen  = (int) strlen(alphabet);
    int         i;

    for (i=0; i<(int)sizeof(base32DecodeMap); i++)
        base32DecodeMap[i] = (char) 0xFF;

    for (i=0; i<alphaLen; i++)
        base32DecodeMap[(int)alphabet[i]] = (char) i;
}

natsStatus
nats_Base32_DecodeString(const char *src, char *dst, int dstMax, int *dstLen)
{
    natsStatus  s         = NATS_OK;
    char        *ptr      = (char*) src;
    int         n         = 0;
    bool        done      = false;
    int         srcLen    = (int) strlen(src);
    int         remaining = srcLen;

    *dstLen = 0;

    while (remaining > 0)
    {
        char dbuf[8];
        int  dLen = 8;
        int  j;
        int  needs;

        for (j=0; j<8; )
        {
            int in;

            if (remaining == 0)
            {
                dLen = j;
                done  = true;
                break;
            }

            in = (int) *ptr;
            ptr++;
            remaining--;

            dbuf[j] = base32DecodeMap[in];
            // If invalid character, report the position but as the number of character
            // since beginning, not array index.
            if (dbuf[j] == (char) 0xFF)
                return nats_setError(NATS_ERR, "base32: invalid data at location %d", srcLen - remaining);
            j++;
        }

        needs = 0;
        switch (dLen)
        {
            case 8: needs = 5; break;
            case 7: needs = 4; break;
            case 5: needs = 3; break;
            case 4: needs = 2; break;
            case 2: needs = 1; break;
        }
        if (n+needs > dstMax)
            return nats_setError(NATS_INSUFFICIENT_BUFFER, "based32: needs %d bytes, max is %d", n+needs, dstMax);

        if (dLen == 8)
            dst[4] = dbuf[6]<<5 | dbuf[7];
        if (dLen >= 7)
            dst[3] = dbuf[4]<<7 | dbuf[5]<<2 | dbuf[6]>>3;
        if (dLen >= 5)
            dst[2] = dbuf[3]<<4 | dbuf[4]>>1;
        if (dLen >= 4)
            dst[1] = dbuf[1]<<6 | dbuf[2]<<1 | dbuf[3]>>4;
        if (dLen >= 2)
            dst[0] = dbuf[0]<<3 | dbuf[1]>>2;

        n += needs;

        if (!done)
            dst += 5;
    }

    *dstLen = n;

    return NATS_OK;
}

natsStatus
nats_Base64RawURL_EncodeString(const unsigned char *src, int srcLen, char **pDest)
{
    char        *dst   = NULL;
    int         dstLen = 0;
    int         n;
    int         di = 0;
    int         si = 0;
    int         remain = 0;
    uint32_t    val = 0;

    *pDest = NULL;

    if ((src == NULL) || (src[0] == '\0'))
        return NATS_OK;

    n = srcLen;
    dstLen = (n * 8 + 5) / 6;
    dst = NATS_CALLOC(1, dstLen + 1);
    if (dst == NULL)
        return nats_setDefaultError(NATS_NO_MEMORY);

    n = ((srcLen / 3) * 3);
    for (si = 0; si < n; )
    {
        // Convert 3x 8bit source bytes into 4 bytes
        val = (uint32_t)(src[si+0])<<16 | (uint32_t)(src[si+1])<<8 | (uint32_t)(src[si+2]);

        dst[di+0] = base64EncodeURL[val >> 18 & 0x3F];
        dst[di+1] = base64EncodeURL[val >> 12 & 0x3F];
        dst[di+2] = base64EncodeURL[val >>  6 & 0x3F];
        dst[di+3] = base64EncodeURL[val       & 0x3F];

        si += 3;
        di += 4;
    }

    remain = srcLen - si;
    if (remain == 0)
    {
        *pDest = dst;
        return NATS_OK;
    }

    // Add the remaining small block
    val = (uint32_t)src[si+0] << 16;
    if (remain == 2)
        val |= (uint32_t)src[si+1] << 8;

    dst[di+0] = base64EncodeURL[val >> 18 & 0x3F];
    dst[di+1] = base64EncodeURL[val >> 12 & 0x3F];

    if (remain == 2)
        dst[di+2] = base64EncodeURL[val >> 6 & 0x3F];

    *pDest = dst;

    return NATS_OK;
}

// Returns the 2-byte crc for the data provided.
uint16_t
nats_CRC16_Compute(unsigned char *data, int len)
{
    uint16_t    crc = 0;
    int         i;

    for (i=0; i<len; i++)
        crc = ((crc << 8) & 0xFFFF) ^ crc16tab[((crc>>8)^(uint16_t)(data[i]))&0x00FF];

    return crc;
}

// Checks the calculated crc16 checksum for data against the expected.
bool
nats_CRC16_Validate(unsigned char *data, int len, uint16_t expected)
{
    uint16_t crc = nats_CRC16_Compute(data, len);
    return crc == expected;
}

natsStatus
nats_ReadFile(natsBuffer **buffer, int initBufSize, const char *fn)
{
    natsStatus  s;
    FILE        *f      = NULL;
    natsBuffer  *buf    = NULL;
    char        *ptr    = NULL;
    int         total   = 0;

    if ((initBufSize <= 0) || nats_IsStringEmpty(fn))
        return nats_setDefaultError(NATS_INVALID_ARG);

    f = fopen(fn, "r");
    if (f == NULL)
        return nats_setError(NATS_ERR, "error opening file '%s': %s", fn, strerror(errno));

    s = natsBuf_Create(&buf, initBufSize);
    if (s == NATS_OK)
        ptr = natsBuf_Data(buf);
    while (s == NATS_OK)
    {
        int r = (int) fread(ptr, 1, (size_t) natsBuf_Available(buf), f);
        if (r == 0)
            break;

        total += r;
        natsBuf_MoveTo(buf, total);
        if (natsBuf_Available(buf) == 0)
            s = natsBuf_Expand(buf, natsBuf_Capacity(buf)*2);
        if (s == NATS_OK)
            ptr = natsBuf_Data(buf) + total;
    }

    // Close file. If there was an error, do not report possible closing error
    // as the actual error
    if (s != NATS_OK)
        fclose(f);
    else if (fclose(f) != 0)
        s = nats_setError(NATS_ERR, "error closing file '%s': '%s", fn, strerror(errno));

    if (s == NATS_OK)
    {
        natsBuf_AppendByte(buf, '\0');
        *buffer = buf;
    }
    else if (buf != NULL)
    {
        memset(natsBuf_Data(buf), 0, natsBuf_Capacity(buf));
        natsBuf_Destroy(buf);
    }

    return NATS_UPDATE_ERR_STACK(s);
}

bool
nats_HostIsIP(const char *host)
{
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    bool            isIP = true;

    memset(&hint, '\0', sizeof hint);

    hint.ai_family = PF_UNSPEC;
    hint.ai_flags = AI_NUMERICHOST;

    if (getaddrinfo(host, NULL, &hint, &res) != 0)
        isIP = false;

    freeaddrinfo(res);

    return isIP;
}

static bool
_isLineAnHeader(const char *ptr)
{
    char    *last   = NULL;
    int     len     = 0;
    int     count   = 0;
    bool    done    = false;

    // We are looking for a header. Based on the Go client's regex,
    // the strict requirement is that it ends with at least 3 consecutive
    // `-` characters. It must also have 3 consecutive `-` before that.
    // So the minimum size would be 6.
    len = (int) strlen(ptr);
    if (len < 6)
        return false;

    // First make sure that we have at least 3 `-` at the end.
    last = (char*) (ptr + len - 1);

    while ((*last == '-') && (last != ptr))
    {
        count++;
        last--;
        if (count == 3)
            break;
    }
    if (count != 3)
        return false;

    // Now from that point and going backward, we consider
    // to have proper header if we find again 3 consecutive
    // dashes.
    count = 0;
    while (!done)
    {
        if (*last == '-')
        {
            // We have at least `---`, we are done.
            if (++count == 3)
                return true;
        }
        else
        {
            // Reset.. we need 3 consecutive dashes
            count = 0;
        }
        if (last == ptr)
            done = true;
        else
            last--;
    }
    // If we are here, it means we did not find `---`
    return false;
}

natsStatus
nats_GetJWTOrSeed(char **val, const char *content, int item)
{
    natsStatus  s       = NATS_OK;
    char        *pch    = NULL;
    char        *str    = NULL;
    char        *saved  = NULL;
    int         curItem = 0;
    int         orgLen  = 0;
    char        *nt     = NULL;

    // First, make a copy of the original content since
    // we are going to call strtok on it, which alters it.
    str = NATS_STRDUP(content);
    if (str == NULL)
        return nats_setDefaultError(NATS_NO_MEMORY);

    orgLen = (int) strlen(str);

    pch = nats_strtok(str, "\n", &nt);
    while (pch != NULL)
    {
        if (_isLineAnHeader(pch))
        {
            // We got the start of the section. Save the next line
            // as the possible returned value if the following line
            // is a header too.
            pch = nats_strtok(NULL, "\n", &nt);
            saved = pch;

            while (pch != NULL)
            {
                pch = nats_strtok(NULL, "\n", &nt);
                if (pch == NULL)
                    break;

                // We tolerate empty string(s).
                if (*pch == '\0')
                    continue;

                break;
            }
            if (pch == NULL)
                break;

            if (_isLineAnHeader(pch))
            {
                // Is this the item we were looking for?
                if (curItem == item)
                {
                    // Return a copy of the saved line
                    *val = NATS_STRDUP(saved);
                    if (*val == NULL)
                        s = nats_setDefaultError(NATS_NO_MEMORY);

                    break;
                }
                else if (++curItem > 1)
                {
                    break;
                }
            }
        }
        pch = nats_strtok(NULL, "\n", &nt);
    }

    memset(str, 0, orgLen);
    NATS_FREE(str);

    // Nothing was found, return NATS_NOT_FOUND but don't set the stack error.
    if ((s == NATS_OK) && (*val == NULL))
        return NATS_NOT_FOUND;

    return NATS_UPDATE_ERR_STACK(s);
}
