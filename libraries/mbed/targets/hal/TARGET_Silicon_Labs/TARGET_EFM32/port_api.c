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

#include "device.h"
#if DEVICE_PORTOUT

#include "port_api.h"
#include "pinmap.h"
#include "gpio_api.h"
#include "em_gpio.h"
#include "em_cmu.h"

#define PORT_NUM_PINS 16

PinName port_pin(PortName port, int pin_n)
{
    return (PinName)(pin_n | port << 4); // Encode pin and port number in one uint32
}

void port_init(port_t *obj, PortName port, int mask, PinDirection dir)
{
    /* Enable GPIO clock */
    CMU_ClockEnable(cmuClock_GPIO, true);

    obj->mask = mask;
    obj->port = port;
    obj->dir = dir;

    port_dir(obj, dir);
}

void port_mode(port_t *obj, PinMode mode)
{
    /* Set mode for pins given by mask */
    for (uint32_t pin = 0; pin < PORT_NUM_PINS; pin++) {
        if (obj->mask & (1 << pin)) {
            pin_mode(port_pin(obj->port, pin), mode);
        }
    }
}

void port_dir(port_t *obj, PinDirection dir)
{
    obj->dir = dir;

    /* Set default pin mode for pins given by mask */
    switch (dir) {
        case PIN_INPUT:
            port_mode(obj, PullDefault);
            break;
        case PIN_OUTPUT:
            port_mode(obj, PullNone);
            break;
    }
}

void port_write(port_t *obj, int value)
{
    if (value) {
        GPIO_PortOutSet(obj->port, obj->mask);
    } else {
        GPIO_PortOutClear(obj->port, obj->mask);
    }
}

int port_read(port_t *obj)
{
    if (obj->dir == PIN_INPUT) {
        return (int) (GPIO_PortInGet(obj->port) & obj->mask);
    } else {
        return (int) (GPIO_PortOutGet(obj->port) & obj->mask);
    }
}

#endif
