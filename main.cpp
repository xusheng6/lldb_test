#include <iostream>
#include <thread>
#include <cinttypes>
#include <sstream>
#include "lldb/API/LLDB.h"

using namespace std;
using namespace lldb;

uint64_t m_start = 0;
uint64_t m_entryPoint = 0x1080;


struct ModuleNameAndOffset
{
    // TODO: maybe we should use DebugModule instead of its name
    // Update: We are not using a DebugModule here because the base address information of it can be outdated;
    // instead, we only keep a name and an offset.
    std::string module;
    uint64_t offset;

    ModuleNameAndOffset() : module(""), offset(0) {}
    ModuleNameAndOffset(std::string mod, uint64_t off) : module(mod), offset(off) {}
};


std::string InvokeBackendCommand(SBDebugger& m_debugger, const std::string& command)
{
    SBCommandInterpreter interpreter = m_debugger.GetCommandInterpreter();
    SBCommandReturnObject commandResult;
    interpreter.HandleCommand(command.c_str(), commandResult);

    std::string result;
    if (commandResult.GetOutputSize() > 0)
        result += commandResult.GetOutput();

    if (commandResult.GetErrorSize() > 0)
        result += commandResult.GetError();

    return result;
}


bool AddBreakpoint(SBDebugger& debugger, const ModuleNameAndOffset& address)
{
    uint64_t addr = address.offset + m_start;
    std::stringstream stream;
    stream << "b -s \"" << address.module << "\" -a 0x" << std::hex << addr;
    std::string entryBreakpointCommand = stream.str();
    auto ret = InvokeBackendCommand(debugger, entryBreakpointCommand);

    return true;
}


void EventListener(SBDebugger& m_debugger)
{
//    SBListener listener = SBListener("listener");
//    listener.StartListeningForEventClass(m_debugger, SBProcess::GetBroadcasterClassName(),
//                                         lldb::SBProcess::eBroadcastBitStateChanged);

    auto listener = m_debugger.GetListener();

    bool done = false;
    while (!done)
    {
        SBEvent event;
//        cout << "wait for event" << endl;
        if (!listener.WaitForEvent(1, event))
            continue;

        uint32_t event_type = event.GetType();
//        cout << "got an event, type " << event_type << endl;
        if (lldb::SBProcess::EventIsProcessEvent(event))
        {
            SBProcess process = lldb::SBProcess::GetProcessFromEvent(event);
            if (event_type & lldb::SBProcess::eBroadcastBitStateChanged)
            {
                // This can solve the problem that if the user resumes/steps the target from the console,
                // the UI is not updated. However, in order to receive the eBroadcastBitStateChanged notification,
                // We need to turn on async mode, which requires other changes as well.

                StateType state = SBProcess::GetStateFromEvent(event);
                switch (state)
                {
                    case lldb::eStateRunning:
                    {
                        cout << "lldb::eStateRunning" << endl;
                        break;
                    }
                        // LLDB seems to always report eStateRunning instead of eStateStepping
                    case lldb::eStateStepping:
                    {
                        cout << "lldb::eStateStepping" << endl;
                        break;
                    }
                    case lldb::eStateStopped:
                    {
                        cout << "lldb::eStateStopped" << endl;
                        break;
                    }
                    case lldb::eStateExited:
                    {
                        cout << "lldb::eStateExited" << endl;
                        done = true;
                        break;
                    }
                    case lldb::eStateDetached:
                    {
                        cout << "lldb::eStateDetached" << endl;
                        done = true;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

//    listener.StopListeningForEventClass(m_debugger, SBProcess::GetBroadcasterClassName(),
//                                        lldb::SBProcess::eBroadcastBitStateChanged);
//
//    listener.Clear();
}


bool ExecuteWithArgs(SBDebugger& m_debugger, const std::string& path)
{
    m_debugger.SetAsync(false);

    SBError err;

    auto m_target = m_debugger.CreateTarget(path.c_str(), "", "", true, err);

    if (!m_target.IsValid())
    {
        return false;
    }

    std::thread thread([&]() { EventListener(m_debugger); });
    thread.detach();

    AddBreakpoint(m_debugger, ModuleNameAndOffset(path, m_entryPoint - m_start));

    std::string launchCommand = "process launch";

    auto result = InvokeBackendCommand(m_debugger, launchCommand);

    auto m_process = m_target.GetProcess();
    if (!m_process.IsValid() || (m_process.GetState() != StateType::eStateStopped) || (result.rfind("error: ", 0) == 0))
    {
        return false;
    }
    m_debugger.SetAsync(true);
    return true;
}


bool StepInto(SBDebugger& m_debugger)
{
    auto m_process = m_debugger.GetTargetAtIndex(0).GetProcess();
    if (m_process.GetState() != lldb::eStateStopped)
    {
        cout << "step into failed" << endl;
        return false;
    }

    SBThread thread = m_process.GetSelectedThread();
    if (!thread.IsValid())
    {
        cout << "step into failed, invalid thread" << endl;
        return false;
    }

    SBError error;
    thread.StepInstruction(false, error);
    if (!error.Success())
    {
        cout << "step into failed " << string(error.GetCString() ? error.GetCString() : "") << endl;
        return false;
    }
    cout << "step into requested" << endl;
    return true;
}


int main(int argc, char** argv)
{
    if (argc < 2)
        cout << "pass in executable path" << endl;

    const char* executable = argv[1];

    SBDebugger::Initialize();
    auto m_debugger = SBDebugger::Create();
    if (!m_debugger.IsValid())
        cout << "Invalid debugger" << endl;

    m_debugger.SetAsync(false);

    ExecuteWithArgs(m_debugger, executable);

    for (auto i = 0; i < 10; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        StepInto(m_debugger);
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}
