#ifdef BUILDCONFIG_EMBEDDED_BUILD
#include "Application.hpp"
#include <cmsis_os2.h>

#include <memory>

// source of huart1, huart2
#include <usart.h>

// source of htim6
#include <tim.h>

// source of hcan
#include <can.h>

// source of hiwdg
#include <iwdg.h>

extern osThreadId_t ApplicationHandle;

namespace remote_control_device
{

Application::Application()
    : _hal(),                                                                               //
      _terminalIO(_hal, huart1),                                                            //
      _receiverModule(huart2, htim6, _hal, _terminalIO.getLogging()),                       //
      _canIO(hcan, _terminalIO.getLogging()),                                               //
      _cft(_hal, _terminalIO.getLogging()),                                                 //
      _canOpen(_canIO, _terminalIO.getLogging()),                                           //
      _remoteControl(_hal, _terminalIO.getLogging(), _receiverModule), _hardwareSwitches(), //
      _ledHw(ledHardware_GPIO_Port, ledHardware_Pin, true), // NOLINT
      _ledRc(ledRemote_GPIO_Port, ledRemote_Pin, true),     // NOLINT
      _ledUpdater(_ledHw, _ledRc, _canIO),                  //
      _stateMachine(_canOpen, _remoteControl, _hardwareSwitches, _ledUpdater, _terminalIO, _hal,
                    hiwdg, _cft) //
{
    _canIO.setCanopenInstance(_canOpen);
}

void Application::run()
{
    // canopen initialisation must preceede all object dictionary access
    wrapper::sync::signal(wrapper::sync::Application_Ready);

    // we are quite memory constrained so cft will run in this task instead of creating a new one
    // (cubemx always creates at least one task)
    _cft.taskMain();
}

} // namespace remote_control_device

extern "C" void startApplication(void *argument)
{
    auto app = std::make_unique<remote_control_device::Application>();
    wrapper::Task::registerTask(reinterpret_cast<TaskHandle_t>(ApplicationHandle));
    app->run();
    for (;;)
    {
    }
}

#endif