#ifndef _STDAFX_H
#define _STDAFX_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <cstdlib>
#include <memory>
#include "ctype.h"
#include <cstring>
#include <time.h>
#include <cstdlib>
#include <sys/time.h>
#include <typeinfo>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <random>
#include <stdexcept>
#include <tuple>
#include <thread>
#include <stack>
#include <typeinfo>
#include <tuple>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/string.hpp>

#ifndef RELEASE
//#define DEBUG_STL
#endif

#ifdef DEBUG_STL

#include <debug/vector>
#include <debug/string>
#include <debug/deque>
#include <debug/set>
#include <debug/map>
using __gnu_debug::vector;
using __gnu_debug::map;
using __gnu_debug::set;
using __gnu_debug::deque;
using __gnu_debug::string;

#else

#include <vector>
#include <string>
#include <deque>
#include <set>
#include <map>
using std::vector;
using std::map;
using std::set;
using std::deque;
using std::string;

#endif


using std::queue;
using std::unique_ptr;
using std::default_random_engine;
using std::function;
using std::initializer_list;
using std::unordered_set;
using std::pair;
using std::tuple;
using std::out_of_range;
using std::unordered_map;
using std::min;
using std::max;
using std::ofstream;
using std::ifstream;
using std::endl;
using std::priority_queue;
using std::make_pair;
using std::stack;
using std::uniform_int_distribution;
using std::uniform_real_distribution;
using std::make_tuple;
using std::get;
using std::hash;
using std::thread;

#include "serialization.h"
#endif
