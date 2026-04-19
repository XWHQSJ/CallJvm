add_executable(tpool_test tpool_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../callJvmThreadpool/tpool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../callJvmThreadpool/tpool.h
)
target_include_directories(tpool_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../callJvmThreadpool
)
target_link_libraries(tpool_test PRIVATE GTest::gtest_main Threads::Threads)

include(GoogleTest)
gtest_discover_tests(tpool_test)
