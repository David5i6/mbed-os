/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2014, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */
#include "spi_api.h"

#if DEVICE_SPI

#include <math.h>
#include "cmsis.h"
#include "pinmap.h"
#include "error.h"
#include "stm32f4xx_hal.h"

static const PinMap PinMap_SPI_MOSI[] = {
    {PA_7,  SPI_1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    {PB_5,  SPI_1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    {PC_3,  SPI_2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    {PB_15, SPI_2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    {PC_12, SPI_3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3)},
    {PB_5 , SPI_3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3)},
    {NC,    NC,    0}
};

static const PinMap PinMap_SPI_MISO[] = {
    {PA_6,  SPI_1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    {PB_4,  SPI_1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    {PC_2,  SPI_2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    {PB_14, SPI_2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    {PC_11, SPI_3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3)},
    {PB_4 , SPI_3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3)},
    {NC,    NC,    0}
};

static const PinMap PinMap_SPI_SCLK[] = {
    {PA_5,  SPI_1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    {PB_3,  SPI_1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI1)},
    {PB_10, SPI_2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    {PB_13, SPI_2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF5_SPI2)},
    {PC_10, SPI_3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3)},
    {PB_3 , SPI_3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3)},
    {NC,    NC,    0}
};

// Only used in Slave mode
static const PinMap PinMap_SPI_SSEL[] = {
	{PA_4,  SPI_1, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, GPIO_AF5_SPI1)},
	{PA_15, SPI_1, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, GPIO_AF5_SPI1)},
	{PB_9 , SPI_2, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, GPIO_AF5_SPI2)},
	{PB_12, SPI_2, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, GPIO_AF5_SPI2)},
	{PA_4 , SPI_3, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, GPIO_AF6_SPI3)},
	{PA_15, SPI_3, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, GPIO_AF6_SPI3)},
    {NC,    NC,    0}
};

static SPI_HandleTypeDef SpiHandle;

static void init_spi(spi_t *obj) {
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
  
    __HAL_SPI_DISABLE(&SpiHandle);

    SpiHandle.Init.Mode              = obj->mode;
    SpiHandle.Init.BaudRatePrescaler = obj->br_presc;
    SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
    SpiHandle.Init.CLKPhase          = obj->cpha;
    SpiHandle.Init.CLKPolarity       = obj->cpol;
    SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
    SpiHandle.Init.CRCPolynomial     = 7;
    SpiHandle.Init.DataSize          = obj->bits;
    SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    SpiHandle.Init.NSS               = obj->nss;
    SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLED;
  
    HAL_SPI_Init(&SpiHandle);

    __HAL_SPI_ENABLE(&SpiHandle);
}

void spi_init(spi_t *obj, PinName mosi, PinName miso, PinName sclk, PinName ssel) {
    // Determine the SPI to use
    SPIName spi_mosi = (SPIName)pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    SPIName spi_miso = (SPIName)pinmap_peripheral(miso, PinMap_SPI_MISO);
    SPIName spi_sclk = (SPIName)pinmap_peripheral(sclk, PinMap_SPI_SCLK);
    SPIName spi_ssel = (SPIName)pinmap_peripheral(ssel, PinMap_SPI_SSEL);
  
    SPIName spi_data = (SPIName)pinmap_merge(spi_mosi, spi_miso);
    SPIName spi_cntl = (SPIName)pinmap_merge(spi_sclk, spi_ssel);
  
    obj->spi = (SPIName)pinmap_merge(spi_data, spi_cntl);
  
    if (obj->spi == (SPIName)NC) {
        error("SPI error: pinout mapping failed.");
    }
    
    // Enable SPI clock
    if (obj->spi == SPI_1) {
        __SPI1_CLK_ENABLE();
    }
    if (obj->spi == SPI_2) {
        __SPI2_CLK_ENABLE();
    }
    if (obj->spi == SPI_3) {
        __SPI3_CLK_ENABLE();
    }
    
    // Configure the SPI pins
    pinmap_pinout(mosi, PinMap_SPI_MOSI);
    pinmap_pinout(miso, PinMap_SPI_MISO);
    pinmap_pinout(sclk, PinMap_SPI_SCLK);
    if (ssel != NC) {									// slave mode
            pinmap_pinout(ssel, PinMap_SPI_SSEL);
        }
    
    // Save new values
    obj->bits = SPI_DATASIZE_8BIT;
    obj->cpol = SPI_POLARITY_LOW;
    obj->cpha = SPI_PHASE_1EDGE;
    obj->br_presc = SPI_BAUDRATEPRESCALER_256; // 1MHz (with HSI=16MHz and APB2CLKDivider=2)
    
    if (ssel == NC) { // SW NSS Master mode
        obj->mode = SPI_MODE_MASTER;
        obj->nss = SPI_NSS_SOFT;
    }
    else { // Slave
        pinmap_pinout(ssel, PinMap_SPI_SSEL);
        obj->mode = SPI_MODE_SLAVE;
        obj->nss = SPI_NSS_HARD_INPUT;
    }

    init_spi(obj);
}

