cmake_minimum_required(VERSION 3.19)
project(BankApp LANGUAGES CXX)

find_package(Qt6 REQUIRED
    COMPONENTS
        Core
        Widgets
        Network
        Sql)

qt_standard_project_setup()

add_subdirectory(server)
add_subdirectory(client)

