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

# 找到你的目标（你的可执行文件名，比如 BankServer）
# 使用 target_compile_definitions 可以针对特定目标
target_compile_definitions(BankServer PRIVATE
    # 对于所有构建类型都生效的定义可以放这里
    
    # $<CONFIG:Release> 是 CMake 的生成器表达式
    # 意思是：如果当前的构建类型是 Release，则添加后面的定义
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

