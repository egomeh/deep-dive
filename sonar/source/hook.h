#pragma once

#include <stdint.h>
#include <functional>
#include <windows.h>

enum class HookedFunction
{
	Any,
	HelmManagerFixedUpdate
};

struct HookData
{
	uint32_t ebp;
};

struct HookExecutionRequest
{
	// Sync event used if the client caller wants to wait for the hook to finish
	HANDLE sync_event;
};

class HookManager
{
public:
	HookManager()
	{
		if (!InitializeCriticalSectionAndSpinCount(&submit_function_for_hook_cs, 0x4000))
			abort();

		// Default protection, manual reset, start non-signaled
		hook_service_event = CreateEvent(NULL, TRUE, FALSE, TEXT("HookServiceEvent"));

		requested_hook = HookedFunction::Any;

		hook_request_in_flight = false;

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

	void ServiceHook(HookedFunction current_hook)
	{
		// No request
		if (!hook_request_in_flight)
			return;
		
		// Wrong hook
		if (current_hook != requested_hook && requested_hook != HookedFunction::Any)
			return;

		// Execute function at the hook
		code(hook_data);

		hook_request_in_flight = false;
		SetEvent(hook_service_event);
	}

	bool ExecuteInHookBase(HookedFunction hook_to_execute, std::function<void(const HookData&)> function, bool async)
	{
		EnterCriticalSection(&submit_function_for_hook_cs);

		ResetEvent(hook_service_event);
		requested_hook = hook_to_execute;
		code = function;
		hook_request_in_flight = true;

		if (!async)
			WaitForSingleObject(hook_service_event, INFINITE);

		LeaveCriticalSection(&submit_function_for_hook_cs);
		return true;
	}

	bool ExecuteInHookSync(HookedFunction hook_to_execute, std::function<void(const HookData&)> function)
	{
		return ExecuteInHookBase(hook_to_execute, function, false);
	}

	bool ExecuteInHookAsync(HookedFunction hook_to_execute, std::function<void(const HookData&)> function)
	{
		return ExecuteInHookBase(hook_to_execute, function, true);
	}

	int ebp;

private:
	CRITICAL_SECTION submit_function_for_hook_cs;
	HANDLE hook_service_event;
	HookData hook_data;
	std::function<void(const HookData&)> code;
	HookedFunction requested_hook;
	bool hook_request_in_flight;
};

