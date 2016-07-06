///
/// Copyright (c) 2009-2015 Nous Xiong (348944179 at qq dot com)
///
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// See https://github.com/nousxiong/gce for latest version.
///

#ifndef GCE_HTTP_HEADER_HPP
#define GCE_HTTP_HEADER_HPP

#include <gce/http/config.hpp>
#include <string>

namespace gce
{
namespace http
{
struct header
{
  std::string name_;
  std::string value_;
};
} /// namespace http
} /// namespace gce

#endif /// GCE_HTTP_HEADER_HPP
