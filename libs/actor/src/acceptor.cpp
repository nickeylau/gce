﻿///
/// Copyright (c) 2009-2014 Nous Xiong (348944179 at qq dot com)
///
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// See https://github.com/nousxiong/gce for latest version.
///

#include <gce/actor/detail/acceptor.hpp>
#include <gce/actor/detail/socket.hpp>
#include <gce/actor/impl/tcp/acceptor.hpp>
#include <gce/actor/detail/cache_pool.hpp>
#include <gce/actor/mixin.hpp>
#include <gce/actor/detail/mailbox.hpp>
#include <gce/actor/message.hpp>
#include <gce/detail/scope.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/variant/get.hpp>
#include <boost/lexical_cast.hpp>

namespace gce
{
namespace detail
{
///----------------------------------------------------------------------------
acceptor::acceptor(context* ctx)
  : basic_actor(ctx->get_attributes().max_cache_match_size_)
  , stat_(ready)
  , ctx_(*ctx)
  , acpr_(0)
{
}
///----------------------------------------------------------------------------
acceptor::~acceptor()
{
}
///----------------------------------------------------------------------------
void acceptor::init(cache_pool* user, cache_pool* owner, net_option opt)
{
  BOOST_ASSERT_MSG(stat_ == ready, "socket status error");
  user_ = user;
  owner_ = owner;
  opt_ = opt;

  base_type::update_aid();
}
///----------------------------------------------------------------------------
void acceptor::bind(std::string const& ep, aid_t master)
{
  master_ = master;
  base_type::add_link(master_);

  boost::asio::spawn(
    user_->get_strand(),
    boost::bind(
      &acceptor::run, this, ep, _1
      ),
    boost::coroutines::attributes(
      boost::coroutines::stack_allocator::default_stacksize()
      )
    );
}
///----------------------------------------------------------------------------
void acceptor::on_free()
{
  base_type::on_free();

  stat_ = ready;
  acpr_ = 0;
  master_ = aid_t();
}
///----------------------------------------------------------------------------
void acceptor::on_recv(pack* pk)
{
  strand_t& snd = user_->get_strand();
  snd.dispatch(
    boost::bind(
      &acceptor::handle_recv, this, pk
      )
    );
}
///----------------------------------------------------------------------------
void acceptor::run(std::string const& ep, yield_t yield)
{
  exit_code_t exc = exit_normal;
  std::string exit_msg("exit normal");
  try
  {
    stat_ = on;
    acpr_ = make_acceptor(ep);
    acpr_->bind();

    while (stat_ == on)
    {
      errcode_t ec;
      basic_socket* prot = acpr_->accept(yield[ec]);
      if (ec)
      {
        close();
        break;
      }

      socket* s = user_->get_socket();
      s->init(ctx_.select_cache_pool(), user_, opt_);
      s->start(prot, master_);
    }
  }
  catch (std::exception& ex)
  {
    exc = exit_except;
    exit_msg = ex.what();
    close();
  }
  catch (...)
  {
    exc = exit_unknown;
    exit_msg = "unexpected exception";
    close();
  }
  free_self(exc, exit_msg, yield);
}
///----------------------------------------------------------------------------
basic_acceptor* acceptor::make_acceptor(std::string const& ep)
{
  /// Find protocol name
  std::size_t pos = ep.find("://");
  if (pos == std::string::npos)
  {
    throw std::runtime_error("protocol name parse failed");
  }

  std::string prot_name = ep.substr(0, pos);
  if (prot_name == "tcp")
  {
    /// Parse address
    std::size_t begin = pos + 3;
    pos = ep.find(':', begin);
    if (pos == std::string::npos)
    {
      throw std::runtime_error("tcp address parse failed");
    }

    std::string address = ep.substr(begin, pos - begin);

    /// Parse port
    begin = pos + 1;
    pos = ep.size();

    boost::uint16_t port =
      boost::lexical_cast<boost::uint16_t>(
        ep.substr(begin, pos - begin)
        );
    return new tcp::acceptor(user_->get_strand(), address, port);
  }

  throw std::runtime_error("unsupported protocol");
}
///----------------------------------------------------------------------------
void acceptor::handle_recv(pack* pk)
{
  scope scp(boost::bind(&basic_actor::dealloc_pack, user_, pk));
  if (check(pk->recver_))
  {
    if (exit_t* ex = boost::get<exit_t>(&pk->tag_))
    {
      base_type::remove_link(ex->get_aid());
      if (ex->get_aid() == master_)
      {
        close();
      }
    }
  }
  else if (!pk->is_err_ret_)
  {
    if (detail::link_t* link = boost::get<detail::link_t>(&pk->tag_))
    {
      /// send actor exit msg
      base_type::send_already_exited(link->get_aid(), pk->recver_, user_);
    }
    else if (detail::request_t* req = boost::get<detail::request_t>(&pk->tag_))
    {
      /// reply actor exit msg
      response_t res(req->get_id(), pk->recver_);
      base_type::send_already_exited(req->get_aid(), res, user_);
    }
  }
}
///----------------------------------------------------------------------------
void acceptor::close()
{
  stat_ = off;
  acpr_->close();
}
///----------------------------------------------------------------------------
void acceptor::free_self(exit_code_t exc, std::string const& exit_msg, yield_t yield)
{
  delete acpr_;

  base_type::send_exit(exc, exit_msg, user_);
  base_type::update_aid();
  user_->free_acceptor(owner_, this);
}
///----------------------------------------------------------------------------
}
}