#include "TerminalIO.hpp"
#include "SpecialAssert.hpp"
#include "Wrapper/Sync.hpp"

#include <cmsis_os2.h>
#include <cstring>

// DO NOT USE LOGGING IN THIS TASK AS IT WILL CAUSE SLOWDOWN
// Due to ram constraints the TX stream is very small
// log will cause a call to write() which when the stream is full
// will block until timeout or data is transmitted
// this task is the only one transmitting so timeout will always occur

namespace remote_control_device
{
TerminalIO *TerminalIO::_instance{nullptr};

TerminalIO::TerminalIO(wrapper::HAL &hal, UART_HandleTypeDef &uart)
    : _hal(hal), _uart(uart),
      _task(&TerminalIO::taskMain, "TerminalIO", StackSize, this,
            static_cast<UBaseType_t>(osPriority_t::osPriorityNormal), wrapper::sync::TerminalIO_Ready),
      _log(*this, _hal)
{
    specialAssert(_instance == nullptr);
    _instance = this;

    _txStream = xStreamBufferCreate(TX_BUFFER_SIZE, 1);

    HAL_UART_RegisterCallback(&_uart, HAL_UART_TX_COMPLETE_CB_ID, &TerminalIO::cbTxCompleteISR);
    HAL_UART_RegisterCallback(&_uart, HAL_UART_ERROR_CB_ID, &TerminalIO::cbErrorISR);
}

TerminalIO::~TerminalIO()
{
    _instance = nullptr;
    vStreamBufferDelete(_txStream);
}

void TerminalIO::dispatch(uint32_t flags)
{
    // some error, restart everyting
    if ((flags & NOTIFY_ERROR) > 0)
    {
        HAL_UART_Abort(&_uart);
        flags |= NOTIFY_TX_START;
    }

    // transmit data via dma
    if ((flags & NOTIFY_TX_START) > 0)
    {
        // check if transfer isn't already running
        if (_uart.gState == HAL_UART_STATE_READY)
        {
            if (_transferSize > 0)
            {
                _txAttempts++;
            }
            if (_txAttempts > MAX_TX_ATTEMPTS)
            {
                _txAttempts = 0;
                _transferSize = 0;
            }
            if (_transferSize == 0)
            {
                // last transfer was sucessful or aborted, get new data
                _transferSize =
                    xStreamBufferReceive(_txStream, reinterpret_cast<void *>(_txTransferBuffer),
                                         TX_TRANSFER_BUFFER_SIZE, 0);
            }
            if (_transferSize > 0)
            {
                if (HAL_UART_Transmit_DMA(&_uart, _txTransferBuffer, _transferSize) !=
                    HAL_OK) // NOLINT
                {
                    testHook_ErrorHALTXStart();
                    _task.notify(NOTIFY_TX_START, eNotifyAction::eSetBits);
                }
            }
        }
    }
}

void TerminalIO::write(const char *str)
{
    const auto sp = std::span(str, strlen(str));
    write(sp);
}

void TerminalIO::write(std::span<const char> str)
{
    static constexpr uint8_t MaxAttempts = 10;
    size_t current{0};
    size_t len{str.size()};

    // adding to the buffer bit by bit and notifying
    // as trying to copy it all in one go would never reach xTaskNotify
    // without discarding some of the buffer when the stream is very full
    for (uint8_t i = 0; i < MaxAttempts; ++i)
    {
        const size_t bytesCopied{xStreamBufferSend(
            _txStream, reinterpret_cast<void *>(const_cast<char *>(&str[current])), len,
            WRITE_MAX_WAITTIME / 10)};
        _task.notify(NOTIFY_TX_START, eNotifyAction::eSetBits);

        specialAssert(bytesCopied <= str.size());
        len -= bytesCopied;
        current += bytesCopied;
        if (len == 0)
        {
            return;
        }
    }
}

void TerminalIO::finishISR(uint32_t flags)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TerminalIO &instance = *TerminalIO::_instance;

    instance._task.notifyFromISR(flags, eNotifyAction::eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // NOLINT
}

void TerminalIO::cbTxCompleteISR(UART_HandleTypeDef *huart)
{
    TerminalIO &instance = *TerminalIO::_instance;

    // by resetting transfer size we signal a successful transfer
    _instance->signalTXSuccessFromISR();
    finishISR(NOTIFY_TX_START);
}

void TerminalIO::cbErrorISR(UART_HandleTypeDef *huart)
{
    finishISR(NOTIFY_ERROR);
}

void TerminalIO::taskMain(void *parameter)
{
    uint32_t notifyValue = 0;
    for (;;)
    {
        _instance->dispatch(notifyValue);
        _instance->_task.notifyWait(0, wrapper::Task::ClearAllBits, &notifyValue, portMAX_DELAY);
    }
}
} // namespace remote_control_device