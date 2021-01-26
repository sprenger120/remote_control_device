#pragma once

/*
Blinking behaviour:

HardwareLED:
    - off/dim: device is starting
    - fast blinking:
        waiting for all canfestival nodes to start up
        no pdos will be sent until this is done
    - on: (no errors)
    - counting mode:
        - lower counts overwrite higher ones (lower = more important error)
        - 1 count: can network error
        - 2 count: brake heartbeat timeout
        - 3 count: steering timeout
        - 4 count: wheel motor timeout
RemoteLED:
    - on: remote control connection established (no errors)
    - fast blinking: manual mode (overwrites connection lost / established)
    - off: connection lost
*/

namespace remote_control_device
{
class LED;
class CanIO;
class StateChaningSources;

/**
 * @brief Handles both leds on the board, executes behaviour seen above
 *
 */
class LEDUpdater
{
public:
    /**
     * @brief Construct a new LEDUpdater object
     *
     * @param ledHw hardware led instance
     * @param ledRC rc led instance
     * @param canio canio peripheral driver
     */
    LEDUpdater(LED &ledHw, LED &ledRC, CanIO &canio);
    virtual ~LEDUpdater() = default;

    LEDUpdater(const LEDUpdater&) = delete;
    LEDUpdater(LEDUpdater&&) = delete;
    LEDUpdater& operator=(const LEDUpdater&) = delete;
    LEDUpdater& operator=(LEDUpdater&&) = delete;

    /**
     * @brief Updates blinking behaviour
     * 
     */
    virtual void update(StateChaningSources &);

private:
    LED &_ledHw;
    LED &_ledRC;
    CanIO &_canIO;
};
} // namespace remote_control_device