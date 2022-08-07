find_package(Threads REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(TBB REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#set(CMAKE_CXX_FLAGS "-fsanitize=address")

add_library(unicode-generator-common
OBJECT
    load.hpp
    load.cpp
    codepoint_data.hpp
    bitset_generator.hpp
    skiplist_generator.hpp
    perfect_hash.hpp
    utils.hpp
    generators.hpp
)
target_link_libraries(unicode-generator-common PUBLIC
  Threads::Threads
  pugixml
  fmt::fmt
  range-v3-concepts
  TBB::tbb
)
target_include_directories(unicode-generator-common PUBLIC .)


add_executable(unicode-properties-data-generator
    properties-generator.cpp
    categories.cpp
    scripts.cpp
)
target_link_libraries(unicode-properties-data-generator PRIVATE unicode-generator-common)


add_executable(unicode-normalization-data-generator
    normalization.cpp
)
target_link_libraries(unicode-normalization-data-generator PRIVATE unicode-generator-common)

#add_executable(unicode-cp-to-name-data-generator cp-to-name-generator.cpp)

#target_link_libraries(unicode-cp-to-name-data-generator
#    PRIVATE
#      unicode-generator-common
#      TBB::tbb
#)

#target_compile_options(unicode-cp-to-name-data-generator PRIVATE "-fopenmp-simd" "-march=native")

add_executable(unicode-name-to-cp-data-generator name-to-cp-generator.cpp)
target_link_libraries(unicode-name-to-cp-data-generator
      PRIVATE
        unicode-generator-common
)

target_compile_options(unicode-name-to-cp-data-generator PRIVATE "-fopenmp-simd" "-march=native")
