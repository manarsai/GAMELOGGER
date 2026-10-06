cmake_minimum_required(VERSION 3.20)

project(GameLoggerBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(GameLoggerBackend
    src/main.cpp
)

target_link_libraries(GameLoggerBackend
    PRIVATE
        Drogon::Drogon
        unofficial::sqlite3::sqlite3
        OpenSSL::Crypto
)