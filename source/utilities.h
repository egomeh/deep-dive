#pragma once

#include <functional>

class RunOnExit
{
public:
	RunOnExit()
	{
		m_function = nullptr;
	}

	~RunOnExit()
	{
		if (m_function)
			m_function();
	}

	RunOnExit(RunOnExit&& other) noexcept
	{
		m_function = std::move(other.m_function);
		other.m_function = nullptr;
	}

	std::function<void()> m_function;
};

#define ON_EXIT RunOnExit roe; roe.m_function = [&] ()
