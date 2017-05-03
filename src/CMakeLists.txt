add_library(rts rts.cpp rts/cpu.cpp)

target_link_libraries(rts ${MATH_LIBRARIES} PUBLIC Boost::context)