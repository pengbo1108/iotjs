/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "iotjs_def.h"
#include "iotjs_module_buffer.h"

#include <stdlib.h>
#include <string.h>


namespace iotjs {


BufferWrap::BufferWrap(JObject& jbuiltin,
                       size_t length)
    : JObjectWrap(jbuiltin)
    , _buffer(NULL)
    , _length(length) {
  if (length > 0) {
    _buffer = iotjs_buffer_allocate(length);
    IOTJS_ASSERT(_buffer != NULL);
  }
}


BufferWrap::~BufferWrap() {
  if (_buffer != NULL) {
    iotjs_buffer_release(_buffer);
  }
}


BufferWrap* BufferWrap::FromJBufferBuiltin(JObject& jbuiltin) {
  IOTJS_ASSERT(jbuiltin.IsObject());
  BufferWrap* buffer = reinterpret_cast<BufferWrap*>(jbuiltin.GetNative());
  IOTJS_ASSERT(buffer != NULL);
  return buffer;
}


BufferWrap* BufferWrap::FromJBuffer(JObject& jbuffer) {
  IOTJS_ASSERT(jbuffer.IsObject());
  JObject jbuiltin(jbuffer.GetProperty("_builtin"));
  return FromJBufferBuiltin(jbuiltin);
}



JObject BufferWrap::jbuiltin() {
  return jobject();
}


JObject BufferWrap::jbuffer() {
  return jbuiltin().GetProperty("_buffer");
}


char* BufferWrap::buffer() {
  return _buffer;
}


size_t BufferWrap::length() {
#ifndef NDEBUG
  int length = jbuffer().GetProperty("length").GetInt32();
  IOTJS_ASSERT(static_cast<size_t>(length) == _length);
#endif
  return _length;
}


static size_t BoundRange(int index, size_t low, size_t upper) {
  if (index < static_cast<int>(low)) {
    return low;
  }
  if (index > static_cast<int>(upper)) {
    return upper;
  }
  return index;
}


int BufferWrap::Compare(const BufferWrap& other) const {
  size_t i = 0;
  size_t j = 0;
  while (i < _length && j < other._length) {
    if (_buffer[i] < other._buffer[j]) {
      return -1;
    } else if (_buffer[i] > other._buffer[j]) {
      return 1;
    }
    ++i;
    ++j;
  }
  if (j < other._length) {
    return -1;
  } else if (i < _length) {
    return 1;
  }
  return 0;
}


size_t BufferWrap::Copy(const char* src, size_t len) {
  return Copy(src, 0, len, 0);
}


size_t BufferWrap::Copy(const char* src,
                        size_t src_from,
                        size_t src_to,
                        size_t dst_from) {
  size_t copied = 0;
  size_t dst_length = _length;
  for (size_t i = src_from, j = dst_from;
       i < src_to && j < dst_length;
       ++i, ++j) {
    *(_buffer + j) = *(src + i);
    ++copied;
  }
  return copied;
}


JObject CreateBuffer(size_t len) {
  JObject jglobal(JObject::Global());
  IOTJS_ASSERT(jglobal.IsObject());

  JObject jBuffer(jglobal.GetProperty("Buffer"));
  IOTJS_ASSERT(jBuffer.IsFunction());

  iotjs_jargs_t jargs = iotjs_jargs_create(1);
  iotjs_jargs_append_number(&jargs, len);
  JResult jres(jBuffer.Call(JObject::Undefined(), jargs));
  IOTJS_ASSERT(jres.IsOk());
  IOTJS_ASSERT(jres.value().IsObject());
  iotjs_jargs_destroy(&jargs);

  return jres.value();
}



JHANDLER_FUNCTION(Buffer) {
  JHANDLER_CHECK(iotjs_jhandler_get_this(jhandler)->IsObject());
  JHANDLER_CHECK(iotjs_jhandler_get_arg_length(jhandler) == 2);
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 0)->IsObject());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 1)->IsNumber());

  int length = iotjs_jhandler_get_arg(jhandler, 1)->GetInt32();
  JObject* jbuffer = iotjs_jhandler_get_arg(jhandler, 0);
  JObject* jbuiltin = iotjs_jhandler_get_this(jhandler);

  jbuiltin->SetProperty("_buffer", *jbuffer);

  BufferWrap* buffer_wrap = new BufferWrap(*jbuiltin, length);
  IOTJS_ASSERT(buffer_wrap == (BufferWrap*)(jbuiltin->GetNative()));
  IOTJS_ASSERT(length == 0 || buffer_wrap->buffer() != NULL);
}


JHANDLER_FUNCTION(Compare) {
  JHANDLER_CHECK(iotjs_jhandler_get_this(jhandler)->IsObject());
  JHANDLER_CHECK(iotjs_jhandler_get_arg_length(jhandler) == 1);
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 0)->IsObject());

  JObject* jsrc_builtin = iotjs_jhandler_get_this(jhandler);
  BufferWrap* src_buffer_wrap = BufferWrap::FromJBufferBuiltin(*jsrc_builtin);

  JObject* jdst_buffer = iotjs_jhandler_get_arg(jhandler, 0);
  BufferWrap* dst_buffer_wrap = BufferWrap::FromJBuffer(*jdst_buffer);

  int compare = src_buffer_wrap->Compare(*dst_buffer_wrap);
  iotjs_jhandler_return_number(jhandler, compare);
}


