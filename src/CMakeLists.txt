add_library(rts rts.cpp rts/cpu.cpp)

include(CheckFunctionExists)

CHECK_FUNCTION_EXISTS(pow POW_EXISTS)
if(NOT POW_EXISTS)
  unset(POW_EXISTS)
  list(APPEND CMAKE_REQUIRED_LIBRARIES m)
  CHECK_FUNCTION_EXISTS(pow POW_EXISTS)
  if(POW_EXISTS)
		set(MATH_LIBRARIES "m")
  else()
    message(FATAL_ERROR "No pow() found")
  endif()
endif()

target_link_libraries(rts ${MATH_LIBRARIES} PUBLIC Boost::context)