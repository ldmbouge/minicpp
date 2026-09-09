#pragma once

namespace gfl {
// A list of types and nothing else — no storage, no instances, no members.
// Lets a header name a set of types without depending on whatever is later
// built from them: the list is written where the types are defined, and the
// machinery that consumes it specializes on Types<Ts...> to get the pack back.
template<typename... Ts> struct Types {};
}