void spi_free(spi_t *obj) {
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
    HAL_SPI_DeInit(&SpiHandle);
}

void spi_format(spi_t *obj, int bits, int mode, int slave) {  
    // Save new values
    if (bits == 8) {
        obj->bits = SPI_DATASIZE_8BIT;
    }
    else {
        obj->bits = SPI_DATASIZE_16BIT;
    }
    
    switch (mode) {
        case 0:
          obj->cpol = SPI_POLARITY_LOW;
          obj->cpha = SPI_PHASE_1EDGE;
        break;
        case 1:
          obj->cpol = SPI_POLARITY_LOW;
          obj->cpha = SPI_PHASE_2EDGE;
        break;
        case 2:
          obj->cpol = SPI_POLARITY_HIGH;
          obj->cpha = SPI_PHASE_1EDGE;          
        break;
        default:
          obj->cpol = SPI_POLARITY_HIGH;
          obj->cpha = SPI_PHASE_2EDGE;          
        break;
    }
    
    if (slave == 0) {
        obj->mode = SPI_MODE_MASTER;
        obj->nss = SPI_NSS_SOFT;
    }
    else {
        obj->mode = SPI_MODE_SLAVE;
        obj->nss = SPI_NSS_HARD_INPUT;      
    }
    
    init_spi(obj);
}

void spi_frequency(spi_t *obj, int hz) {
    // Get SPI clock frequency

	// SPI1 runs from PCLK2, which runs at SystemCoreClock / 2.  SPI2 and SPI3
    // run from PCLK1, which runs at SystemCoreClock / 4.
    uint32_t PCLK = SystemCoreClock;
    switch ((int)obj->spi) {
           case SPI_1: PCLK = PCLK >> 1; break;
           case SPI_2: PCLK = PCLK >> 2; break;
           case SPI_3: PCLK = PCLK >> 2; break;
    }
    // Choose the baud rate divisor (between 2 and 256)
    uint32_t divisor = PCLK / hz;

    // Find the nearest power-of-2
    divisor = (divisor > 0 ? divisor-1 : 0);
    divisor |= divisor >> 1;
    divisor |= divisor >> 2;
    divisor |= divisor >> 4;
    divisor |= divisor >> 8;
    divisor |= divisor >> 16;
    divisor++;

    uint32_t baud_rate = __builtin_ffs(divisor) - 2;
    
    // Save new value
    obj->br_presc = ((baud_rate > 7) ? (7 << 3) : (baud_rate << 3));
 
    init_spi(obj);
}

static inline int ssp_readable(spi_t *obj) {
    int status;
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
    // Check if data is received
    status = ((__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_RXNE) != RESET) ? 1 : 0);
    return status;  
}

static inline int ssp_writeable(spi_t *obj) {
    int status;
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
    // Check if data is transmitted
    status = ((__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_TXE) != RESET) ? 1 : 0);
    return status;
}

static inline void ssp_write(spi_t *obj, int value) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    while (!ssp_writeable(obj));
    if(obj->bits == SPI_DATASIZE_8BIT)
            spi->DR = (uint8_t)value;       // 8 bit mode
    else spi->DR = (uint16_t)value;   		// 16 bit mode
}

static inline int ssp_read(spi_t *obj) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    while (!ssp_readable(obj));
    return (int)spi->DR;
}

static inline int ssp_busy(spi_t *obj) {
    int status;
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
    status = ((__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_BSY) != RESET) ? 1 : 0);
    return status;
}

int spi_master_write(spi_t *obj, int value) {
    ssp_write(obj, value);
    return ssp_read(obj);
}

int spi_slave_receive(spi_t *obj) {
    return (ssp_readable(obj) && !ssp_busy(obj)) ? (1) : (0);
};

int spi_slave_read(spi_t *obj) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    return (int)spi->DR;
}

void spi_slave_write(spi_t *obj, int value) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    while (!ssp_writeable(obj));
    spi->DR = (uint8_t)value;
}

int spi_busy(spi_t *obj) {
    return ssp_busy(obj);
}

#endif
