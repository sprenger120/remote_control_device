#pragma once
#include "Logging.hpp"
#include "SpanCompatibility.hpp"
#include "Wrapper/HAL.hpp"
#include "Wrapper/Task.hpp"
#include <stream_buffer.h>

#include <stm32f3xx_hal.h>
#include <stm32f3xx_hal_uart.h>

/**
 * @brief Receives / Transmits string data over the NUCLEO's serial connection
 * Usually this is done via the USART2 peripheral but due to a layout error
 * USARTS had to be swapped so now USART1 is used.
 *
 * Parameters are 115200 boud 8N1
 */

namespace remote_control_device
{

class TerminalIO
{
public:
    static constexpr uint16_t StackSize = 150;
    TerminalIO(wrapper::HAL &hal, UART_HandleTypeDef &uart);
    virtual ~TerminalIO();

    TerminalIO(const TerminalIO &) = delete;
    TerminalIO(TerminalIO &&) = delete;
    TerminalIO &operator=(const TerminalIO &) = delete;
    TerminalIO &operator=(TerminalIO &&) = delete;

    /**
     * @brief Starts TX and RX DMA, executes received commands
     *
     * @param flags
     */
    virtual void dispatch(uint32_t flags);
    static constexpr uint32_t NOTIFY_TX_START = 1 << 0;
    static constexpr uint32_t NOTIFY_ERROR = 1 << 3;

    /**
     * @brief Retrieves logging instance
     *
     * @return Logging&
     */
    virtual Logging &getLogging() {
        return _log;
    };

    /**
     * @brief How long Logging should wait at max to transmit data
     * Increase for high logging rate
     *
     */
    static constexpr TickType_t WRITE_MAX_WAITTIME = pdMS_TO_TICKS(50);

    /**
     * @brief Size of TX data buffer
     * Increase for high logging rate
     */
    static constexpr size_t TX_BUFFER_SIZE = 64;

    /**
     * @brief Chunk size for transmission
     * Increase for high logging rate
     */
    static constexpr uint8_t TX_TRANSFER_BUFFER_SIZE = 16;

    /**
     * @brief How often retransmission should be attempted for the same chunk before data is dropped
     *
     */
    static constexpr uint8_t MAX_TX_ATTEMPTS = 3;

    /**
     * @brief Adds data to be transmitted later,
     * SteamBuffers may only be written / read from one task
     * Logging provides mutex against mutual access
     */
    virtual void write(const char *); // only use with literals
    virtual void write(std::span<const char>);

    /* Hooks for testing */
    static void testHook_ErrorHALTXStart(){};
    virtual void signalTXSuccessFromISR() {
        _transferSize = 0;
    }

private:
    wrapper::HAL &_hal;
    UART_HandleTypeDef &_uart;
    wrapper::Task _task;
    static TerminalIO *_instance;

    // tx
    Logging _log;
    StreamBufferHandle_t _txStream;
    volatile size_t _transferSize = 0;
    uint8_t _txAttempts = 0;
    uint8_t _txTransferBuffer[TX_TRANSFER_BUFFER_SIZE] = {0}; // NOLINT

    static void cbTxCompleteISR(UART_HandleTypeDef *huart);
    static void cbErrorISR(UART_HandleTypeDef *huart);
    static void finishISR(uint32_t flags);

    static void taskMain(void *parameter);
};

} // namespace remote_control_device
