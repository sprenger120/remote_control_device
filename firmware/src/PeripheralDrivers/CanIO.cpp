#include "CanIO.hpp"
#include "BuildConfiguration.hpp"
#include "CanFestivalLocker.hpp"
#include "Logging.hpp"
#include "SpecialAssert.hpp"
#include "Wrapper/Sync.hpp"
#include <cmsis_os.h>
#include <limits>
#include <task.h>

namespace remote_control_device
{
CanIO *CanIO::_instance{nullptr};

CanIO::CanIO(CAN_HandleTypeDef &can, Logging &log)
    : _can(can), _log(log),
      _task(&CanIO::taskMain, "CanIO", StackSize, reinterpret_cast<void *>(this),
            osPriority_t::osPriorityHigh, wrapper::sync::CanIO_Ready)

{
    specialAssert(_instance == nullptr);
    _instance = this;

    _txQueue = xQueueCreate(QUEUES_SIZE, sizeof(Message));
    _rxQueue = xQueueCreate(QUEUES_SIZE, sizeof(Message));
    // Tx
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_TX_MAILBOX0_COMPLETE_CB_ID,
                             &CanIO::cbTxMailboxCompleteISR);
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_TX_MAILBOX1_COMPLETE_CB_ID,
                             &CanIO::cbTxMailboxCompleteISR);
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_TX_MAILBOX2_COMPLETE_CB_ID,
                             &CanIO::cbTxMailboxCompleteISR);

    // Rx
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID, &CanIO::cbRxMsgPending0ISR);
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID, &CanIO::cbRxMsgPending1ISR);

    // Error
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_ERROR_CB_ID, &CanIO::cbErrorISR);

    // Overload
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_RX_FIFO0_FULL_CB_ID, &CanIO::cbOverloadISR);
    HAL_CAN_RegisterCallback(&_can, HAL_CAN_RX_FIFO1_FULL_CB_ID, &CanIO::cbOverloadISR);

    CAN_FilterTypeDef conf;
    static constexpr uint8_t BankCount = 13;
    for (uint8_t i = 0; i < BankCount; ++i)
    {
        // mask filter that lets everythign through
        conf.FilterIdHigh = 0;
        conf.FilterIdLow = 0;
        conf.FilterMaskIdHigh = 0;
        conf.FilterMaskIdLow = 0;
        conf.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        conf.FilterBank = i;
        conf.FilterMode = CAN_FILTERMODE_IDMASK;   // CAN_filter_mode
        conf.FilterScale = CAN_FILTERSCALE_32BIT;  // CAN_filter_scale
        conf.FilterActivation = CAN_FILTER_ENABLE; // CAN_filter_activation
        HAL_CAN_ConfigFilter(&_can, &conf);
    }

    // Due to a bug in cubemx code generation, some can interrupts are not started
    static constexpr uint32_t CanIOInterruptPriority = 13;
    static constexpr uint32_t CanIOInterruptSubPriority = 0;
    HAL_NVIC_SetPriority(USB_HP_CAN_TX_IRQn, CanIOInterruptPriority, CanIOInterruptSubPriority);
    HAL_NVIC_EnableIRQ(USB_HP_CAN_TX_IRQn);
    HAL_NVIC_SetPriority(USB_LP_CAN_RX0_IRQn, CanIOInterruptPriority, CanIOInterruptSubPriority);
    HAL_NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN_RX1_IRQn, CanIOInterruptPriority, CanIOInterruptSubPriority);
    HAL_NVIC_EnableIRQ(CAN_RX1_IRQn);

    HAL_CAN_ActivateNotification(&_can, CAN_IT_RX_FIFO1_MSG_PENDING);
    HAL_CAN_ActivateNotification(&_can, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(&_can, CAN_IT_ERROR);
    HAL_CAN_ActivateNotification(&_can, CAN_IT_RX_FIFO1_OVERRUN);
    HAL_CAN_ActivateNotification(&_can, CAN_IT_RX_FIFO0_OVERRUN);
    HAL_CAN_ActivateNotification(&_can, CAN_IT_TX_MAILBOX_EMPTY);

    HAL_CAN_Start(&_can);
}

CanIO::~CanIO()
{
    _instance = nullptr;
}

