#include "ReceiverModule.hpp"
#include "BuildConfiguration.hpp"
#include "Logging.hpp"
#include "SpecialAssert.hpp"
#include "Wrapper/Sync.hpp"
#include "Wrapper/HAL.hpp"
#include <cmsis_os.h>
#include <limits>
#include <task.h>

namespace remote_control_device
{
ReceiverModule *ReceiverModule::_instance{nullptr};

ReceiverModule::ReceiverModule(UART_HandleTypeDef &uart, TIM_HandleTypeDef &tim, wrapper::HAL &hal,
                               Logging &log)
    : _uart(uart), _tim(tim), _hal(hal), _log(log),
      _task(&ReceiverModule::taskMain, "ReceiverModule", StackSize, reinterpret_cast<void *>(this),
            osPriority_t::osPriorityAboveNormal, wrapper::sync::ReceiverModule_Ready)
{
    specialAssert(_instance == nullptr);
    _instance = this;

    HAL_UART_RegisterCallback(&_uart, HAL_UART_RX_COMPLETE_CB_ID, &ReceiverModule::cbRxCompleteISR);
    HAL_UART_RegisterCallback(&_uart, HAL_UART_ABORT_COMPLETE_CB_ID, &ReceiverModule::cbRxAbortISR);
    HAL_UART_RegisterCallback(&_uart, HAL_UART_ABORT_RECEIVE_COMPLETE_CB_ID,
                              &ReceiverModule::cbRxAbortISR);

    HAL_TIM_RegisterCallback(&_tim, HAL_TIM_PERIOD_ELAPSED_CB_ID,
                             &ReceiverModule::cbPeriodElapsedISR);

    _frameSemphr = xSemaphoreCreateMutex();
}

ReceiverModule::~ReceiverModule()
{
    _instance = nullptr;
    vSemaphoreDelete(_frameSemphr);
}

void ReceiverModule::dispatch(uint32_t flags)
{
    // Reception timing out or abort due to error
    // force restart
    if ((flags & NOTIFY_ERROR) > 0)
    {
        // device might be still busy when timeout occurs
        if (_uart.RxState != HAL_UART_STATE_READY)
        {
            HAL_UART_AbortReceive(&_uart);
        }
        _synchronized = false;
        flags = NOTIFY_RX_START;
    }

    // process received data
    if ((flags & NOTIFY_RX_SUCCESSFUL) > 0)
    {
        if (xSemaphoreTake(_frameSemphr, MUTEX_TIMEOUT) == pdPASS)
        {
            const auto ret = SBUS::Decoder::decode(_rxBuffer);
            if (ret.first == SBUS::DecodeError::NoError)
            {
                testing_SuccessfulDecode();
                _decodedFrame = ret.second;
                _decodedFrame.lastUpdate = _hal.GetTick();
            }
            else
            {
                testing_ErrorDecode();
                // Decoding errors will show up as timeouts for state machine
                _log.logWarning(Logging::Origin::RadioControl, "Unable to decode");

                // force resync as we might have started mid frame
                _synchronized = false;
            }
            xSemaphoreGive(_frameSemphr);
        }
        else
        {
            _log.logError(Logging::Origin::RadioControl,
                          "Unable to aquire mutex to convert sbus frame");
        }
        flags = NOTIFY_RX_START;
    }

    if ((flags & NOTIFY_RX_START) > 0)
    {
        if (!_synchronized)
        {
            // Starting blocking reception with inbuilt timeout
            // timeout after a bit more than one frame to be sure we are within
            // inter frame dealy
            // this approach only works when inter frame delay is longer than frame time + 1
            // which it is :)
            HAL_UART_Receive(&_uart, _rxBuffer.data(), SBUS::Protocol::RAW_FRAME_SIZE,
                             (FrameTime_us / 1000) + 1);
            // logInfo(Logging::Origin::RadioControl, "Synchronized");
            _synchronized = true;
        }

        // timeout timer is still running in the background and might generate
        // timeout errors, clear those as these are false positives
        _task.notify(0, eNotifyAction::eSetValueWithOverwrite);

        if (HAL_UART_Receive_DMA(&_uart, _rxBuffer.data(), SBUS::Protocol::RAW_FRAME_SIZE) !=
            HAL_OK)
        {
            testing_RxRestart();
            _log.logError(Logging::Origin::RadioControl, "Starting RX DMA failed");
            _task.notify(NOTIFY_RX_START, eNotifyAction::eSetBits);
        }

        // restart timeout timer
        restartTimer();
    }
}

void ReceiverModule::restartTimer()
{
#ifdef BUILDCONFIG_EMBEDDED_BUILD
    __HAL_TIM_SET_COUNTER(&_tim, 0);
    __HAL_TIM_SET_AUTORELOAD(&_tim, ReceiverModule::TimerPeriod_Timeout);
    HAL_TIM_Base_Start_IT(&_tim);
#endif
}

void ReceiverModule::cbPeriodElapsedISR(TIM_HandleTypeDef *htim)
{
    finishISR(NOTIFY_ERROR);
}

bool ReceiverModule::getSBUSFrame(SBUS::Frame &frame)
{
    if (xSemaphoreTake(_frameSemphr, MUTEX_TIMEOUT) == pdPASS)
    {
        frame = _decodedFrame;
        xSemaphoreGive(_frameSemphr);
        return true;
    }
    return false;
}

void ReceiverModule::finishISR(uint32_t flags)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    ReceiverModule &instance = *ReceiverModule::_instance;

    instance._task.notifyFromISR(flags, eNotifyAction::eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // NOLINT
}

void ReceiverModule::cbRxAbortISR(UART_HandleTypeDef *huart)
{
    finishISR(NOTIFY_ERROR);
}

void ReceiverModule::cbRxCompleteISR(UART_HandleTypeDef *huart)
{
    finishISR(NOTIFY_RX_SUCCESSFUL);
}

void ReceiverModule::taskMain(void *inst)
{
    // initial dma startup
    uint32_t notifyValue = ReceiverModule::NOTIFY_RX_START;
    for (;;)
    {
        _instance->dispatch(notifyValue);
        _instance->_task.notifyWait(0, wrapper::Task::ClearAllBits, &notifyValue, portMAX_DELAY);
    }
}

} // namespace remote_control_device