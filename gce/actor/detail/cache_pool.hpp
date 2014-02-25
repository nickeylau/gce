﻿///
/// Copyright (c) 2009-2014 Nous Xiong (348944179 at qq dot com)
///
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// See https://github.com/nousxiong/gce for latest version.
///

#ifndef GCE_ACTOR_DETAIL_CACHE_POOL_HPP
#define GCE_ACTOR_DETAIL_CACHE_POOL_HPP

#include <gce/actor/config.hpp>
#include <gce/actor/detail/pack.hpp>
#include <gce/actor/detail/object_pool.hpp>
#include <gce/detail/mpsc_queue.hpp>
#include <gce/detail/unique_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <vector>

namespace gce
{
class context;
class actor;
class thin;
struct attributes;

namespace detail
{
class socket;
class acceptor;
typedef object_pool<actor, detail::actor_attrs> actor_pool_t;
typedef object_pool<thin, detail::thin_attrs> thin_pool_t;
typedef object_pool<socket, context*> socket_pool_t;
typedef object_pool<acceptor, context*> acceptor_pool_t;
typedef mpsc_queue<actor> actor_queue_t;
typedef mpsc_queue<thin> thin_queue_t;
typedef mpsc_queue<socket> socket_queue_t;
typedef mpsc_queue<acceptor> acceptor_queue_t;

class cache_pool
  : private boost::noncopyable
{
public:
  cache_pool(context& ctx, std::size_t id, attributes const& attrs, bool mixed);
  ~cache_pool();

public:
  inline std::size_t get_id() const { return id_; };
  inline context& get_context() { return *ctx_; }
  inline std::size_t get_cache_match_size() const { return cache_match_size_; }
  inline strand_t& get_strand() { return snd_; }
  inline timer_t& get_gc_timer() { return gc_tmr_; }

  actor* get_actor();
  thin* get_thin();
  pack* get_pack();
  socket* get_socket();
  acceptor* get_acceptor();

  void free_actor(cache_pool*, actor*);
  void free_thin(cache_pool*, thin*);
  void free_pack(cache_pool*, pack*);
  void free_socket(cache_pool*, socket*);
  void free_acceptor(cache_pool*, acceptor*);

  void free_object();
  void free_cache();

private:
  template <typename T, typename FreeQueue>
  struct cache
  {
    cache()
      : obj_(0)
      , que_(0)
      , size_(0)
    {
    }

    inline void push(T* obj, FreeQueue* que)
    {
      if (que_)
      {
        BOOST_ASSERT(que == que_);
      }
      que_ = que;

      object_access::set_next(obj, obj_);
      obj_ = obj;
      ++size_;
    }

    inline void free()
    {
      if (obj_)
      {
        que_->push(obj_);
        clear();
      }
    }

    inline void clear()
    {
      obj_ = 0;
      que_ = 0;
      size_ = 0;
    }

    T* obj_;
    FreeQueue* que_;
    std::size_t size_;
  };

  template <
    typename T, typename Pool,
    typename FreeQueue, typename DirtyList
    >
  void free_object(
    cache_pool*, T*,
    std::vector<cache<T, FreeQueue> >&, Pool&,
    FreeQueue&, DirtyList&
    );

  template <typename T, typename Pool, typename FreeQueue>
  void free_object(Pool&, FreeQueue&);

private:
  byte_t pad0_[GCE_CACHE_LINE_SIZE]; /// Ensure start from a new cache line.

  GCE_CACHE_ALIGNED_VAR(context*, ctx_)

  GCE_CACHE_ALIGNED_VAR(std::size_t, id_)
  GCE_CACHE_ALIGNED_VAR(std::size_t, cache_num_)
  GCE_CACHE_ALIGNED_VAR(std::size_t, cache_match_size_)

  GCE_CACHE_ALIGNED_VAR(bool, mixed_)

  GCE_CACHE_ALIGNED_VAR(strand_t, snd_)
  GCE_CACHE_ALIGNED_VAR(timer_t, gc_tmr_)

  /// pools
  GCE_CACHE_ALIGNED_VAR(actor_pool_t, actor_pool_)
  GCE_CACHE_ALIGNED_VAR(thin_pool_t, thin_pool_)
  GCE_CACHE_ALIGNED_VAR(pack_pool_t, pack_pool_)
  GCE_CACHE_ALIGNED_VAR(socket_pool_t, socket_pool_)
  GCE_CACHE_ALIGNED_VAR(acceptor_pool_t, acceptor_pool_)

  /// queues
  GCE_CACHE_ALIGNED_VAR(actor_queue_t, actor_free_queue_)
  GCE_CACHE_ALIGNED_VAR(thin_queue_t, thin_free_queue_)
  GCE_CACHE_ALIGNED_VAR(pack_queue_t, pack_free_queue_)
  GCE_CACHE_ALIGNED_VAR(socket_queue_t, socket_free_queue_)
  GCE_CACHE_ALIGNED_VAR(acceptor_queue_t, acceptor_free_queue_)

  /// thread local vals
  typedef cache<actor, actor_queue_t> actor_cache_t;
  typedef cache<thin, thin_queue_t> thin_cache_t;
  typedef cache<pack, pack_queue_t> pack_cache_t;
  typedef cache<socket, socket_queue_t> socket_cache_t;
  typedef cache<acceptor, acceptor_queue_t> acceptor_cache_t;

  std::vector<actor_cache_t> actor_cache_list_;
  std::vector<thin_cache_t> thin_cache_list_;
  std::vector<pack_cache_t> pack_cache_list_;
  std::vector<socket_cache_t> socket_cache_list_;
  std::vector<acceptor_cache_t> acceptor_cache_list_;

  std::vector<actor_cache_t*> actor_cache_dirty_list_;
  std::vector<thin_cache_t*> thin_cache_dirty_list_;
  std::vector<pack_cache_t*> pack_cache_dirty_list_;
  std::vector<socket_cache_t*> socket_cache_dirty_list_;
  std::vector<acceptor_cache_t*> acceptor_cache_dirty_list_;
};
}
}

#endif /// GCE_ACTOR_DETAIL_CACHE_POOL_HPP