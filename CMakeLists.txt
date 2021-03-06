# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.5.1)
project(cppgraphqlgen)

if(WIN32)
  # Let CMake figure out the exports for the SHARED library (DLL) on Windows.
  SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

find_package(Threads MODULE REQUIRED)

include(CMakePackageConfigHelpers)

# Set the minimum C++ standard required for compiling,
# but allow for user to override on the command line using:
#     cmake -DCMAKE_CXX_STANDARD=[11|14|17|...] -DCMAKE_CXX_EXTENSIONS=[ON|OFF]
function(cppgraphqlgen_target_set_cxx_standard target)
  if (CMAKE_VERSION VERSION_LESS "3.8")
    # The cxx_std_11 abstract compile feature
    # is available only starting from CMake 3.8.
    # We assume the availability of lambdas
    # indicates a C++11-compatible compiler mode.
    target_compile_features(${target} PUBLIC cxx_lambdas)
  else()
    target_compile_features(${target} PUBLIC cxx_std_11)
  endif()
endfunction()

add_executable(schemagen
  GraphQLTree.cpp
  GraphQLResponse.cpp
  SchemaGenerator.cpp)
target_link_libraries(schemagen PRIVATE CONAN_PKG::pegtl)
target_include_directories(schemagen PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
cppgraphqlgen_target_set_cxx_standard(schemagen)

add_custom_command(
  OUTPUT
    IntrospectionSchema.cpp
    include/graphqlservice/IntrospectionSchema.h
  COMMAND schemagen
  COMMAND ${CMAKE_COMMAND} -E rename IntrospectionSchema.h include/graphqlservice/IntrospectionSchema.h
  DEPENDS schemagen
  COMMENT "Generating IntrospectionSchema files"
)

add_library(graphqlservice
  GraphQLTree.cpp
  GraphQLResponse.cpp
  GraphQLService.cpp
  Introspection.cpp
  IntrospectionSchema.cpp)
target_link_libraries(graphqlservice PUBLIC
  CONAN_PKG::pegtl
  Threads::Threads)
target_include_directories(graphqlservice PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)
cppgraphqlgen_target_set_cxx_standard(graphqlservice)

# RapidJSON is the only option for JSON serialization used in this project, but if you want
# to use another JSON library you can implement an alternate version of the functions in
# JSONResponse.cpp to serialize to and from GraphQLResponse and build graphqljson from that.
option(USE_RAPIDJSON "Use RapidJSON for JSON serialization." ON)

if(USE_RAPIDJSON)

  add_library(graphqljson
    JSONResponse.cpp)
  target_link_libraries(graphqljson PUBLIC
    PRIVATE CONAN_PKG::rapidjson
    PUBLIC graphqlservice)
  target_include_directories(graphqljson SYSTEM PRIVATE
    ${RAPIDJSON_INCLUDE_DIRS})
  cppgraphqlgen_target_set_cxx_standard(graphqljson)
endif()

option(BUILD_TESTS "Build the tests and sample schema library." OFF)
option(UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests and the sample library." ON)

if(BUILD_TESTS OR UPDATE_SAMPLES)
  add_custom_command(
    OUTPUT
      TodaySchema.cpp
      include/TodaySchema.h
    COMMAND schemagen ${CMAKE_CURRENT_SOURCE_DIR}/samples/schema.today.graphql Today today
    COMMAND ${CMAKE_COMMAND} -E rename TodaySchema.h include/TodaySchema.h
    DEPENDS schemagen ${CMAKE_CURRENT_SOURCE_DIR}/samples/schema.today.graphql
    COMMENT "Generating mock TodaySchema files"
  )

  # force the generation of samples on the default build target
  add_custom_target(update_samples ALL
    DEPENDS TodaySchema.cpp
  )

  if(BUILD_TESTS)
    add_library(todaygraphql
      Today.cpp
      TodaySchema.cpp)
    target_link_libraries(todaygraphql PUBLIC
      graphqlservice
      graphqljson)
    target_include_directories(todaygraphql PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_BINARY_DIR}/include)
    cppgraphqlgen_target_set_cxx_standard(todaygraphql)

    add_executable(test_today
      test_today.cpp)
    target_link_libraries(test_today PRIVATE
      todaygraphql)
    cppgraphqlgen_target_set_cxx_standard(test_today)

    enable_testing()

    add_executable(tests
      tests.cpp)
    target_link_libraries(tests PRIVATE
      todaygraphql
      CONAN_PKG::googletest)
    cppgraphqlgen_target_set_cxx_standard(tests)

    add_test(NAME TodayServiceCase
      COMMAND tests --gtest_filter=TodayServiceCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME ArgumentsCase
      COMMAND tests --gtest_filter=ArgumentsCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME PegtlCase
      COMMAND tests --gtest_filter=PegtlCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME ResponseCase
      COMMAND tests --gtest_filter=ResponseCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
  endif()

  if(UPDATE_SAMPLES)
    install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/include/graphqlservice/IntrospectionSchema.h
      ${CMAKE_CURRENT_BINARY_DIR}/IntrospectionSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/include/TodaySchema.h
      ${CMAKE_CURRENT_BINARY_DIR}/TodaySchema.cpp
      DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/samples)
  endif()
endif()

install(TARGETS
    graphqlservice
    graphqljson
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

set(CMAKE_INSTALL_TOOLSDIR ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}
  CACHE PATH "Install schemagen to this directory.")

install(TARGETS schemagen
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_TOOLSDIR}
  CONFIGURATIONS Release)

install(FILES
  include/graphqlservice/GraphQLTree.h
  include/graphqlservice/GraphQLResponse.h
  include/graphqlservice/GraphQLService.h
  include/graphqlservice/JSONResponse.h
  include/graphqlservice/Introspection.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/graphqlservice/IntrospectionSchema.h
  DESTINATION include/graphqlservice
  CONFIGURATIONS Release)

set(CMAKE_INSTALL_CONFIGDIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME}
  CACHE PATH "Install the CMake config files to this directory.")

install(FILES cmake/${PROJECT_NAME}-config.cmake
  DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
)

install(EXPORT cppgraphqlgen-targets
  NAMESPACE cppgraphqlgen::
  DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
)
