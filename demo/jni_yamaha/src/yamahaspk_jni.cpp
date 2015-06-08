/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include "jni_defines.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "yamahaspk.h"


jmethodID TimerFunc;
jobject  Spk;
JNIEnv* Jenv;

/*
void  Yamahaspk_timer_func(int timer_count)
{	
	Jenv->CallVoidMethod(Spk, TimerFunc, timer_count);		
	return;
}
*/

static void Yamahaspk_open(JNIEnv* env, jclass clazz) 
{
//	Jenv  = env;
	yamahaspk_open();
	return ;
}

static void Yamahaspk_close(JNIEnv* env, jobject thiz) {
	yamahaspk_close();
	return;
}

static void Yamahaspk_resetLed(JNIEnv* env, jobject thiz) {
	yamahaspk_reset_led();
	return;
}

static void Yamahaspk_startLed(JNIEnv* env, jobject thiz) {
	yamahaspk_start_led();
	return ;
}

static void Yamahaspk_stopLed(JNIEnv* env, jobject thiz) {
	yamahaspk_stop_led();
	return;
}

static void Yamahaspk_switchAudioJackets(JNIEnv* env, jobject thiz, jint route) {
	yamahaspk_switch_audio(route);
	return;
}


static void Yamahaspk_setTimer(JNIEnv* env, jobject thiz) {
	yamahaspk_start_timer();
	return;
}


static JNINativeMethod gsMethods[] = { 
	{ "init", "()V", (void*)Yamahaspk_open },
        { "release", "()V", (void*)Yamahaspk_close },
        { "resetLed", "()V", (void*)Yamahaspk_resetLed },
	{ "startLed", "()V", (void*)Yamahaspk_startLed },
	{ "startTimer", "()V", (void*)Yamahaspk_setTimer }, 
	{ "stopLed", "()V", (void*)Yamahaspk_stopLed },
        { "switchAudioJackets", "(I)V", (void*)Yamahaspk_switchAudioJackets } };

static JNINativeMethod gsMethods1[] = { 
        { "init", "()V", (void*)Yamahaspk_open },
        { "release", "()V", (void*)Yamahaspk_close },
        { "resetLed", "()V", (void*)Yamahaspk_resetLed },
        { "startLed", "()V", (void*)Yamahaspk_startLed },
        { "startTimer", "()V", (void*)Yamahaspk_setTimer },  
        { "stopLed", "()V", (void*)Yamahaspk_stopLed },
/*        { "switchAudioJackets", "(I)V", (void*)Yamahaspk_switchAudioJackets }*/ };

static int registerNativeMethods(JNIEnv* env, const char* className,
        JNINativeMethod* gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static const char *javaClassPathName = "com/raibow/yamahaspk/app/Main";
static const char *javaClassPathName1 = "com/raibow/yamahaspk/app/Demo1";

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Error: GetEnv failed in JNI_OnLoad");
        return -1;
    }
    if (!registerNativeMethods(env, javaClassPathName, gsMethods,
            sizeof(gsMethods) / sizeof(gsMethods[0]))) {
        LOGE("Error: could not register native methods for yamahaspk Demo0");
        return -1;
    }

   if (!registerNativeMethods(env, javaClassPathName1, gsMethods1,
		sizeof(gsMethods1) / sizeof(gsMethods1[0]))) {
	LOGE("Error: could not register native methods for yamahaspk Demo1");
	return -1;
   }

/*
    jclass clazz = env->FindClass("com/raibow/yamahaspk/app/Main");
    TimerFunc = env->GetMethodID(clazz, "sevenSecTimerFunc", "(I)V");
    if(TimerFunc == NULL)
	LOGE("Unable to find sevenSecTimerFunc function in class");
*/

    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
