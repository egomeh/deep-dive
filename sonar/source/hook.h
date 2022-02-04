#pragma once

#include <stdint.h>
#include <functional>
#include <windows.h>
#include <list>

enum class HookedFunction
{
    Any,
    HelmManagerFixedUpdate,
    WeaponSourceSetWaypoint,
    TranslateShipForward,
};

struct HookData
{
    uint32_t ebp;
    uint32_t eax;
};

struct HookExecutionRequest
{
    // Sync event used if the client caller wants to wait for the hook to finish
    HANDLE sync_event = NULL;
    HookedFunction hook;
    std::function<bool(const HookData)> function = {};
};

class HookManager
{
public:
    HookManager()
    {
        if (!InitializeCriticalSectionAndSpinCount(&submit_function_for_hook_cs, 0x4000))
            abort();

        ebp = 0;
        hook_data = {};
    }

    static HookManager& Get()
    {
        static HookManager hook_manager;
        return hook_manager;
    }

    void SetEbp(int ebp)
    {
        hook_data.ebp = ebp;
    }

    void SetEax(int eax)
    {
        hook_data.eax = eax;
    }

    void ServiceHook(HookedFunction current_hook)
    {
        for (auto it = hook_execution_requests.begin(); it != hook_execution_requests.end();)
        {
            HookExecutionRequest& current_request = *it;

            if (current_hook != current_request.hook && current_request.hook != HookedFunction::Any)
            {
                ++it;
                continue;
            }

            // if the function is not satisifed, we exit.
            if (!current_request.function(hook_data))
            {
                ++it;
                continue;
            }

            // at this point, the function is happy and we can remove the request

            // and we can also signal the caller if its sync
            if (current_request.sync_event)
            {
                // set to awake the callee
                SetEvent(current_request.sync_event);

                // wait for callee to awake and move on
                WaitForSingleObject(current_request.sync_event, INFINITE);

                // now close as we konw the callee has moved on
                CloseHandle(current_request.sync_event);
            }

            it = hook_execution_requests.erase(it);
        }
    }

    bool ExecuteInHookBase(HookedFunction hook_to_execute, std::function<bool(const HookData&)> function, bool async)
    {
        HookExecutionRequest request;
        request.hook = hook_to_execute;
        request.function = function;

        if (!async)
            request.sync_event = CreateEventA(NULL, FALSE, FALSE, "hook_request_event");
        else
            request.sync_event = NULL;

        EnterCriticalSection(&submit_function_for_hook_cs);
        hook_execution_requests.push_back(request);
        LeaveCriticalSection(&submit_function_for_hook_cs);

        if (!async && request.sync_event != NULL)
        {
            WaitForSingleObject(request.sync_event, INFINITE);

            // set to allow the hook manager know we can remove the event
            SetEvent(request.sync_event);
        }

        return true;
    }

    bool ExecuteInHookSync(HookedFunction hook_to_execute, std::function<bool(const HookData&)> function)
    {
        return ExecuteInHookBase(hook_to_execute, function, false);
    }

    bool ExecuteInHookAsync(HookedFunction hook_to_execute, std::function<bool(const HookData&)> function)
    {
        return ExecuteInHookBase(hook_to_execute, function, true);
    }

    int ebp;

private:
    CRITICAL_SECTION submit_function_for_hook_cs;
    HookData hook_data;
    std::list<HookExecutionRequest> hook_execution_requests;
};

