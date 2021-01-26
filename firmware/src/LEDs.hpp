#pragma once
#include <FreeRTOS.h>
#include <array>
#include <stm32f3xx_hal.h>
#include <timers.h>
#include <utility>

namespace remote_control_device
{
/**
 * @brief Controls the behaviour of one led. Use getLED... functions.
 *
 */

class LED
{
public:
    /**
     * @brief Construct a new LED object
     *
     * @param gpio gpio port where led is connected
     * @param pin pin where led is connected
     */
    LED(GPIO_TypeDef *gpio, const uint16_t pin, bool create = true);
    virtual ~LED();

    LED(const LED &) = default;
    LED(LED &&) = delete;
    LED &operator=(const LED &) = delete;
    LED &operator=(LED &&) = delete;

    /**
     * @brief Different modes available
     * Everything should be self explainatory except counting.
     * As this device doesn't have a display to show error numbers
     * the number of blinks must be counted to get the error code.
     * - off
     * * on
     *
     * ---***---***--------- ---*** ... etc.
     *
     * Counting mode showing error two ^. The long off time at the end signals
     * that you should stop counting.
     *
     */
    enum class Mode : uint8_t
    {
        On,
        Off,
        FastBlink,
        SlowBlink,
        Counting
    };

    /**
     * @brief Sets the current led mode
     *
     * @param mode see above
     * @param count error code for counting mode
     */
    virtual void setMode(const Mode mode);
    virtual void setMode(const Mode mode, const uint8_t count);

    /**
     * @brief Internal use
     *
     */
    virtual TickType_t dispatch();
    static void _dispatch(TimerHandle_t xTimer);

    /**
     * @brief Timings for the various modes
     *
     */
    static constexpr TickType_t SLOW_BLINK_INTERVAL = pdMS_TO_TICKS(750);
    static constexpr TickType_t FAST_BLINK_INTERVAL = pdMS_TO_TICKS(50);

    static constexpr TickType_t COUNT_BLINK_INTERVAL = pdMS_TO_TICKS(500);
    static constexpr TickType_t COUNT_END_TIME = pdMS_TO_TICKS(2000);

private:
    static constexpr uint8_t LED_MAX_COUNT = 2;
    static size_t _instanceCount;
    static std::array<LED *, LED_MAX_COUNT> _instances;

    GPIO_TypeDef *_gpio;
    const uint16_t _pin;
    TimerHandle_t _timer{nullptr};
    bool _created{false};

    Mode _mode{Mode::Off};

    uint8_t _maxCount{0};
    uint8_t _currCount{0};
    bool _toggle{false};

    void reset();
};

} // namespace remote_control_device