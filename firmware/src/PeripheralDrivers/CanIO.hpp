#pragma once
#include "Wrapper/Task.hpp"
#include <queue.h>

extern "C"
{
#include <canfestival/can.h>
}

#include <stm32f3xx_hal.h>

/**
 * @brief Processes incoming and outgoing data from the can perihperal
 * Dispatches data to canfestival stack
 *
 */

namespace remote_control_device
{
class Logging;
class Canopen;

class CanIO
{
public:
    /**
     * @brief Initializes queues and registers can peripheral hooks
     *
     * @param task Task handle to use for internal processes
     * @param hCan can perihperal handle for internal processes
     */
    static constexpr uint16_t StackSize = 300;
    CanIO(CAN_HandleTypeDef &hCan, Logging &log);
    virtual ~CanIO();

    /**
     * @brief Set the Canopen Instance object
     * Both classes depend on another but canIO only needs
     * it after everything is brought up
     * 
     * @param inst 
     */
    virtual void setCanopenInstance(Canopen& inst) {
        _canopen = &inst;
    }

    CanIO(const CanIO &) = delete;
    CanIO(CanIO &&) = delete;
    CanIO &operator=(const CanIO &) = delete;
    CanIO &operator=(CanIO &&) = delete;

    /**
     * @brief Sends TX frames, dispatches RX frames to canfestival, handles errors
     *
     * @param flags what event happened
     */
    virtual void dispatch(uint32_t flags);
    static constexpr uint8_t NOTIFY_ATTEMPT_TX = 1 << 0;
    static constexpr uint8_t NOTIFY_RX_PENDING = 1 << 1;
    static constexpr uint8_t NOTIFY_ERROR = 1 << 2;
    static constexpr uint8_t NOTIFY_OVERLOAD = 1 << 3;
    static void retrieveMessageFromISR(CanIO &canio, CAN_HandleTypeDef *hcan, uint32_t mailboxNmbr);

    /**
     * @brief Retturns true if bus communication works as intendet
     *
     * @return true
     * @return false
     */
    virtual bool isBusOK();

    /**
     * @brief Adds a can frame to the TX queue
     *
     */
    virtual void canSend(Message *);
    static void _canSend(Message*);

    /**
     * @brief Used for canfestival initialisation as calling canDispatch in Statemachine task
     * consumes too much stack that is usually not used there.
     *
     * @param m
     */
    virtual void addRXMessage(Message &m);

    /**
     * @brief Queue size for TX and RX queue
     * increase when overload errors start appearing
     *
     */
    static constexpr UBaseType_t QUEUES_SIZE = 16;

    /**
     * @brief How long a canSend call should wait for a slot in the TX queue
     * increase for high bus load
     *
     */
    static constexpr TickType_t QUEUE_WAITTIME = pdMS_TO_TICKS(10);

    // Some functions to mock in tests
    static void testing_TxFailed() {};
    static void testing_RxAttempt() {};
    static void testing_Overload() {};
    static void testing_Error() {};

private:
    CAN_HandleTypeDef &_can;
    Logging &_log;
    wrapper::Task _task;
    Canopen* _canopen{nullptr};

    static CanIO *_instance;
    QueueHandle_t _rxQueue, _txQueue;
    bool _busOK = true;

    /* Hooks and support functions */
    static void cbTxMailboxCompleteISR(CAN_HandleTypeDef *hcan);
    static void cbRxMsgPending0ISR(CAN_HandleTypeDef *hcan);
    static void cbRxMsgPending1ISR(CAN_HandleTypeDef *hcan);
    static void cbErrorISR(CAN_HandleTypeDef *hcan);
    static void cbOverloadISR(CAN_HandleTypeDef *hcan);

    /**
     * @brief Notifys canIO task about 'flag'. Attempts deferred processing
     *
     * @param flag
     */
    static void finishCallback(uint32_t flag);

    static void taskMain(void* parameter);
};
} // namespace remote_control_device