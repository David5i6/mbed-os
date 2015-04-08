/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pinmap.h"
#include "error.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "mbed_assert.h"

static int gpio_clock_inited = 0;

void pin_function(PinName pin, int function)
{
    //Intentionally left empty. We have repurposed the function field.
}

void pin_mode(PinName pin, PinMode mode)
{
    MBED_ASSERT(pin != NC);

    /* Enable GPIO clock if not already done */
    if (!gpio_clock_inited) {
        CMU_ClockEnable(cmuClock_GPIO, true);
        gpio_clock_inited = 1;
    }

    /* Pin and port index encoded in one uint32.
     * First four bits represent the pin number
     * The remaining bits represent the port number */
    uint32_t pin_number = (uint32_t) pin;
    int pin_index = (pin_number & 0xF);
    int port_index = pin_number >> 4;

    /* Value of DOUT encoded in mode at position 0x10 */
    unsigned int dout = mode & 0x10;
    GPIO_PinModeSet(port_index, pin_index, mode & 0xF, dout);
}
