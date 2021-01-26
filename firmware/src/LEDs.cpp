#include "LEDs.hpp"
#include <algorithm>
#include "SpecialAssert.hpp"

namespace remote_control_device
{
size_t LED::_instanceCount{0};
std::array<LED *, LED::LED_MAX_COUNT> LED::_instances{{}};

LED::LED(GPIO_TypeDef *gpio, const uint16_t pin, bool create)
    : _gpio(gpio), _pin(pin), _created{create}
{
    if (_instanceCount == 0)
    {
        std::fill(_instances.begin(), _instances.end(), nullptr);
    }
    if (create)
    {
        specialAssert(_instanceCount < LED_MAX_COUNT);
        _timer = xTimerCreate("LedTimer", pdMS_TO_TICKS(10), pdFALSE, nullptr, &LED::_dispatch);
        specialAssert(_timer != nullptr);

        _instances.at(_instanceCount) = this;
        _instanceCount++;
    }
    reset();
}

LED::~LED()
{
    // instance system is not designed to dispose of created timers
    // won't be reached in embedded context but should alert
    // when create is left true in testing
    specialAssert(!_created);
}

void LED::setMode(Mode mode)
{
    setMode(mode, 0);
}

void LED::setMode(Mode mode, uint8_t count)
{
    if (_mode == mode && _maxCount == count)
    {
        return;
    }
    reset();
    _mode = mode;
    _maxCount = count;
    if (count == 0 && mode == Mode::Counting)
    {
        _mode = Mode::Off;
    }
    dispatch();
}

void LED::_dispatch(TimerHandle_t xTimer)
{
    for(auto & inst : _instances) {
        if (inst == nullptr) {
            continue;
        }
        if (inst->_timer == xTimer) {
            inst->dispatch();
            return;
        }
    }
    specialAssert(false);
}

TickType_t LED::dispatch()
{
    TickType_t callAgainIn = pdMS_TO_TICKS(1);

    switch (_mode)
    {
        case Mode::Off:
            return 0;
        case Mode::On:
            HAL_GPIO_WritePin(_gpio, _pin, GPIO_PIN_SET);
            return 0;
        case Mode::FastBlink:
            HAL_GPIO_TogglePin(_gpio, _pin);
            callAgainIn = FAST_BLINK_INTERVAL;
            break;
        case Mode::SlowBlink:
            HAL_GPIO_TogglePin(_gpio, _pin);
            callAgainIn = SLOW_BLINK_INTERVAL;
            break;
        case Mode::Counting:
            if (_toggle)
            {
                HAL_GPIO_WritePin(_gpio, _pin, GPIO_PIN_RESET);
                _currCount++;
            }
            else
            {
                HAL_GPIO_WritePin(_gpio, _pin, GPIO_PIN_SET);
            }
            _toggle = !_toggle;

            if (_currCount >= _maxCount)
            {
                // finished last segment, restart
                _currCount = 0;
                _toggle = false;
                callAgainIn = COUNT_END_TIME;
            }
            else
            {
                callAgainIn = COUNT_BLINK_INTERVAL;
            }
            break;
    }

    if (_timer != nullptr)
    {
        xTimerChangePeriod(_timer, callAgainIn, 0);
    }

    return callAgainIn;
}

void LED::reset()
{
    HAL_GPIO_WritePin(_gpio, _pin, GPIO_PIN_RESET);
    _mode = Mode::Off;

    _maxCount = 0;
    _currCount = 0;
    _toggle = false;
}
} // namespace remote_control_device