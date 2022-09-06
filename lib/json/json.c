/*
 * Copyright 2017-2021 Thingstream AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "json.h"

void json_init_buffer(JsonBuffer* jbuf, char* buf, int len)
{
    jbuf->buffer = buf;
    jbuf->ptr = jbuf->buffer;
    /* reserve one character at the end of buf for 0-termination */
    jbuf->end = jbuf->buffer + len - 1;
    /* the library automatically inserts separators before keys,
     * when needed.
     */
    jbuf->need_key_sep = false;
}

void json_char_append(JsonBuffer* jbuf, char ch)
{
    if (jbuf->ptr < jbuf->end)
        *jbuf->ptr++ = ch;
}

void json_char_append_keysep(JsonBuffer* jbuf, char ch, bool keysep)
{
    if (jbuf->ptr < jbuf->end)
        *jbuf->ptr++ = ch;
    jbuf->need_key_sep = keysep;
}

uint32_t json_terminate(JsonBuffer* jbuf)
{
    uint32_t strlen = jbuf->ptr - jbuf->buffer;
    if (json_full(jbuf))
    {
        *jbuf->end = '\0';
        strlen = jbuf->end - jbuf->buffer;
    }
    else
    {
        json_char_append(jbuf, '\0');
    }
    return strlen;
}

uint32_t json_offset(JsonBuffer* jbuf)
{
    return jbuf->ptr - jbuf->buffer;
}

void json_reset(JsonBuffer* jbuf, uint32_t offset)
{
    jbuf->ptr = jbuf->buffer + offset;
}

void json_chars_append(JsonBuffer* jbuf, const char* str, int len)
{
    if (len < 0)
        len = strlen(str);
    while ((len-- > 0) && (jbuf->ptr < jbuf->end))
        *jbuf->ptr++ = *str++;
}

static void json_quoted_string_len_append(JsonBuffer* jbuf, const char* str, int len)
{
    json_char_append(jbuf, '"');
    while ((len-- > 0) && (jbuf->ptr < jbuf->end))
    {
        char ch = *str++;
        if (ch == '\0')
            break;
        if (((ch == '"') || (ch == '\\')) && (++jbuf->ptr < jbuf->end))
        {
            jbuf->ptr[-1] = '\\';
        }
        else if ((ch == '\n') && (++jbuf->ptr < jbuf->end))
        {
            jbuf->ptr[-1] = '\\';
            ch = 'n';
        }

        if ((ch >= ' ') && (ch <= '~'))
        {
            *jbuf->ptr++ = ch;
        }
        else
        {
            // TODO: not JSON spec compliant
            jbuf->ptr += 3;
            if (jbuf->ptr < jbuf->end)
            {
                jbuf->ptr[-3] = '\\';
                jbuf->ptr[-2] = "0123456789abcdef"[(ch >> 4) & 0xf];
                jbuf->ptr[-1] = "0123456789abcdef"[ch & 0xf];
            }
        }
    }
    json_char_append(jbuf, '"');  // Will record if buffer overran above
}

void json_key_append(JsonBuffer* jbuf, const char* str)
{
    if (jbuf->need_key_sep)
        json_char_append(jbuf, ',');
    jbuf->need_key_sep = true;
    json_quoted_string_len_append(jbuf, str, INT_MAX);
    json_char_append(jbuf, ':');
}

static void json_uint32_append(JsonBuffer* jbuf, uint32_t val)
{
    char buf[12];
    char* end = buf + sprintf(buf, "%u", val);
    json_chars_append(jbuf, buf, end - buf);
}

void json_named_uint32_append(JsonBuffer* jbuf, const char* name, uint32_t val)
{
    json_key_append(jbuf, name);
    json_uint32_append(jbuf, val);
}

void json_named_int32_append(JsonBuffer* jbuf, const char* name, int32_t val)
{
    json_key_append(jbuf, name);
    if (val < 0)
    {
        /* TODO handle 0x80000000 */
        json_char_append(jbuf, '-');
        val = -val;
    }
    json_uint32_append(jbuf, (uint32_t)val);
}

void json_named_string_append(JsonBuffer* jbuf, const char* name, const char* str, int len)
{
    json_key_append(jbuf, name);
    json_quoted_string_len_append(jbuf, str, len);
}

void json_named_cstring_append(JsonBuffer* jbuf, const char* name, const char* str)
{
    json_key_append(jbuf, name);
    json_quoted_string_len_append(jbuf, str, INT_MAX);
}

void json_named_chars_append(JsonBuffer* jbuf, const char* name, const char* str, int len)
{
    json_key_append(jbuf, name);
    json_chars_append(jbuf, str, len);
}

void json_named_bool_append(JsonBuffer* jbuf, const char* name, bool val)
{
    json_key_append(jbuf, name);
    if (val)
        json_chars_append(jbuf, "true", 4);
    else
        json_chars_append(jbuf, "false", 5);
}

void json_named_fixedpoint_append(JsonBuffer *jbuf, const char *name,
                                  int32_t number, uint32_t places)
{
    char buf[40];
    char *cp = buf + sizeof(buf);
    uint32_t val = (uint32_t)number;

    if (number < 0)
        val = (uint32_t)-val;

    do
    {
        *--cp = '0' + (val % 10);
        val /= 10;
    }
    while (--places > 0);
    *--cp = '.';
    do
    {
        *--cp = '0' + (val % 10);
        val /= 10;
    }
    while (val != 0);
    if (number < 0)
        *--cp = '-';

    json_key_append(jbuf, name);
    json_chars_append(jbuf, cp, buf + sizeof(buf) - cp);
}
