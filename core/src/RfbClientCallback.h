/*
 * RfbClientCallback.h - wrapper for using member functions as libvncclient callbacks
 *
 * Copyright (c) 2021-2024 Tobias Junghans <tobydox@veyon.io>
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

#include "VncConnection.h"

using rfbClient = struct _rfbClient;

struct RfbClientCallback
{
	template<auto fn, typename ...Args>
	using ReturnType = typename std::result_of<decltype(fn)(VncConnection*, Args...)>::type;

	template<auto fn, typename ...Args>
	using ReturnTypeIsVoid = std::is_void<ReturnType<fn, Args...>>;

	template<auto fn, typename ...Args>
	using EnableIfHasReturnType = typename std::enable_if<!ReturnTypeIsVoid<fn, Args...>::value,
										ReturnType<fn, Args...>>::type;

	template<auto fn, typename ...Args>
	using EnableIfHasNoReturnType = typename std::enable_if<std::is_void<ReturnType<fn, Args...>>::value,
												 void>::type;

	template<typename ...Args>
	static VncConnection* vncConnectionFromClient( rfbClient* client, Args... )
	{
		return static_cast<VncConnection *>( VncConnection::clientData( client, VncConnection::VncConnectionTag ) );
	}

	template<auto fn, auto DefaultReturnValue = 0, typename ...Args>
	static EnableIfHasReturnType<fn, Args...> wrap(Args...args)
	{
		if( const auto connection = vncConnectionFromClient(std::forward<Args>(args)...); connection )
		{
			return (connection->*fn)( std::forward<Args>(args)...);
		}

		return DefaultReturnValue;
	}

	template<auto fn, auto DefaultReturnValue = 0, typename ...Args>
	static EnableIfHasReturnType<fn, Args...> wrap(rfbClient* client, Args...args)
	{
		if( const auto connection = vncConnectionFromClient(client); connection )
		{
			return (connection->*fn)( std::forward<Args>(args)...);
		}

		return DefaultReturnValue;
	}

	template<auto fn, typename ...Args>
	static EnableIfHasNoReturnType<fn, Args...> wrap(Args...args)
	{
		if( const auto connection = vncConnectionFromClient(std::forward<Args>(args)...); connection )
		{
			(connection->*fn)( std::forward<Args>(args)...);
		}
	}

	template<auto fn, typename ...Args>
	static EnableIfHasNoReturnType<fn, Args...> wrap(rfbClient* client, Args...args)
	{
		if( const auto connection = vncConnectionFromClient(client); connection )
		{
			(connection->*fn)( std::forward<Args>(args)...);
		}
	}

};

