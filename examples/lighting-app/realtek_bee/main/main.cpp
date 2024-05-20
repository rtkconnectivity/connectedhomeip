/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <system/SystemError.h>
#include <zephyr/logging/log.h>
#include "AppTask.h"

#ifdef CONFIG_CHIP_PW_RPC
#include "Rpc.h"
#endif

LOG_MODULE_REGISTER(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;

#include <openthread/instance.h>
#include <openthread/platform/time.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread-system.h>
//#include "rtl_wdt.h"
#include "soc.h"
#include "mac_driver.h"

extern "C"
{
// replace misc.c
//extern void WDG_SystemReset(WDTMode_TypeDef wdt_mode, int reset_reason);
void __wrap_otPlatReset(otInstance *aInstance)
{
 	ARG_UNUSED(aInstance);
	//WDG_SystemReset(RESET_ALL, 0xff);
}

bool milli_fired = false;
void milli_handler(void)
{
    milli_fired = true;
    otSysEventSignalPending();
}

bool micro_fired = false;
void micro_handler(void)
{
    micro_fired = true;
    otSysEventSignalPending();
}

typedef void (*bt_timer_handler_t)(void);
extern void mac_RegisterBtTimerHandler(uint32_t tid, bt_timer_handler_t handler);
void __wrap_platformAlarmInit(void)
{
    mac_RegisterBtTimerHandler(MAC_BT_TIMER0, milli_handler);
    mac_RegisterBtTimerHandler(MAC_BT_TIMER1, micro_handler);
}

void __wrap_platformAlarmProcess(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
	if (micro_fired) {
		micro_fired = false;
		otPlatAlarmMicroFired(aInstance);
	}
#endif
	if (milli_fired) {
		milli_fired = false;
        otPlatAlarmMilliFired(aInstance);
	}
}

uint32_t __wrap_otPlatAlarmMicroGetNow(void)
{
    return (uint32_t)otPlatTimeGet();
}

void __wrap_otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
    uint64_t now = otPlatTimeGet();
    uint64_t target_us = mac_ConvertT0AndDtTo64BitTime(t0, dt, &now);
    if (target_us > now)
    {
        target_us = target_us % MAX_BT_CLOCK_COUNTER;
        mac_SetBTClkUSInt(MAC_BT_TIMER1, target_us);
    }
    else
    {
        otPlatAlarmMicroFired(aInstance);
    }
}

void __wrap_otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

uint32_t __wrap_otPlatAlarmMilliGetNow(void)
{
    return (uint32_t)(otPlatTimeGet() / 1000);
}

void __wrap_otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
    uint64_t now = otPlatTimeGet();
    uint64_t target_us;
    uint64_t now_ms = now/1000;
    uint64_t target_ms = mac_ConvertT0AndDtTo64BitTime(t0, dt, &now_ms);
    if (target_ms > now_ms)
    {
        target_us = now + (target_ms - now_ms)*1000;
        target_us = target_us % MAX_BT_CLOCK_COUNTER;
        mac_SetBTClkUSInt(MAC_BT_TIMER0, target_us);
    }
    else
    {
        otPlatAlarmMilliFired(aInstance);
    }
}

void __wrap_otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}
}

int main()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

#ifdef CONFIG_CHIP_PW_RPC
    rpc::Init();
#endif

    if (err == CHIP_NO_ERROR)
    {
        err = AppTask::Instance().StartApp();
    }

    LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
    return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
