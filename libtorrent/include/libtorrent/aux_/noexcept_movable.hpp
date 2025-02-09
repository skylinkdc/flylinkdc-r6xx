/*

Copyright (c) 2017-2021, Arvid Norberg
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/
#ifndef TORRENT_NOEXCEPT_MOVABLE_HPP_INCLUDED
#define TORRENT_NOEXCEPT_MOVABLE_HPP_INCLUDED

#include <type_traits>
#include <utility>

namespace libtorrent {
namespace aux {

#if defined _MSC_VER && defined TORRENT_BUILD_SIMULATOR
	// see simulation/test_error_handling.cpp for a description of this variable
	extern thread_local int g_must_not_fail;

	template <typename T>
	T&& wrap(T&& v) {
		++g_must_not_fail;
		return v;
	}
#endif

	// this is a silly wrapper for address and endpoint to make their move
	// constructors be noexcept (because boost.asio is incorrectly making them
	// potentially throwing). This can be removed once libtorrent uses the
	// networking TS.
	template <typename T>
	struct noexcept_movable : T
	{
		noexcept_movable() = default;
#if defined _MSC_VER && defined TORRENT_BUILD_SIMULATOR
		noexcept_movable(noexcept_movable<T>&& rhs) noexcept
			: T(std::forward<T>(wrap(rhs)))
		{
			--g_must_not_fail;
		}
		noexcept_movable(T&& rhs) noexcept : T(std::forward<T>(wrap(rhs))) // NOLINT
		{
			--g_must_not_fail;
		}
#else
		noexcept_movable(noexcept_movable<T>&& rhs) noexcept
			: T(std::forward<T>(rhs)) {}
		noexcept_movable(T&& rhs) noexcept : T(std::forward<T>(rhs)) {} // NOLINT
#endif
		noexcept_movable(noexcept_movable<T> const& rhs) = default;
		noexcept_movable(T const& rhs) : T(rhs) {} // NOLINT
		noexcept_movable& operator=(noexcept_movable const& rhs) = default;
		noexcept_movable& operator=(noexcept_movable&& rhs) = default;
		using T::T;
		using T::operator=;
	};

	template <typename T>
	struct noexcept_move_only : T
	{
#if defined _MSC_VER && defined TORRENT_BUILD_SIMULATOR
		noexcept_move_only(noexcept_move_only<T>&& rhs) noexcept
			: T(std::forward<T>(wrap(rhs)))
		{
			--g_must_not_fail;
		}
		noexcept_move_only(T&& rhs) noexcept : T(std::forward<T>(wrap(rhs))) // NOLINT
		{
			--g_must_not_fail;
		}
#else
		noexcept_move_only(noexcept_move_only<T>&& rhs) noexcept
			: T(std::forward<T>(rhs))
		{}
		noexcept_move_only(T&& rhs) noexcept : T(std::forward<T>(rhs)) {} // NOLINT
#endif
		noexcept_move_only(noexcept_move_only<T> const& rhs) = default;
		noexcept_move_only(T const& rhs) : T(rhs) {} // NOLINT
		noexcept_move_only& operator=(noexcept_move_only const& rhs) = default;
		noexcept_move_only& operator=(noexcept_move_only&& rhs) = default;
		using T::T;
	};

}
}

#endif
