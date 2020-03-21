cmake_minimum_required(VERSION 3.12)
project(lmake LANGUAGES CXX VERSION 0.1)

find_package(Lua REQUIRED)

add_executable(lmake main.cpp)
target_compile_features(lmake PUBLIC cxx_std_20)
target_include_directories(lmake PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(lmake PUBLIC ${LUA_LIBRARIES})
target_compile_definitions(lmake PUBLIC LUA_USE_APICHECK)


