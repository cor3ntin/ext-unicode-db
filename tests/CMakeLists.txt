find_package(Threads)
find_package(Catch2)
find_package(fmt CONFIG REQUIRED)

macro(create_test name file)
    add_executable(${name} ${file} common.h common.cpp )
    target_link_libraries(${name} std_ext_uni Catch2::Catch2 pugixml)
    target_compile_options(${name} PRIVATE -std=c++17 -O3 -Wsign-conversion)
    target_compile_definitions(${name} PRIVATE -DUCDXML_FILE="${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml")
    add_test(NAME ${name} COMMAND ${name})
endmacro()

create_test(tst_prop_script prop_script.cpp)

create_test(tst_name tst_names.cpp)
target_link_libraries(tst_name fmt::fmt)

add_executable(dump
    dump.cpp)
target_link_libraries(dump std_ext_uni)
