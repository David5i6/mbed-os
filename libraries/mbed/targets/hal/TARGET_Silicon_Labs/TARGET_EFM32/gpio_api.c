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
#include "gpio_api.h"
#include "pinmap.h"
#include "em_cmu.h"
#include "mbed_assert.h"

/*
 * @return the GPIO port mask for this pin
 * Pin and port index encoded in one uint32.
 * First four bits represent the pin number
 * The remaining bits represent the pin mode
 */
uint32_t gpio_set(PinName pin) {
    return 1 << ((uint32_t) pin & 0xF);
}

void gpio_init(gpio_t *obj, PinName pin) {
    MBED_ASSERT(pin != NC);

    //Enable GPIO clock
    CMU_ClockEnable(cmuClock_GPIO, true);

    obj->pin = pin;
    obj->mask = gpio_set(pin);
    obj->port = pin >> 4;
}

void gpio_mode(gpio_t *obj, PinMode mode) {
    obj->mode = mode; // Update object
    pin_mode(obj->pin, mode); // Update register
}

// Used by DigitalInOut to set correct mode when direction is set
void gpio_dir(gpio_t *obj, PinDirection direction) {
    obj->dir = direction;
    switch (direction) {
    case PIN_INPUT :
        gpio_mode(obj, PullDefault);
        break;
    case PIN_OUTPUT :
        gpio_mode(obj, PullNone);
        break;
    }
}


