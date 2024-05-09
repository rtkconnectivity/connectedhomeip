/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

/**
 * @file LEDWidget.cpp
 *
 * Implements an LED Widget controller that is usually tied to a GPIO
 * It also updates the display widget if it's enabled
 */

#include <stdlib.h>
#include "LEDWidget.h"

void LEDWidget::InitGpio()
{
    
}

void LEDWidget::Init(uint8_t gpioNum)
{
    mState     = false;
}

void LEDWidget::Invert(void)
{
    Set(!mState);
}

void LEDWidget::Set(bool state)
{
    mState = state;
}

void LEDWidget::Blink(uint32_t changeRateMS)
{
    
}

void LEDWidget::Blink(uint32_t onTimeMS, uint32_t offTimeMS)
{
    
}

void LEDWidget::BlinkStop()
{
    
}



