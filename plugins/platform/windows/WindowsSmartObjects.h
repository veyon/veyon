// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <userenv.h>

template<typename Traits>
class UniqueResource
{
public:
	using Pointer = typename Traits::Pointer;

	UniqueResource() noexcept = default;
	explicit UniqueResource(Pointer value) noexcept :
		m_ptr(value)
	{
	}

	~UniqueResource() noexcept
	{
		reset();
	}

	UniqueResource(const UniqueResource&) = delete;
	UniqueResource& operator=(const UniqueResource&) = delete;

	UniqueResource(UniqueResource&& other) noexcept :
		m_ptr(other.release())
	{
	}

	UniqueResource& operator=(UniqueResource&& other) noexcept
	{
		if (this != &other)
		{
			reset(other.release());
		}
		return *this;
	}

	void reset(Pointer value = Traits::invalidValue()) noexcept
	{
		if (isValid())
		{
			Traits::close(m_ptr);
		}
		m_ptr = value;
	}

	[[nodiscard]] Pointer get() const noexcept
	{
		return m_ptr;
	}

	[[nodiscard]] bool isInvalid() const noexcept
	{
		return m_ptr == Traits::invalidValue();
	}

	[[nodiscard]] bool isValid() const noexcept
	{
		return isInvalid() == false;
	}

	[[nodiscard]] explicit operator bool() const noexcept
	{
		return isValid();
	}

	[[nodiscard]] Pointer release() noexcept
	{
		Pointer tmp = m_ptr;
		m_ptr = Traits::invalidValue();
		return tmp;
	}

	[[nodiscard]] Pointer* put() noexcept
	{
		reset();
		return &m_ptr;
	}

private:
	Pointer m_ptr = Traits::invalidValue();
};


template<typename P>
struct NullableResourceTraits
{
	using Pointer = P;

	static constexpr Pointer invalidValue() noexcept { return nullptr; }
};


struct HandleTraits : NullableResourceTraits<HANDLE>
{
	static void close(Pointer p) noexcept { CloseHandle(p); }
};

using SmartHandle = UniqueResource<HandleTraits>;
using SmartToken = UniqueResource<HandleTraits>;


struct FileHandleTraits
{
	using Pointer = HANDLE;

	static Pointer invalidValue() noexcept { return INVALID_HANDLE_VALUE; }
	static void close(Pointer p) noexcept { CloseHandle(p); }
};

using SmartFileHandle = UniqueResource<FileHandleTraits>;


struct EnvBlockTraits : NullableResourceTraits<LPVOID>
{
	static void close(Pointer p) noexcept { DestroyEnvironmentBlock(p); }
};

using SmartEnvBlockPtr = UniqueResource<EnvBlockTraits>;


template<typename T>
struct CoTaskMemTraits : NullableResourceTraits<T*>
{
	static void close(T* p) noexcept { CoTaskMemFree(p); }
};

template<typename T>
using SmartCoTaskMemPtr = UniqueResource<CoTaskMemTraits<T>>;


struct SecurityDescriptorTraits : NullableResourceTraits<PSECURITY_DESCRIPTOR>
{
	static void close(Pointer p) noexcept { LocalFree(p); }
};

using SmartSecurityDescriptor = UniqueResource<SecurityDescriptorTraits>;


struct SIDTraits : NullableResourceTraits<PSID>
{
	static void close(Pointer p) noexcept { FreeSid(p); }
};

using SmartSID = UniqueResource<SIDTraits>;


struct ACLTraits : NullableResourceTraits<PACL>
{
	static void close(Pointer p) noexcept { LocalFree(p); }
};

using SmartACL = UniqueResource<ACLTraits>;


struct StringSIDTraits : NullableResourceTraits<LPWSTR>
{
	static void close(Pointer p) noexcept { LocalFree(p); }
};

using SmartStringSID = UniqueResource<StringSIDTraits>;


struct WCharArrayTraits : NullableResourceTraits<wchar_t*>
{
	static void close(Pointer p) noexcept { delete[] p; }
};

using SmartWCharPtr = UniqueResource<WCharArrayTraits>;
