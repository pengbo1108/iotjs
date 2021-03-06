/* Copyright 2015 Samsung Electronics Co., Ltd.
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
#include "iotjs_module_tcp.h"

#include "iotjs_reqwrap.h"
#include "uv.h"

namespace iotjs {


typedef ReqWrap<uv_getaddrinfo_t> GetAddrInfoReqWrap;

#if !defined(__NUTTX__)
static void AfterGetAddrInfo(uv_getaddrinfo_t* req, int status, addrinfo* res) {
  GetAddrInfoReqWrap* req_wrap = reinterpret_cast<GetAddrInfoReqWrap*>(
      req->data);

  iotjs_jargs_t args = iotjs_jargs_create(3);
  iotjs_jargs_append_number(&args, status);

  if (status == 0) {
    char ip[INET6_ADDRSTRLEN];
    const char *addr;

    // Only first address is used
    if (res->ai_family == AF_INET) {
      struct sockaddr_in* sockaddr = reinterpret_cast<struct sockaddr_in*>(
          res->ai_addr);
      addr = reinterpret_cast<char*>(&(sockaddr->sin_addr));
    } else {
      struct sockaddr_in6* sockaddr = reinterpret_cast<struct sockaddr_in6*>(
          res->ai_addr);
      addr = reinterpret_cast<char*>(&(sockaddr->sin6_addr));
    }

    int err = uv_inet_ntop(res->ai_family, addr, ip, INET6_ADDRSTRLEN);
    if (err) {
      ip[0] = 0;
    }

    iotjs_jargs_append_raw_string(&args, ip);
  }

  uv_freeaddrinfo(res);

  // Make the callback into JavaScript
  MakeCallback(req_wrap->jcallback(), JObject::Undefined(), args);

  iotjs_jargs_destroy(&args);

  delete req_wrap;
}
#endif


JHANDLER_FUNCTION(GetAddrInfo) {
  JHANDLER_CHECK(iotjs_jhandler_get_this(jhandler)->IsObject());
  JHANDLER_CHECK(iotjs_jhandler_get_arg_length(jhandler) == 4);
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 0)->IsString());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 1)->IsNumber());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 2)->IsNumber());
  JHANDLER_CHECK(iotjs_jhandler_get_arg(jhandler, 3)->IsFunction());

  iotjs_string_t hostname = iotjs_jhandler_get_arg(jhandler, 0)->GetString();
  int option = iotjs_jhandler_get_arg(jhandler, 1)->GetInt32();
  int flags = iotjs_jhandler_get_arg(jhandler, 2)->GetInt32();
  JObject* jcallback = iotjs_jhandler_get_arg(jhandler, 3);

  int family;
  if (option == 0) {
    family = AF_UNSPEC;
  } else if (option == 4) {
    family = AF_INET;
  } else if (option == 6) {
    family = AF_INET6;
  } else {
    JHANDLER_THROW_RETURN(TypeError, "bad address family");
  }

#if defined(__NUTTX__)
  iotjs_jargs_t args = iotjs_jargs_create(3);
  int err = 0;
  char ip[INET6_ADDRSTRLEN];
  const char* hostname_data = iotjs_string_data(&hostname);

  if (strcmp(hostname_data, "localhost") == 0) {
    strcpy(ip, "127.0.0.1");
  } else {
    sockaddr_in addr;
    int result = inet_pton(AF_INET, hostname_data, &(addr.sin_addr));

    if (result != 1) {
      err = errno;
    } else {
      inet_ntop(AF_INET, &(addr.sin_addr), ip, INET6_ADDRSTRLEN);
    }
  }

  JObject ipobj(ip);
  iotjs_jargs_append_number(&args, err);
  iotjs_jargs_append_obj(&args, &ipobj);
  iotjs_jargs_append_number(&args, family);

  MakeCallback(*jcallback, JObject::Undefined(), args);
#else
  GetAddrInfoReqWrap* req_wrap = new GetAddrInfoReqWrap(*jcallback);

  struct addrinfo hints = {0};
  hints.ai_family = family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = flags;

  int err = uv_getaddrinfo(Environment::GetEnv()->loop(),
                           req_wrap->req(),
                           AfterGetAddrInfo,
                           iotjs_string_data(&hostname),
                           NULL,
                           &hints);

  if (err) {
    delete req_wrap;
  }
#endif

  iotjs_jhandler_return_number(jhandler, err);

  iotjs_string_destroy(&hostname);
}


JObject* InitDns() {
  Module* module = GetBuiltinModule(MODULE_DNS);
  JObject* dns = module->module;

  if (dns == NULL) {
    dns = new JObject();
    dns->SetMethod("getaddrinfo", GetAddrInfo);
    dns->SetProperty("AI_ADDRCONFIG", iotjs_jval_number(AI_ADDRCONFIG));
    dns->SetProperty("AI_V4MAPPED", iotjs_jval_number(AI_V4MAPPED));

    module->module = dns;
  }

  return dns;
}


} // namespace iotjs
