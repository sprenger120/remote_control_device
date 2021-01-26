#pragma once

#if __has_include(<span>)
#include <span>
#else
#include <tcb/span.hpp>
// UGLY TEMPORARY HACK UNTIL YOUR CURRENT C++ COMPILER INCLUDES std::span SUPPORT
namespace std
{
using tcb::span;
}
#endif