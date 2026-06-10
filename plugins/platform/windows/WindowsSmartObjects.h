/*
 * WindowsSmartObjects.h - smart pointers and handles for RAII
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <userenv.h>

template<typename Traits>
class UniqueResource
{
public:
	using Pointer = typename Traits::Pointer;

	UniqueResource() noexcept = default;
	explicit UniqueResource(Pointer value) noexcept
		: m_ptr(value)
	{
	}

	~UniqueResource() noexcept
	{
		reset();
	}

	UniqueResource(const UniqueResource&) = delete;
	UniqueResource& operator=(const UniqueResource&) = delete;

	UniqueResource(UniqueResource&& other) noexcept
		: m_ptr(other.release())
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


template <typename P>
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


template <typename T>
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


struct WCharArrayTraits : NullableResourceTraits<wchar_t *>
{
	static void close(Pointer p) noexcept { delete [] p; }
};

using SmartWCharPtr = UniqueResource<WCharArrayTraits>;
