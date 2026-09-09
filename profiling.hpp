#ifndef __PROFILING_HPP
#define __PROFILING_HPP

#include <cstdlib>
#include <iostream>
#include <string>

#include "gfl/Timer.hpp"

/**
 * @brief Phase timing for the propagators, keyed by name.
 *
 * Off unless MINICPP_PROFILE is set in the environment, in which case a summary
 * of where the time went is printed when the program exits. An ordinary run
 * pays one predictable branch per phase and allocates nothing.
 */
inline bool profiling()
{
   static bool const on = [] {
      bool const enabled = std::getenv("MINICPP_PROFILE") != nullptr;
      if (enabled) {
         // Build the timer's singleton before registering the handler that
         // reads it. Destructors of function local statics run in reverse
         // order of construction, interleaved with atexit handlers, so a
         // singleton first touched afterwards would already be gone by then.
         gfl::Timer::reset();
         std::atexit([] { std::cout << gfl::Timer::summary(); });
      }
      return enabled;
   }();
   return on;
}

/**
 * @brief Times the scope it is declared in, under `name`.
 *
 * Does nothing when profiling is off. The name is held by reference, so it must
 * outlive the scope -- pass a member, not a temporary.
 */
class Phase {
   std::string const * _name;
public:
   explicit Phase(std::string const & name) : _name(profiling() ? &name : nullptr) {
      if (_name) gfl::Timer::begin(*_name);
   }
   ~Phase() { if (_name) gfl::Timer::end(*_name);}
   Phase(Phase const &) = delete;
   Phase& operator=(Phase const &) = delete;
};

#endif
