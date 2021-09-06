find_package(OpenSSL)
find_package(Threads)

add_library(core INTERFACE)
target_link_libraries(core
        INTERFACE Threads::Threads)

add_library(net
        core/net/http.cpp
        core/net/socket.cpp
        core/net/ssl.cpp
        core/net/url.cpp)
target_link_libraries(net
        PUBLIC OpenSSL::SSL OpenSSL::Crypto)

add_library(html_parser
        html_parser/html_parser.cpp)
target_link_libraries(html_parser
        PUBLIC core net)

add_library(crawler
        crawler/crawler.cpp
        crawler/robots_catalog.cpp
        distributed/distributed.cpp)
target_link_libraries(crawler
        PUBLIC core net html_parser)

add_executable(crawler_cli
        crawler/main.cpp)
target_link_libraries(crawler_cli
        PRIVATE crawler)
