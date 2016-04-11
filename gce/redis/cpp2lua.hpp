///
/// Copyright (c) 2009-2015 Nous Xiong (348944179 at qq dot com)
///
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// See https://github.com/nousxiong/gce for latest version.
///

#ifndef GCE_REDIS_CPP2LUA_HPP
#define GCE_REDIS_CPP2LUA_HPP

#include <gce/redis/config.hpp>
#include <gce/redis/detail/lua_wrap.hpp>

namespace gce
{
namespace redis
{
namespace lua
{
///------------------------------------------------------------------------------
/// conn
///------------------------------------------------------------------------------
inline void load(lua_State* L, int arg, conn_t& c)
{
  detail::lua::load(L, arg, c);
}

inline void push(lua_State* L, conn_t const& c)
{
  detail::lua::push(L, c);
}
///------------------------------------------------------------------------------
/// context_id
///------------------------------------------------------------------------------
inline void load(lua_State* L, int arg, ctxid_t& ctxid)
{
  detail::lua::load(L, arg, ctxid);
}

inline void push(lua_State* L, ctxid_t const& ctxid)
{
  detail::lua::push(L, ctxid);
}
///------------------------------------------------------------------------------
} /// namespace lua
} /// namespace redis
} /// namespace gce

#endif /// GCE_REDIS_CPP2LUA_HPP