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


#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res

#define DIVE_UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define ON_EXIT RunOnExit DIVE_UNIQUE_NAME(roe); DIVE_UNIQUE_NAME(roe).m_function = [&] ()

#define FOUR_BYTES(value) (unsigned char)(value >>  0), (unsigned char)(value >> 8), (unsigned char)(value >> 16), (unsigned char)(value >> 24)
