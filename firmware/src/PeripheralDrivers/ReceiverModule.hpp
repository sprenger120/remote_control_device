#pragma once
#include "SBUSDecoder.hpp"
#include "Wrapper/Task.hpp"
#include <semphr.h>
#include <stm32f3xx_hal.h>

/**
 * @brief Receives data from the FrSky XM+ module via USART1
 * See TerminalIO for information about layout error and USART1 and 2 swap
 *
 * The XM+ receiver module sends discret frames of data with start
 * byte, payload and an endbyte.
 * Transmission from the receiver board starts upon first contect with the
 * remote control and continued even when radio link is dropped.
 * (Flags will signal loss of connection)

 * As this task can start any time within a frame and the DMA requires
 * the exact size of the data to receive; the receive timeout functionality
 * is used to abort half complete receptions and retry.
 */

namespace wrapper
{
class HAL;
}

namespace remote_control_device
{
class Logging;

template <uint32_t usec>
constexpr uint16_t getTimerPeriod()
{
    constexpr uint32_t timerClock = 64'000'000;
    constexpr uint16_t prescaler = 16;
    constexpr uint16_t MaxTimerRuntimeUSec = 16'000;
    static_assert(usec < MaxTimerRuntimeUSec);

    const uint64_t ret = ((usec * (timerClock / 1000)) / prescaler) / 1000;
    return ret;
}

class ReceiverModule
{
public:
    static constexpr uint16_t StackSize = 220;

    ReceiverModule(UART_HandleTypeDef &huart, TIM_HandleTypeDef &tim, wrapper::HAL &hal,
                   Logging &log);
    virtual ~ReceiverModule();

    ReceiverModule(const ReceiverModule &) = delete;
    ReceiverModule(ReceiverModule &&) = delete;
    ReceiverModule &operator=(const ReceiverModule &) = delete;
    ReceiverModule &operator=(ReceiverModule &&) = delete;

    /**
     * @brief Handles uart peripheral, decodes data upon sucessful reception
     *
     * @param flags
     */
    virtual void dispatch(uint32_t flags);
    static constexpr uint32_t NOTIFY_RX_START = 1 << 0;
    static constexpr uint32_t NOTIFY_RX_SUCCESSFUL = 1 << 1;
    static constexpr uint32_t NOTIFY_ERROR = 1 << 2;

    /**
     * @brief Retrieves the latest s.bus frame. Blocks for max. MUTEX_TIMEOUT
     *
     * @param frame target to write to
     * @return true frame sucessfully copied
     * @return false mutex timeout
     */
    virtual bool getSBUSFrame(SBUS::Frame &frame);
    static constexpr TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(50);

    /**
     * @brief Used in Receive timeout detection
     *
     */
    static constexpr uint32_t FrameTime_us = 3000;
    static constexpr uint32_t InterFrameDelay_us = 7000;
    static constexpr uint16_t TimerPeriod_Timeout =
        getTimerPeriod<FrameTime_us + InterFrameDelay_us + 1000>();

    /**
     * @brief Hippomocks hooks for testing
     *
     */
    static void testing_RxRestart(){};
    static void testing_SuccessfulDecode(){};
    static void testing_ErrorDecode(){};

private:
    UART_HandleTypeDef &_uart;
    TIM_HandleTypeDef &_tim;
    wrapper::HAL &_hal;
    Logging &_log;
    wrapper::Task _task;
    static ReceiverModule* _instance;

    SBUS::Protocol::FrameData _rxBuffer;
    SBUS::Frame _decodedFrame;
    SemaphoreHandle_t _frameSemphr;
    volatile bool _synchronized = false;

    void restartTimer();

    static void cbRxCompleteISR(UART_HandleTypeDef *huart);
    static void cbRxAbortISR(UART_HandleTypeDef *huart);
    static void finishISR(uint32_t flags);
    static void cbPeriodElapsedISR(TIM_HandleTypeDef *htim);

    static void taskMain(void* parameter);
};

} // namespace remote_control_device