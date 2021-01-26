#pragma once
#include "Statemachine/State.hpp"
#include <array>

namespace remote_control_device
{

/**
 * @brief See States.cpp for state behaviour
 *
 */

using StatesArray = std::array<State, 7>;
class States
{
public:
    /**
     * @brief Performes various checks
     *
     */
    States();

    const StatesArray &get() const
    {
        return _states;
    }

    const State& getIdleState() const {
        return _idleState;
    };

    static constexpr uint8_t IDLE_STATE_PRIORITY = 0;
private:
    StatesArray _states;
    const State& _idleState;

    
    const State& _lookupIdleState() const;
};

} // namespace remote_control_device