JHANDLER_FUNCTION(Copy) {
  JHANDLER_CHECK(iotjs_jhandler_get_this(jhandler)->IsObject());
  JHANDLER_CHECK(iotjs_jhandler_get_arg_length(jhandler) == 4);
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 0)->IsObject());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 1)->IsNumber());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 2)->IsNumber());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 3)->IsNumber());

  JObject* jsrc_builtin = iotjs_jhandler_get_this(jhandler);
  BufferWrap* src_buffer_wrap = BufferWrap::FromJBufferBuiltin(*jsrc_builtin);

  JObject* jdst_buffer = iotjs_jhandler_get_arg(jhandler, 0);
  BufferWrap* dst_buffer_wrap = BufferWrap::FromJBuffer(*jdst_buffer);

  int dst_start = iotjs_jhandler_get_arg(jhandler, 1)->GetInt32();
  int src_start = iotjs_jhandler_get_arg(jhandler, 2)->GetInt32();
  int src_end = iotjs_jhandler_get_arg(jhandler, 3)->GetInt32();

  dst_start = BoundRange(dst_start, 0, dst_buffer_wrap->length());
  src_start = BoundRange(src_start, 0, src_buffer_wrap->length());
  src_end = BoundRange(src_end, 0, src_buffer_wrap->length());

  if (src_end < src_start) {
    src_end = src_start;
  }

  int copied = dst_buffer_wrap->Copy(src_buffer_wrap->buffer(),
                                     src_start,
                                     src_end,
                                     dst_start);

  iotjs_jhandler_return_number(jhandler, copied);
}


JHANDLER_FUNCTION(Write) {
  JHANDLER_CHECK(iotjs_jhandler_get_arg_length(jhandler) == 3);
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 0)->IsString());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 1)->IsNumber());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 2)->IsNumber());

  iotjs_string_t src = iotjs_jhandler_get_arg(jhandler, 0)->GetString();
  int offset = iotjs_jhandler_get_arg(jhandler, 1)->GetInt32();
  int length = iotjs_jhandler_get_arg(jhandler, 2)->GetInt32();

  JObject* jbuiltin = iotjs_jhandler_get_this(jhandler);

  BufferWrap* buffer_wrap = BufferWrap::FromJBufferBuiltin(*jbuiltin);

  offset = BoundRange(offset, 0, buffer_wrap->length());
  length = BoundRange(length, 0, buffer_wrap->length() - offset);
  length = BoundRange(length, 0, iotjs_string_size(&src));

  size_t copied = buffer_wrap->Copy(iotjs_string_data(&src), 0, length, offset);

  iotjs_jhandler_return_number(jhandler, copied);

  iotjs_string_destroy(&src);
}


JHANDLER_FUNCTION(Slice) {
  JHANDLER_CHECK(iotjs_jhandler_get_arg_length(jhandler) == 2);
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 0)->IsNumber());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 1)->IsNumber());

  JObject* jbuiltin = iotjs_jhandler_get_this(jhandler);
  BufferWrap* buffer_wrap = BufferWrap::FromJBufferBuiltin(*jbuiltin);

  int start = iotjs_jhandler_get_arg(jhandler, 0)->GetInt32();
  int end = iotjs_jhandler_get_arg(jhandler, 1)->GetInt32();

  if (start < 0) {
    start += buffer_wrap->length();
  }
  start = BoundRange(start, 0, buffer_wrap->length());

  if (end < 0) {
    end += buffer_wrap->length();
  }
  end = BoundRange(end, 0, buffer_wrap->length());

  if (end < start) {
    end = start;
  }

  int length = end - start;
  IOTJS_ASSERT(length >= 0);

  JObject jnew_buffer = CreateBuffer(length);
  BufferWrap* new_buffer_wrap = BufferWrap::FromJBuffer(jnew_buffer);
  new_buffer_wrap->Copy(buffer_wrap->buffer(), start, end, 0);

  iotjs_jhandler_return_obj(jhandler, &jnew_buffer);
}


JHANDLER_FUNCTION(ToString) {
  JHANDLER_CHECK(iotjs_jhandler_get_this(jhandler)->IsObject());
  JHANDLER_CHECK(iotjs_jhandler_get_arg_length(jhandler) == 2);
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 0)->IsNumber());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 1)->IsNumber());

  JObject* jbuiltin = iotjs_jhandler_get_this(jhandler);
  BufferWrap* buffer_wrap = BufferWrap::FromJBufferBuiltin(*jbuiltin);

  int start = iotjs_jhandler_get_arg(jhandler, 0)->GetInt32();
  int end = iotjs_jhandler_get_arg(jhandler, 1)->GetInt32();

  start = BoundRange(start, 0, buffer_wrap->length());
  end = BoundRange(end, 0, buffer_wrap->length());

  if (end < start) {
    end = start;
  }

  int length = end - start;
  IOTJS_ASSERT(length >= 0);

  const char* data = buffer_wrap->buffer() + start;
  length = strnlen(data, length);
  iotjs_string_t str = iotjs_string_create_with_size(data, length);

  iotjs_jhandler_return_string(jhandler, &str);

  iotjs_string_destroy(&str);
}


JObject* InitBuffer() {
  Module* module = GetBuiltinModule(MODULE_BUFFER);
  JObject* buffer = module->module;

  if (buffer == NULL) {
    buffer = new JObject(Buffer);

    JObject prototype;
    buffer->SetProperty("prototype", prototype);

    prototype.SetMethod("compare", Compare);
    prototype.SetMethod("copy", Copy);
    prototype.SetMethod("write", Write);
    prototype.SetMethod("slice", Slice);
    prototype.SetMethod("toString", ToString);

    module->module = buffer;
  }

  return buffer;
}


} // namespace iotjs
