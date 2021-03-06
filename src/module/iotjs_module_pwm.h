/* Copyright 2016 Samsung Electronics Co., Ltd.
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


#ifndef IOTJS_MODULE_PWM_H
#define IOTJS_MODULE_PWM_H

#include "iotjs_def.h"
#include "iotjs_objectwrap.h"
#include "iotjs_reqwrap.h"


namespace iotjs {

struct PwmReqData {
  void* data; // pointer to PwmReqWrap
};

typedef ReqWrap<PwmReqData> PwmReqWrap;


// This Pwm class provides interfaces for PWM operation.
class Pwm : public JObjectWrap {
 public:
  explicit Pwm(JObject& jpwm);
  virtual ~Pwm();

  static Pwm* Create(JObject& jpwm);
  static Pwm* GetInstance();
  static JObject* GetJPwm();

  virtual int Export(PwmReqWrap* pwm_req) = 0;
  virtual int SetDutyCycle(PwmReqWrap* pwm_req) = 0;
  virtual int SetPeriod(PwmReqWrap* pwm_req) = 0;
  virtual int Enable(PwmReqWrap* pwm_req) = 0;
  virtual int Unexport(PwmReqWrap* pwm_rea) = 0;
};


JObject* InitPwm();


} // namespace iotjs


#endif /* IOTJS_MODULE_PWM_H */
