#ifdef USE_MODULE
module;
#endif

// #include "emhash/hash_table5.hpp"
#include <unordered_map>
#include <functional>
#include <functional>

#ifdef USE_MODULE
export module MHashMap;
#undef MEXPORT
#define MEXPORT export
#else
#undef MEXPORT
#define MEXPORT
#endif


// template <typename KeyT, typename ValueT, typename HashT = std::hash<KeyT>, typename EqT = std::equal_to<KeyT>>
// using MMMAP = std::unordered_map<KeyT, ValueT, HashT, EqT>;

template <typename KeyT, typename ValueT>
using MMMAP = std::unordered_map<KeyT, ValueT>;