void CanIO::dispatch(uint32_t flags)
{
    /* Sending queued frames */
    if ((flags & CanIO::NOTIFY_ATTEMPT_TX) > 0)
    {
        volatile uint8_t freeMailboxes = HAL_CAN_GetTxMailboxesFreeLevel(&_can);
        uint8_t filledMailboxes = 0;

        for (uint8_t i = 0; i < freeMailboxes; ++i)
        {
            Message m;
            BaseType_t ret = xQueueReceive(_txQueue, reinterpret_cast<void *>(&m), 0);
            if (ret == errQUEUE_EMPTY)
            {
                break;
            }

            CAN_TxHeaderTypeDef header;
            header.StdId = m.cob_id;
            header.IDE = CAN_ID_STD;
            if (m.rtr)
            {
                header.RTR = CAN_RTR_REMOTE;
            }
            else
            {
                header.RTR = CAN_RTR_DATA;
            }
            header.DLC = m.len;
            header.TransmitGlobalTime = DISABLE;

            uint32_t targetMailbox = 0;
            volatile HAL_StatusTypeDef status =
                HAL_CAN_AddTxMessage(&_can, &header, m.data, &targetMailbox);
            if (status != HAL_StatusTypeDef::HAL_OK)
            {
                testing_TxFailed();
                _task.notify(CanIO::NOTIFY_ATTEMPT_TX, eNotifyAction::eSetBits);
                _log.logError(Logging::Origin::CanIO, "Unable to send frame with COBId %d",
                              m.cob_id);
            }
            filledMailboxes++;
        }

        // isr fires continuously when there are free mailboxes
        // no use in using up cpu with this when there is little traffic
        if (filledMailboxes == 3)
        {
            HAL_CAN_ActivateNotification(&_can, CAN_IT_TX_MAILBOX_EMPTY);
        }
        else
        {
            HAL_CAN_DeactivateNotification(&_can, CAN_IT_TX_MAILBOX_EMPTY);
        }
    }

    /* dispatch received frames to canfestival */
    if ((flags & CanIO::NOTIFY_RX_PENDING) > 0)
    {
        specialAssert(_canopen != nullptr);
        testing_RxAttempt();
        if (!_busOK)
        {
            _busOK = true;
            _log.logInfo(Logging::Origin::CanIO, "Bus connection recovered");
        }
        {
            CFLocker locker;
            Message m;
            while (xQueueReceive(_rxQueue, reinterpret_cast<void *>(&m), 0) != errQUEUE_EMPTY)
            {
                canDispatch(locker.getOD(), &m);
            }
        }
    }

    if ((flags & CanIO::NOTIFY_ERROR) > 0)
    {
        testing_Error();
        _busOK = false;
        _log.logError(Logging::Origin::CanIO, "Bus connection failed!");
    }

    if ((flags & CanIO::NOTIFY_OVERLOAD) > 0)
    {
        testing_Overload();
        _log.logWarning(Logging::Origin::CanIO, "RX overload");
    }
}

void CanIO::canSend(Message *m)
{
#ifndef BUILDCONFIG_FUZZING_BUILD
    if (xQueueSendToBack(_txQueue, m, QUEUE_WAITTIME) != pdPASS)
    {
        _log.logWarning(Logging::Origin::CanIO, "TX queue full, frame dropped");
    }
    _task.notify(CanIO::NOTIFY_ATTEMPT_TX, eNotifyAction::eSetBits);
#endif
}

void CanIO::_canSend(Message *m)
{
    specialAssert(CanIO::_instance != nullptr);
    CanIO &inst = *CanIO::_instance;
    inst.canSend(m);
}

void CanIO::addRXMessage(Message &m)
{
    if (xQueueSendToBack(_rxQueue, &m, QUEUE_WAITTIME) != pdPASS)
    {
        _log.logWarning(Logging::Origin::CanIO, "RX queue full, frame dropped");
    }
    _task.notify(CanIO::NOTIFY_RX_PENDING, eNotifyAction::eSetBits);
}

void CanIO::retrieveMessageFromISR(CanIO &canio, CAN_HandleTypeDef *hcan, uint32_t mailboxNmbr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    CAN_RxHeaderTypeDef header;
    Message m = Message_Initializer;
    if (HAL_CAN_GetRxMessage(hcan, mailboxNmbr, &header, m.data) == HAL_OK) // NOLINT
    {
        if (header.IDE != CAN_ID_STD)
        {
            return;
        }
        m.cob_id = header.StdId;
        m.rtr = header.RTR == CAN_RTR_REMOTE;
        m.len = header.DLC;
        xQueueSendToBackFromISR(canio._rxQueue, &m, &xHigherPriorityTaskWoken);
    }
}

void CanIO::cbTxMailboxCompleteISR(CAN_HandleTypeDef *hcan)
{
    finishCallback(CanIO::NOTIFY_ATTEMPT_TX);
}

void CanIO::cbRxMsgPending1ISR(CAN_HandleTypeDef *hcan)
{
    CanIO &inst = *CanIO::_instance;
    retrieveMessageFromISR(inst, hcan, CAN_RX_FIFO1);
    finishCallback(CanIO::NOTIFY_RX_PENDING);
}

void CanIO::cbRxMsgPending0ISR(CAN_HandleTypeDef *hcan)
{
    CanIO &inst = *CanIO::_instance;
    retrieveMessageFromISR(inst, hcan, CAN_RX_FIFO0);
    finishCallback(CanIO::NOTIFY_RX_PENDING);
}

void CanIO::cbErrorISR(CAN_HandleTypeDef *hcan)
{
    finishCallback(CanIO::NOTIFY_ERROR);
}

void CanIO::cbOverloadISR(CAN_HandleTypeDef *hcan)
{
    finishCallback(CanIO::NOTIFY_OVERLOAD);
}

void CanIO::finishCallback(uint32_t flag)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    CanIO &inst = *CanIO::_instance;
    inst._task.notifyFromISR(flag, eNotifyAction::eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // NOLINT
}

bool CanIO::isBusOK()
{
    return _busOK;
}

void CanIO::taskMain(void *parameters)
{
    uint32_t notifyValue = 0;
    for (;;)
    {
        _instance->_task.notifyWait(0, wrapper::Task::ClearAllBits, &notifyValue, portMAX_DELAY);
        _instance->dispatch(notifyValue);
    }
}
} // namespace remote_control_device

extern "C" unsigned char canSend(CAN_PORT notused __attribute__((unused)), Message *m)
{
    remote_control_device::CanIO::_canSend(m);
    return 0;
}