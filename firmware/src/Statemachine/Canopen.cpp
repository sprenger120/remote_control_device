#include "Canopen.hpp"
#include "ANSIEscapeCodes.hpp"
#include "CanFestivalLocker.hpp"
#include "Logging.hpp"
#include "PeripheralDrivers/CanIO.hpp"
#include "PeripheralDrivers/TerminalIO.hpp"
#include <algorithm>


namespace remote_control_device
{

Canopen *Canopen::_instance = nullptr;
Message Canopen::RTD_StateBootupMessage = {
    Canopen::RPDO1_RTD_State, NOT_A_REQUEST, 1, {Canopen::RTD_State_Bootup, 0, 0, 0, 0, 0, 0, 0}};
Message Canopen::RTD_StateReadyMessage = {
    Canopen::RPDO1_RTD_State, NOT_A_REQUEST, 1, {Canopen::RTD_State_Ready, 0, 0, 0, 0, 0, 0, 0}};
Message Canopen::RTD_StateEmcyMessage = {Canopen::RPDO1_RTD_State,
                                         NOT_A_REQUEST,
                                         1,
                                         {Canopen::RTD_State_Emergency, 0, 0, 0, 0, 0, 0, 0}};
Canopen::Canopen(CanIO &canio, Logging &log, bool bypass)
    : _canIO(canio),
      _log(log), _monitoredDevices{{MonitoredDevice(BusDevices::DriveMotorController),
                                    MonitoredDevice(BusDevices::BrakeActuator),
                                    MonitoredDevice(BusDevices::BrakePressureSensor),
                                    MonitoredDevice(BusDevices::SteeringActuator),
                                    MonitoredDevice(BusDevices::SteeringAngleSensor)}},
      _stateControlledDevices{
          {StateControlledDevice(BusDevices::DriveMotorController, CanDeviceState::Operational),
           StateControlledDevice(BusDevices::BrakeActuator, CanDeviceState::Operational),
           StateControlledDevice(BusDevices::SteeringActuator, CanDeviceState::Operational),
           StateControlledDevice(BusDevices::BrakePressureSensor, CanDeviceState::Operational)}},
      _couplings{{Coupling(BusDevices::BrakeActuator, BrakeActuatorCoupling_IndexOD,
                           BrakeActuatorCoupling_SubIndexOD, BrakeActuatorCoupling_EngagedValue,
                           BrakeActuatorCoupling_DisEngagedValue),
                  Coupling(BusDevices::SteeringActuator, SteeringActuatorCoupling_IndexOD,
                           SteerinActuatorCoupling_SubIndexOD, SteerinActuatorCoupling_EngagedValue,
                           SteerinActuatorCoupling_DisEngagedValue)}}
{
    specialAssert(_instance == nullptr);
    _instance = this;
    if (bypass)
    {
        return;
    }
    {
        CFLocker locker;
        setNodeId(locker.getOD(), static_cast<UNS8>(BusDevices::RemoteControlDevice));

        // fix for REPEAT_NMT_MAX_NODE_ID_TIMES not having enough repeats by default thus not
        // initializing the NMT table correctly which makes post_SlaveStateChange miss bootup
        // events for nodes with higher ids
        for (auto &e : locker.getOD()->NMTable)
        {
            e = Unknown_state;
        }

        // set zero values to not have canfestival send pure zeros 0
        // which could cause damage
        setBrakeForce(0.0);
        setSteeringAngle(0.0);
        setWheelDriveTorque(0.0);
        setSelfState(StateId::Start);
        RTD_State = RTD_State_Bootup;
        setTPDO(TPDOIndex::SelfState, true);
        setActuatorPDOs(false);

        // setup callbacks
        locker.getOD()->heartbeatError = &cbHeartbeatError;
        locker.getOD()->post_SlaveStateChange = &cbSlaveStateChange;

        // avoids global reset sent out
        // preOperational callback is pre-programmed with
        // masterSendNMTstateChange (d, 0, NMT_Reset_Node)
        // by overwriting the callback we stop this from being sent
        locker.getOD()->preOperational = [](CO_Data *d) -> void {};

        setState(locker.getOD(), Initialisation);
        setState(locker.getOD(), Operational);

        // RX timeout requires a timer table that can't be generated and is initialized with NULL
        // every time adding our own
        // also the stack has a bug in timeout registration (since fixed)
        // where it reads from the wrong memory address and uses garbage for the timeout value
        std::fill(_rpdoTimers.begin(), _rpdoTimers.end(), TIMER_NONE);
        locker.getOD()->RxPDO_EventTimers = _rpdoTimers.data();

        // hack in rpdo timeout callback as it can't be registered anyhere
        locker.getOD()->RxPDO_EventTimers_Handler = [](CO_Data *d, UNS32 timerId) -> void {
            // remove reference to previously used timer
            // if not done, the next reception of this rpdo will clear the timer spot previously
            // used again even though it could be attributed to something completely different
            d->RxPDO_EventTimers[timerId] = TIMER_NONE; // NOLINT
            testHook_signalRTDTimeout();
            _instance->signalRTDTimeout();
        };

        // when a rpdo is received, the OD entry where RTD_State resides is updated, every entry
        // can have a callback so we use this cb to reset the timeout flag set by the rpdo timeout
        RegisterSetODentryCallBack(locker.getOD(), RTD_State_ODIndex, RTD_State_ODSubIndex,
                                   [](CO_Data *d, const indextable *, UNS8 bSubindex) -> UNS32 {
                                       testHook_signalRTDRecovery();
                                       _instance->signalRTDRecovery();
                                       return OD_SUCCESSFUL;
                                   });

        // rpdo timeout is a poorly supported feature in canopen
        // canfestival is also not capable to start the rpdo timeout unless at least
        // one message was received, so dispatch one fake message to get the timer going
        _canIO.addRXMessage(RTD_StateBootupMessage);
    }
}

Canopen::~Canopen()
{
    _instance = nullptr;
    CFLocker locker;
    locker.getOD()->RxPDO_EventTimers = nullptr;
}

bool Canopen::isDeviceOnline(const BusDevices device) const
{
    for (auto &dev : _monitoredDevices)
    {
        if (dev.device == device)
        {
            return !dev.disconnected;
        }
    }
    return false;
}

void Canopen::drawUIDevicesPart(TerminalIO &term)
{
    term.write(ANSIEscapeCodes::ColorSection_WhiteText_GrayBackground);
    term.write("\r\nMonitored Devices:\r\n");
    term.write(ANSIEscapeCodes::ColorSection_End);

    auto printMonitoredDevice = [&](const MonitoredDevice &dev) -> void {
        if (dev.disconnected)
        {
            term.write(ANSIEscapeCodes::ColorSection_WhiteText_RedBackground);
        }

        term.write(getBusDeviceName(dev.device));

        if (dev.disconnected)
        {
            term.write(": Offline");
            term.write(ANSIEscapeCodes::ColorSection_End);
        }
        else
        {
            term.write(": Online");
        }

        term.write("\r\n");
    };

    for (auto &dev : _monitoredDevices)
    {
        printMonitoredDevice(dev);
    }

    MonitoredDevice rtd(BusDevices::RealTimeDevice);
    rtd.disconnected = _rtdTimeout;
    printMonitoredDevice(rtd);

    term.write(ANSIEscapeCodes::ColorSection_WhiteText_GrayBackground);
    term.write("\r\nState Controlled Devices:\r\n");
    term.write(ANSIEscapeCodes::ColorSection_End);

    for (auto &scd : _stateControlledDevices)
    {
        if (scd.targetState != scd.currentState)
        {
            term.write(ANSIEscapeCodes::ColorSection_BlackText_YellowBackground);
        }

        term.write(getBusDeviceName(scd.device));
        term.write(": ");
        term.write(getCanDeviceStateName(scd.currentState));
        term.write(" (Target: ");
        term.write(getCanDeviceStateName(scd.targetState));
        term.write(")");

        if (scd.targetState != scd.currentState)
        {
            term.write(ANSIEscapeCodes::ColorSection_End);
        }
        term.write("\r\n");
    }
}

void Canopen::kickstartPDOTranmission()
{
    // setState(operational) calls sendPDOEvent which
    // causes the stack to register event timers for continous sending for all
    // enabled PDOs. As only self state is enabled by default
    // All other pdos get left behind and will not start transmitting
    // on their own until sendPDOEvent is called again.

    // Also: The stack has an oversight within pdo transmission in where there is no easy way
    // to force it to transmit a pdo with the same data continously. Every time a pdo tries to get
    // sent it is compared against the last one and if it is the same the function aborts. In this
    // case the event timers are not re-registered and the pdo is functionally broken You could do
    // something like this here below after every pdo transmission

    /*lock.getOD()->PDO_status[static_cast<size_t>(TPDOIndex::SelfState)].last_message.cob_id = 0;
    lock.getOD()->PDO_status[static_cast<size_t>(TPDOIndex::BrakeForce)].last_message.cob_id = 0;
    lock.getOD()->PDO_status[static_cast<size_t>(TPDOIndex::SteeringAngle)].last_message.cob_id = 0;
    lock.getOD()->PDO_status[static_cast<size_t>(TPDOIndex::MotorTorque)].last_message.cob_id = 0;
    lock.getOD()->PDO_status[static_cast<size_t>(TPDOIndex::TargetValues)].last_message.cob_id =
    0;*/

    // bus this is tedious as there is no callback to hook into after a pdo has fired.
    // So to avoid all this b.s. pdo.c was modified in line 539 to not compare at all.
    CFLocker lock;
    sendPDOevent(lock.getOD());
}

void Canopen::setSelfState(const StateId state)
{
    {
        CFLocker locker;
        SelfState = static_cast<UNS8>(state);
    }
}

void Canopen::setBrakeForce(float force)
{
    {
        CFLocker locker;
        BrakeTargetForce =
            mapValue<float, INTEGER16>(0.0f, 1.0f, BrakeForceRaw_Min, BrakeForceRaw_Max, force);
    }
}

void Canopen::setWheelDriveTorque(float torque)
{
    {
        CFLocker locker;
        WheelTargetTorque = mapValue<float, INTEGER16>(-1.0f, 1.0f, WheelDriveTorqueRaw_Min,
                                                       WheelDriveTorqueRaw_Max, torque);
    }
}

void Canopen::setSteeringAngle(float angle)
{
    // inverted due to hardware gearing
    angle *= -1.0f;
    {
        CFLocker locker;
        SteeringTargetAngle = mapValue<float, INTEGER32>(-1.0f, 1.0f, SteeringAngleRaw_Min,
                                                         SteeringAngleRaw_Max, angle);
    }
}

void Canopen::setCouplingStates(bool brake, bool steering)
{
    {
        CFLocker locker;
        _setCouplingState(_couplings[CouplingIndex_Brake], steering);
        _setCouplingState(_couplings[CouplingIndex_Steering], brake);
    }
}

void Canopen::setTPDO(const TPDOIndex index, bool enable)
{
    {
        CFLocker locker;
        if (enable)
        {
            PDOEnable(locker.getOD(), static_cast<UNS8>(index));
        }
        else
        {
            PDODisable(locker.getOD(), static_cast<UNS8>(index));
        }
    }
}

void Canopen::setActuatorPDOs(bool enable)
{
    setTPDO(TPDOIndex::BrakeForce, enable);
    setTPDO(TPDOIndex::SteeringAngle, enable);
    setTPDO(TPDOIndex::MotorTorque, enable);
    setTPDO(TPDOIndex::TargetValues, enable);

    if (enable)
    {
        kickstartPDOTranmission();
    }
}

void Canopen::update(BusDevicesState &target)
{
    {
        CFLocker locker;
        target.rtdEmergency = RTD_State == RTD_State_Emergency;
        target.rtdBootedUp = RTD_State != RTD_State_Bootup;
    }
    target.timeout = false;
    for (auto &e : _monitoredDevices)
    {
        target.timeout = target.timeout || e.disconnected;
    }
    target.timeout = target.timeout || _rtdTimeout;
}

void Canopen::cbSlaveStateChange(CO_Data *d, UNS8 heartbeatID, e_nodeState state)
{
    const auto dev = static_cast<BusDevices>(heartbeatID);
    // state controlled devices
    int8_t index{0};
    if ((index = _instance->findInStateControlledList(dev)) != -1)
    {
        {
            CFLocker lock;

            // interpret node state to check if device is in correct state
            CanDeviceState convertedState = CanDeviceState::Unknown;
            switch (state)
            {
                case e_nodeState::Operational:
                    convertedState = CanDeviceState::Operational;
                    break;
                case e_nodeState::Pre_operational:
                    convertedState = CanDeviceState::Preoperational;
                    break;
                case e_nodeState::Initialisation:
                /* fall through */
                case e_nodeState::Disconnected:
                /* fall through */
                case e_nodeState::Connecting:
                /* fall through */
                // case Preparing: has same value as connecting
                /* fall through */
                case e_nodeState::Stopped:
                /* fall through */
                case e_nodeState::Unknown_state:
                /* fall through */
                default:
                    break;
            }
            _instance->_stateControlledDevices[index].currentState = convertedState;

            _instance->_log.logInfo(Logging::Origin::BusDevices, "%s changed state to %s", getBusDeviceName(dev),
                    getCanDeviceStateName(convertedState));

            if (convertedState != _instance->_stateControlledDevices[index].targetState)
            {
                _instance->setDeviceState(
                    dev, _instance->_stateControlledDevices[index].targetState);
            }
        }
    }

    // reset disconnected state
    for (MonitoredDevice &e : _instance->_monitoredDevices)
    {
        if (e.device == dev && e.disconnected)
        {
            _instance->_log.logInfo(Logging::Origin::BusDevices, "%s is online", getBusDeviceName(dev));
            e.disconnected = false;
            return;
        }
    }
}

void Canopen::cbHeartbeatError(CO_Data *d, UNS8 heartbeatID)
{
    // Called when a node registered in the heartbeat consumer od index
    // didn't send a heartbeat within the timeout range
    // after timeout the node's state is changed to disconnected internally
    // when the timed out node recovers,  slave state change callback is called
    const auto dev = static_cast<BusDevices>(heartbeatID);
    _instance->_log.logInfo(Logging::Origin::BusDevices, "%s timed out", getBusDeviceName(dev));

    // Reset internal current state when device is state monitored
    // so UI doesn't show wrong information
    int8_t index = 0;
    if ((index = _instance->findInStateControlledList(dev)) != -1)
    {
        _instance->_stateControlledDevices[index].currentState = CanDeviceState::Unknown;
    }

    for (MonitoredDevice &e : _instance->_monitoredDevices)
    {
        if (e.device == dev)
        {
            e.disconnected = true;
            return;
        }
    }
    _instance->_log.logWarning(Logging::Origin::BusDevices,
               "Received heartbeat error callback for nodeId %d but it isn't registered as a "
               "monitored device",
               heartbeatID);
}

void Canopen::cbSDO(CO_Data *d, UNS8 nodeId)
{
    // search for coupling with nodeid
    Coupling *coupling = nullptr;
    for (Coupling &e : _instance->_couplings)
    {
        if (e.device == static_cast<BusDevices>(nodeId))
        {
            coupling = &e;
            break;
        }
    }

    if (coupling == nullptr)
    {
        _instance->_log.logDebug(Logging::Origin::BusDevices, "Unexpected SDO received from nodeId %d", nodeId);
        return;
    }

    bool restart = false;
    uint32_t abortCode = 0;
    if (getWriteResultNetworkDict(d, nodeId, &abortCode) != SDO_FINISHED)
    {
        // transfer failed
        // abortCode is 0 for timeout which isn't correct
        // but flow errors in the stack cause this to be 0
        if (abortCode == 0)
        {
            // timeout, try again
            restart = true;
        }
        else
        {
            // serious error, most likely ill configured node
            // not attempting new transmission as these errors are more likely
            // to come from an active node which answers quickly (like we do)
            // causing risk of flooding
            _instance->_log.logWarning(Logging::Origin::BusDevices, "SDO to nodeId %d failed (%s)", nodeId,
                       abortCodeToString(abortCode));
            restart = false;
        }
    }
    else
    {
        // sucess but check if target changed mid request processing
        if (coupling->stateForThisRequest != coupling->targetState)
        {
            _instance->_log.logDebug(Logging::Origin::BusDevices,
                     "SDO finished successfully but state changed mid transmission");
            restart = true;
        }
        else
        {
            _instance->_log.logDebug(Logging::Origin::BusDevices, "SDO finished successfully");
        }
    }
    // closing isn't necessary when the transfer is finished but this isn't always the case
    closeSDOtransfer(d, nodeId, SDO_CLIENT);

    if (restart)
    {
        // logDebug(Logging::Origin::BusDevices, "Repeating transmission");
        _instance->_setCouplingState(*coupling, coupling->targetState);
    }
}

const char *Canopen::abortCodeToString(uint32_t abortCode)
{
    switch (abortCode)
    {
       /* case OD_SUCCESSFUL:
            return "OD_SUCCESSFUL";
        case OD_READ_NOT_ALLOWED:
            return "OD_READ_NOT_ALLOWED";
        case OD_WRITE_NOT_ALLOWED:
            return "OD_WRITE_NOT_ALLOWED";
        case OD_NO_SUCH_OBJECT:
            return "OD_NO_SUCH_OBJECT";
        case OD_NOT_MAPPABLE:
            return "OD_NOT_MAPPABLE";
        case OD_ACCES_FAILED:
            return "OD_ACCES_FAILED";
        case OD_LENGTH_DATA_INVALID:
            return "OD_LENGTH_DATA_INVALID";
        case OD_NO_SUCH_SUBINDEX:
            return "OD_NO_SUCH_SUBINDEX";
        case OD_VALUE_RANGE_EXCEEDED:
            return "OD_VALUE_RANGE_EXCEEDED";
        case OD_VALUE_TOO_LOW:
            return "OD_VALUE_TOO_LOW";
        case OD_VALUE_TOO_HIGH:
            return "OD_VALUE_TOO_HIGH";
        case SDOABT_TOGGLE_NOT_ALTERNED:
            return "SDOABT_TOGGLE_NOT_ALTERNED";
        case SDOABT_TIMED_OUT:
            return "SDOABT_TIMED_OUT";
        case SDOABT_CS_NOT_VALID:
            return "SDOABT_CS_NOT_VALID";
        case SDOABT_INVALID_BLOCK_SIZE:
            return "SDOABT_INVALID_BLOCK_SIZE";
        case SDOABT_OUT_OF_MEMORY:
            return "SDOABT_OUT_OF_MEMORY";
        case SDOABT_GENERAL_ERROR:
            return "SDOABT_GENERAL_ERROR";
        case SDOABT_LOCAL_CTRL_ERROR:
            return "SDOABT_LOCAL_CTRL_ERROR";*/
        default:
            return "Error Unknown";
    }
}

int8_t Canopen::findInStateControlledList(const BusDevices device)
{
    for (uint8_t i = 0; i < _stateControlledDevices.size(); ++i)
    {
        if (_stateControlledDevices[i].device == device)
        {
            return i;
        }
    }
    return -1;
}

void Canopen::setDeviceState(const BusDevices device, CanDeviceState state)
{
    // check if allowed to change state
    int8_t index = findInStateControlledList(device);
    if (index == -1)
    {
        _instance->_log.logWarning(Logging::Origin::BusDevices,
                   "Denied request to change state of unlisted device with nodeId %d",
                   static_cast<uint8_t>(device));
        return;
    }

    uint8_t nmtCommand = 0;
    switch (state)
    {
        case CanDeviceState::Operational:
            nmtCommand = NMT_Start_Node;
            break;
        case CanDeviceState::Preoperational:
            nmtCommand = NMT_Enter_PreOperational;
            break;
        case CanDeviceState::Unknown:
        /* fall through */
        default:
            return;
    }

    _instance->_log.logInfo(Logging::Origin::BusDevices, "Requesting %s to change status to %s",
            getBusDeviceName(device), getCanDeviceStateName(state));
    _stateControlledDevices[index].targetState = state;
    {
        CFLocker locker;
        masterSendNMTstateChange(locker.getOD(),
                                 static_cast<uint8_t>(_stateControlledDevices[index].device),
                                 nmtCommand);
        // masterSendNMTstateChange is just blindly transmitting the state change request
        // when a node doesn't switch it isn't noticed as proceedNODE_GUARD which processes incoming
        // heartbeats compares the old state with the unchanged newly received one and finds no
        // difference
        // invalidating the local state of a node will force the change state callback to be fired
        // which allows confirming the state change or retrying
        locker.getOD()->NMTable[static_cast<UNS8>(_stateControlledDevices[index].device)] =
            Disconnected;
    }
}

const char *Canopen::getBusDeviceName(const BusDevices dev)
{
    switch (dev)
    {
        case BusDevices::DriveMotorController:
            return "Drive Motor Controller (10h)";
        case BusDevices::BrakeActuator:
            return "Brake Actuator (20h)";
        case BusDevices::SteeringActuator:
            return "Steering Actuator (30h)";
        case BusDevices::RemoteControlDevice:
            return "Remote Control Device (1h)";
        case BusDevices::RealTimeDevice:
            return "Real Time Device (2h)";
        case BusDevices::WheelSpeedSensor:
            return "Wheel Speed Sensor (11h)";
        case BusDevices::BrakePressureSensor:
            return "Brake Pressure Sensor (21h)";
        case BusDevices::SteeringAngleSensor:
            return "Steering Angle Sensor (31h)";
        default:
            return "Unamed device";
    }
}

Canopen::Coupling::Coupling(BusDevices device, uint16_t odIndex, uint8_t odSubIndex,
                            uint32_t engagedValue, uint32_t disengagedValue)
    : device(device), odIndex(odIndex), odSubIndex(odSubIndex), engagedValue(engagedValue),
      disengagedValue(disengagedValue)
{
}

void Canopen::_setCouplingState(Coupling &coupling, bool state)
{
    coupling.targetState = state;

    // check if sdo is still in porgress
    {
        CFLocker locker;
        UNS32 abortCode = 0;
        if (getWriteResultNetworkDict(locker.getOD(), static_cast<UNS8>(coupling.device),
                                      &abortCode) == SDO_ABORTED_INTERNAL)
        {
            // nothing in progress, start write request
            coupling.stateForThisRequest = coupling.targetState;
            uint32_t couplingState =
                coupling.stateForThisRequest ? coupling.engagedValue : coupling.disengagedValue;
            writeNetworkDictCallBack(locker.getOD(), static_cast<UNS8>(coupling.device),
                                     coupling.odIndex, coupling.odSubIndex, 4, uint32,
                                     &couplingState, &Canopen::cbSDO, false);
        }
        else
        {
            _instance->_log.logInfo(Logging::Origin::BusDevices,
                    "SDO transfer already in progress, repeating after this one finished");
        }
    }
}

std::array<Canopen::BusDevices, Canopen::MonitoredDeviceCount> Canopen::getMonitoredDevices() const
{
    std::array<Canopen::BusDevices, Canopen::MonitoredDeviceCount> list;
    for (uint8_t i = 0; i < _monitoredDevices.size(); ++i)
    {
        list[i] = _monitoredDevices[i].device;
    }
    return list;
}

std::array<Canopen::BusDevices, Canopen::StateControlledDeviceCount>
Canopen::getStateControlledDevices() const
{
    std::array<Canopen::BusDevices, Canopen::StateControlledDeviceCount> list;
    for (uint8_t i = 0; i < _stateControlledDevices.size(); ++i)
    {
        list[i] = _stateControlledDevices[i].device;
    }
    return list;
}
} // namespace remote_control_device