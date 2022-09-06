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

#ifndef INC_JSON_H_
#define INC_JSON_H_

#if defined(__cplusplus)
extern "C" {
#elif 0
}
#endif

typedef struct JsonBuffer_s {
    char* buffer;
    char* ptr;
    char* end;
    bool need_key_sep;
} JsonBuffer;

void json_init_buffer(JsonBuffer* jbuf, char* buf, int len);
void json_reset(JsonBuffer* jbuf, uint32_t offset);
uint32_t json_offset(JsonBuffer* jbuf);
uint32_t json_terminate(JsonBuffer* jbuf);

void json_char_append(JsonBuffer* jbuf, const char ch);

/* append a char and update the state of the need_key_sep flag */
void json_char_append_keysep(JsonBuffer* jbuf, const char ch, bool sep);

/* len can be -1 to use strlen() */
void json_chars_append(JsonBuffer* jbuf, const char* str, int len);

/* prepends a separator if the flag has been set */
void json_key_append(JsonBuffer* jbuf, const char* str);
void json_named_string_append(JsonBuffer* jbuf, const char* name, const char* str, int len);
void json_named_cstring_append(JsonBuffer* jbuf, const char* name, const char* str);
void json_named_chars_append(JsonBuffer* jbuf, const char* name, const char* str, int len);
void json_named_bool_append(JsonBuffer* jbuf, const char* name, bool val);
void json_named_int32_append(JsonBuffer* jbuf, const char* name, int32_t val);
void json_named_uint32_append(JsonBuffer* jbuf, const char* name, uint32_t val);

/* append a fixed-point number, for example
 *    153 with places=1  => "15.3"
 *    -42 with places=4  => "-0.0042"
 * places must be between 1 and 15 - values outside this range
 * will scribble on the stack.
 */
void json_named_fixedpoint_append(JsonBuffer *jbuf, const char *name,
                                  int32_t val, uint32_t places);

#define json_start_array(jbuf)  json_char_append(jbuf, '[')
#define json_end_array(jbuf)    json_char_append_keysep(jbuf, ']', true)
#define json_start_object(jbuf) json_char_append_keysep(jbuf, '{', false)
#define json_end_object(jbuf)   json_char_append_keysep(jbuf, '}', true)
#define json_separator(jbuf)    json_char_append_keysep(jbuf, ',', false)

#define json_full(jbuf) ((jbuf)->ptr >= (jbuf)->end)


#if defined(__cplusplus)
}
#endif

#endif /* INC_JSON_H_ */
