find_package(Threads)
find_package(Catch2)
find_package(fmt CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(ICU REQUIRED COMPONENTS cu data uc dt in io)

macro(create_test name file)
    add_executable(${name} ${file} ) #common.h common.cpp
    target_link_libraries(${name} cedilla Catch2::Catch2 pugixml)
    target_compile_options(${name} PRIVATE -std=c++17 -O3 -Wsign-conversion)
    target_compile_definitions(${name} PRIVATE -DUCDXML_FILE="${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml")
    add_test(NAME ${name} COMMAND ${name})
endmacro()

#create_test(tst_prop_script prop_script.cpp)

#create_test(tst_name tst_names.cpp)

#arget_link_libraries(tst_name fmt::fmt)

add_executable(test-properties
    properties.cpp)
target_link_libraries(test-properties cedilla unicode-generator-common Catch2::Catch2)

add_executable(dump
    dump.cpp)
target_link_libraries(dump cedilla fmt::fmt unicode-generator-common)


add_executable(skip_bench skip_bench.cpp)
target_link_libraries(skip_bench cedilla fmt::fmt
    ${ICU_LIBRARIES}
    unicode-generator-common benchmark::benchmark benchmark::benchmark_main)
target_compile_options(skip_bench PUBLIC -std=c++20 -march=native -O3)
