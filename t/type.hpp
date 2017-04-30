#pragma once

#include <string>
#include <typeinfo>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

static inline std::string unmangle(const char * name) {
  int status = -1;
  std::unique_ptr<char, void(*)(void*)> res {
  	abi::__cxa_demangle(name, NULL,NULL, &status),
  	std::free
  };
  return status == 0 ? res.get() : name;
}

#else

static inline std::string unmangle(const char * name) {
  return name;
}

#endif

template <class T> std::string type() {
  return unmangle(typeid(T).name());
}