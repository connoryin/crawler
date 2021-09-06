FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0)

FetchContent_MakeAvailable(googletest)

add_executable(core_test
        core/bloom_filter_test.cpp
        core/file_system_test.cpp
        core/thread_test.cpp)
target_link_libraries(core_test
        PRIVATE core gtest_main)

add_executable(net_test
        core/net/http_test.cpp
        core/net/socket_test.cpp
        core/net/url_test.cpp)
target_link_libraries(net_test
        PRIVATE net gtest_main)

add_executable(html_parser_test
        html_parser/html_parser_test.cpp)
target_link_libraries(html_parser_test
        PRIVATE html_parser gtest_main)

add_executable(crawler_test
        crawler/robots_catalog_test.cpp)
target_link_libraries(crawler_test
        PRIVATE crawler gtest_main)
