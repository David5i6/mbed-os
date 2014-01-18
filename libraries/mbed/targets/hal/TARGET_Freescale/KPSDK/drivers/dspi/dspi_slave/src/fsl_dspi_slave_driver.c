/*
 * Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_dspi_slave_driver.h"
#include "fsl_dspi_shared_irqs.h"
#include "fsl_clock_manager.h"
#include "fsl_interrupt_manager.h"
#include <string.h>
#include <assert.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! SPI slave constants */
enum _spi_slave_constants
{
    kEmptyChar = 0,                        /*!< Empty character */
};

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern IRQn_Type dspi_irq_ids[HW_SPI_INSTANCE_COUNT];

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief DSPI Slave Generic IRQ handler.
 *
 * This handler uses the callbacks stored in the dspi_slave_state_t struct to transfer data
 * either from the data source or to the data sink functions.
 * This is not a public API as it is called whenever an interrupt occurs.
 */
void dspi_slave_irq_handler(void * state)
{
    /* instantiate local variable of type dspi_slave_state_t and equate it to the
     * pointer to state
     */
    dspi_slave_state_t * dspiState = (dspi_slave_state_t *)state;

    uint32_t instance = dspiState->instance;

    /* Get the callback pointers from the run-time state structure */
    dspi_slave_callbacks_t * callbacks = &dspiState->callbacks;

    uint32_t sourceWord = kEmptyChar;
    uint8_t sourceWord1, sourceWord2, sourceWord3, sourceWord4;

    /* catch tx fifo underflow conditions */
    if (dspi_hal_get_status_flag(instance, kDspiTxFifoUnderflow))
    {
        /* Report SPI slave  transmit underrun error */
        if (callbacks->onError)
        {
            callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
        }
    }

    if (dspi_hal_get_status_flag(instance, kDspiRxFifoOverflow))
    {
        /* Report SPI slave  transmit underrun error */
        if (callbacks->onError)
        {
            callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
        }
    }

    /* Optimize for 1 byte data */
    if (dspiState->bitsPerFrame <= 8)
    {
        /* SPI transmit interrupt (1 byte) */
        /* Fill the tx fifo, where the fifo can be 1 entry or more */
        while(dspi_hal_get_status_flag(instance, kDspiTxFifoFillRequest))
        {
            /* If a call back is provided, get data from there, else send kEmptyChar */
            if (callbacks->dataSource)
            {
                /* get the first 8-bits of data */
                if (callbacks->dataSource(&sourceWord1, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                sourceWord = sourceWord1;
                /* Write the data to the DSPI data register */
                dspi_hal_write_data_slave_mode(instance, sourceWord);
            }
            else
            {
                /* If no callback provided, write empty char to data register */
                dspi_hal_write_data_slave_mode(instance, kEmptyChar);
            }
            /* try to clear TFFF by writing a one to it; it will not clear if TX FIFO not full */
            dspi_hal_clear_status_flag(instance, kDspiTxFifoFillRequest);
        }

        /* SPI receive interrupt (1 byte), read the data from the DSPI data register */
        /* Fill the rx fifo, where the fifo can be 1 entry or more */
        while (dspi_hal_get_status_flag(instance, kDspiRxFifoDrainRequest))
        {
            /* SPI receive interrupt, read the data from the DSPI data register */
            uint32_t readData = dspi_hal_read_data(instance);

            /* clear the rx fifo drain request, needed for non-DMA applications as this flag
             * will remain set even if the rx fifo is empty. By manually clearing this flag, it
             * either remain clear if no more data is in the fifo, or it will set if there is
             * more data in the fifo.
             */
            dspi_hal_clear_status_flag(instance, kDspiRxFifoDrainRequest);

            /* If callback provided, send data to it */
            if (callbacks->dataSink)
            {
                /* Sink the first 8-bits */
                if (callbacks->dataSink((uint8_t)readData, instance) != kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
            }
        }
    }
    /* Optimize for 2 bytes data */
    else if (dspiState->bitsPerFrame <= 16)
    {
        /* SPI transmit interrupt (2 bytes) */
        /* Fill the tx fifo, where the fifo can be 1 entry or more */
        while(dspi_hal_get_status_flag(instance, kDspiTxFifoFillRequest))
        {
            /* If a call back is provided, get data from there, else send kEmptyChar */
            if (callbacks->dataSource)
            {
                /* get the first 8-bits of data */
                if (callbacks->dataSource(&sourceWord1, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                /* get the next 8-bits of data */
                if (callbacks->dataSource(&sourceWord2, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                sourceWord = ((uint32_t)sourceWord2 << 8U) | sourceWord1;
                /* Finally, write the data to the DSPI data register */
                dspi_hal_write_data_slave_mode(instance, sourceWord);
            }
            else
            {
                /* If no callback provided, write empty char to data register */
                dspi_hal_write_data_slave_mode(instance, kEmptyChar);
            }
            /* try to clear TFFF by writing a one to it; it will not clear if TX FIFO not full */
            dspi_hal_clear_status_flag(instance, kDspiTxFifoFillRequest);
        }

        /* SPI receive interrupt (2 bytes), read the data from the DSPI data register */
        /* Fill the rx fifo, where the fifo can be 1 entry or more */
        while (dspi_hal_get_status_flag(instance, kDspiRxFifoDrainRequest))
        {
            /* SPI receive interrupt, read the data from the DSPI data register */
            uint32_t readData = dspi_hal_read_data(instance);

            /* clear the rx fifo drain request, needed for non-DMA applications as this flag
             * will remain set even if the rx fifo is empty. By manually clearing this flag, it
             * either remain clear if no more data is in the fifo, or it will set if there is
             * more data in the fifo.
             */
            dspi_hal_clear_status_flag(instance, kDspiRxFifoDrainRequest);

            /* If callback provided, send data to it */
            if (callbacks->dataSink)
            {

                /* Sink the first 8-bits */
                if (callbacks->dataSink((uint8_t)readData, instance) != kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
                /* Sink the next 8-bits */
                if (callbacks->dataSink((uint8_t)(readData >> 8), instance) != kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
            }
        }

    }
    /* Optimize for 3 bytes data */
    else if (dspiState->bitsPerFrame <= 24)
    {
        /* SPI transmit interrupt (3 bytes) */
        /* Fill the tx fifo, where the fifo can be 1 entry or more */
        while(dspi_hal_get_status_flag(instance, kDspiTxFifoFillRequest))
        {
            /* If a call back is provided, get data from there, else send kEmptyChar */
            if (callbacks->dataSource)
            {
                /* get the first 8-bits of data */
                if (callbacks->dataSource(&sourceWord1, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                /* get the next 8-bits of data */
                if (callbacks->dataSource(&sourceWord2, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                /* get the next 8-bits of data */
                if (callbacks->dataSource(&sourceWord3, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                sourceWord = ((uint32_t)sourceWord3 << 16U) | ((uint32_t)sourceWord2 << 8U) |
                             sourceWord1;
                /* Finally, write the data to the DSPI data register */
                dspi_hal_write_data_slave_mode(instance, sourceWord);
            }
            else
            {
                /* If no callback provided, write empty char to data register */
                dspi_hal_write_data_slave_mode(instance, kEmptyChar);
            }
            /* try to clear TFFF by writing a one to it; it will not clear if TX FIFO not full */
            dspi_hal_clear_status_flag(instance, kDspiTxFifoFillRequest);
        }

        /* SPI receive interrupt (3 bytes), read the data from the DSPI data register */
        /* Fill the rx fifo, where the fifo can be 1 entry or more */
        while (dspi_hal_get_status_flag(instance, kDspiRxFifoDrainRequest))
        {
            /* SPI receive interrupt, read the data from the DSPI data register */
            uint32_t readData = dspi_hal_read_data(instance);

            /* clear the rx fifo drain request, needed for non-DMA applications as this flag
             * will remain set even if the rx fifo is empty. By manually clearing this flag, it
             * either remain clear if no more data is in the fifo, or it will set if there is
             * more data in the fifo.
             */
            dspi_hal_clear_status_flag(instance, kDspiRxFifoDrainRequest);

            /* If callback provided, send data to it */
            if (callbacks->dataSink)
            {

                /* Sink the first 8-bits */
                if (callbacks->dataSink((uint8_t)readData, instance) != kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
                /* Sink the next 8-bits */
                if (callbacks->dataSink((uint8_t)(readData >> 8), instance) != kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
                /* Sink the next 8-bits */
                if (callbacks->dataSink((uint8_t)(readData >> 16), instance) !=
                    kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
            }
        }
    }
    /* Optimize for 4 bytes data */
    else
    {
        /* SPI transmit interrupt (4 bytes) */
        /* Fill the tx fifo, where the fifo can be 1 entry or more */
        while(dspi_hal_get_status_flag(instance, kDspiTxFifoFillRequest))
        {
            /* If a call back is provided, get data from there, else send kEmptyChar */
            if (callbacks->dataSource)
            {
                /* get the first 8-bits of data */
                if (callbacks->dataSource(&sourceWord1, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                /* get the next 8-bits of data */
                if (callbacks->dataSource(&sourceWord2, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                /* get the next 8-bits of data */
                if (callbacks->dataSource(&sourceWord3, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }
                /* get the next 8-bits of data */
                if (callbacks->dataSource(&sourceWord4, instance) != kStatus_DSPI_Success)
                {
                    /* Report SPI slave transmit underrun error */
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveTxUnderrun, instance);
                    }
                }

                sourceWord = ((uint32_t)sourceWord4 << 24U) | ((uint32_t)sourceWord3 << 16U) |
                             ((uint32_t)sourceWord2 << 8U) | sourceWord1;
                /* Finally, write the data to the DSPI data register */
                dspi_hal_write_data_slave_mode(instance, sourceWord);
            }
            else
            {
                /* If no callback provided, write empty char to data register */
                dspi_hal_write_data_slave_mode(instance, kEmptyChar);
            }
            /* try to clear TFFF by writing a one to it; it will not clear if TX FIFO not full */
            dspi_hal_clear_status_flag(instance, kDspiTxFifoFillRequest);
        }

        /* SPI receive interrupt (4 bytes), read the data from the DSPI data register */
        /* Fill the rx fifo, where the fifo can be 1 entry or more */
        while (dspi_hal_get_status_flag(instance, kDspiRxFifoDrainRequest))
        {
            /* SPI receive interrupt, read the data from the DSPI data register */
            uint32_t readData = dspi_hal_read_data(instance);

            /* clear the rx fifo drain request, needed for non-DMA applications as this flag
             * will remain set even if the rx fifo is empty. By manually clearing this flag, it
             * either remain clear if no more data is in the fifo, or it will set if there is
             * more data in the fifo.
             */
            dspi_hal_clear_status_flag(instance, kDspiRxFifoDrainRequest);

            /* If callback provided, send data to it */
            if (callbacks->dataSink)
            {

                /* Sink the first 8-bits */
                if (callbacks->dataSink((uint8_t)readData, instance) != kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
                /* Sink the next 8-bits */
                if (callbacks->dataSink((uint8_t)(readData >> 8), instance) != kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
                /* Sink the next 8-bits */
                if (callbacks->dataSink((uint8_t)(readData >> 16), instance) !=
                    kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
                /* Sink the next 8-bits */
                if (callbacks->dataSink((uint8_t)(readData >> 24), instance) !=
                    kStatus_DSPI_Success)
                {
                    // Report SPI receive overrun error
                    if (callbacks->onError)
                    {
                        callbacks->onError(kStatus_DSPI_SlaveRxOverrun, instance);
                    }
                }
            }
        }
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : dspi_slave_init
 * Description   : Initialize a DSPI instance for slave mode operation.
 * This function saves the callbacks to the run-time state structure for later use in the
 * interrupt handler. It also ungates the clock to the DSPI module, initializes the DSPI
 * for slave mode, enables the module and corresponding interrupts. Once initialized, the
 * DSPI module is configured in slave mode and ready to receive data from a SPI master. The
 * following is an example of how to set up the dspi_slave_state_t and the dspi_slave_user_config_t
 * parameters and how to call the dspi_slave_init function by passing in these parameters:
 *   dspi_slave_state_t dspiSlaveState; <- the user simply allocates memory for this struct
 *   dspi_slave_user_config_t slaveUserConfig;
 *   slaveUserConfig.callbacks.dataSink = data_sink; <- set to user implementation of function
 *   slaveUserConfig.callbacks.dataSource = data_source; <- set to user implementation of function
 *   slaveUserConfig.callbacks.onError = on_error; <- set to user implementation of function
 *   slaveUserConfig.dataConfig.bitsPerFrame = 16;
 *   slaveUserConfig.dataConfig.clkPhase = kDspiClockPhase_FirstEdge;
 *   slaveUserConfig.dataConfig.clkPolarity = kDspiClockPolarity_ActiveHigh;
 *   dspi_slave_init(slaveInstance, &slaveUserConfig, &dspiSlaveState);
 *
 *END**************************************************************************/
dspi_status_t dspi_slave_init(uint32_t instance, const dspi_slave_user_config_t * slaveConfig,
                     dspi_slave_state_t * dspiState)
{
    assert(slaveConfig);
    assert(instance < HW_SPI_INSTANCE_COUNT);

    dspi_status_t errorCode = kStatus_DSPI_Success;

    /* DSPI config struct in hal, fill out it's members below */
    dspi_slave_config_t dspiConfig;

    /* Clear the run-time state struct for this instance. */
    memset(dspiState, 0, sizeof(* dspiState));

    /* Save the application info. */
    dspiState->callbacks = slaveConfig->callbacks;

    /* configure the run-time state struct with the instance number */
    dspiState->instance = instance;

    /* configure the run-time state struct with the nubmer of bits/frame */
    dspiState->bitsPerFrame = slaveConfig->dataConfig.bitsPerFrame;

    /* Enable clock for DSPI */
    clock_manager_set_gate(kClockModuleSPI, instance, true);

    /* Reset the DSPI module */
    dspi_hal_reset(instance);

    /* Initialize the parameters of the hal DSPI config structure with desired data
     * members of the user config struct, then the hal init function will be called
     */
    dspiConfig.isEnabled = false;  /* disable the DSPI module while we're setting it up */
    dspiConfig.isTxFifoDisabled = false;  /* enable tx fifo */
    dspiConfig.isRxFifoDisabled = false;  /* enable rx fifo */
    /* data format field config */
    dspiConfig.dataConfig.bitsPerFrame = slaveConfig->dataConfig.bitsPerFrame;
    dspiConfig.dataConfig.clkPolarity = slaveConfig->dataConfig.clkPolarity;
    dspiConfig.dataConfig.clkPhase = slaveConfig->dataConfig.clkPhase;

    errorCode = dspi_hal_slave_init(instance, &dspiConfig);
    if (errorCode!= kStatus_DSPI_Success)
    {
        return errorCode;  /* return immediately if there's a problem with the init */
    }

    /* DSPI system enable */
    dspi_hal_enable(instance);

    /* flush the fifos*/
    dspi_hal_flush_fifos(instance, true, true);

    /* Configure IRQ state structure, so irq handler can point to the correct state structure */
    dspi_set_shared_irq_state(instance, dspiState, false);

    /* TX FIFO Fill Flag (TFFF) request enable*/
    dspi_hal_configure_interrupt(instance, kDspiTxFifoFillRequest, true);
    /* RX FIFO Drain request: RFDF_RE to enable RFDF interrupt */
    dspi_hal_configure_interrupt(instance, kDspiRxFifoDrainRequest, true);
    /* TX FIFO underflow request enable*/
    dspi_hal_configure_interrupt(instance, kDspiTxFifoUnderflow, true);

    /* Write 0 to tx shift register so a master will receive a known first word (0). */
    dspi_hal_write_data_slave_mode(instance, 0);

    /* Clear the Tx FIFO Fill Flag (TFFF) status bit
     * This should be done after writing the data to the PUSHR register
     */
    dspi_hal_clear_status_flag(instance, kDspiTxFifoFillRequest);

    /* Start DSPI transfers, set to running state */
    dspi_hal_start_transfer(instance);

    /* Enable the interrupt */
    interrupt_enable(dspi_irq_ids[instance]);

    return errorCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : dspi_slave_shutdown
 * Description   : Shutdown a DSPI instance.
 * Resets the DSPI peripheral, disables the interrupt to the core, and gates its clock.
 *
 *END**************************************************************************/
void dspi_slave_shutdown(dspi_slave_state_t * dspiState)
{
    uint32_t instance = dspiState->instance;

    assert(instance < HW_SPI_INSTANCE_COUNT);

    /* disable the interrupt */
    interrupt_disable(dspi_irq_ids[instance]);

    /* Stop the transfer process in the slave */
    dspi_hal_stop_transfer(instance);

    /* Wait until the DSPI run status signals that is has halted before shutting
     * down the module and before gating off the DSPI clock source.  Otherwise, if the DSPI
     * is shut down before it has halted it's internal processes, it may be left in an unknown
     * state.
     * Note that if the master slave select is still asserted, the run status will never clear.
     * Hence, ensure before shutting down the slave that the master has de-asserted the slave
     * select signal (it should be high if slave select active low or it should be low if
     * slave select is active high).
     */
    while((dspi_hal_get_status_flag(instance, kDspiTxAndRxStatus))) { }

    /* Restore the module to defaults then power it down. This also disables the DSPI module. */
    dspi_hal_reset(instance);

    /* Gate the clock for DSPI. */
    clock_manager_set_gate(kClockModuleSPI, instance, false);
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

