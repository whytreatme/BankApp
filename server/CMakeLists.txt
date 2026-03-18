# Server executable
qt_add_executable(BankServer
    main.cpp
    protocolutils.cpp
    database.cpp
    tcpreactor.cpp
    worktask.cpp
    dao/userdao.cpp
    dao/accountdao.cpp
    dao/transactiondao.cpp
    service/userservice.cpp
    service/accountservice.cpp
    service/transactionservice.cpp
    service/adminservice.cpp
    controller/usercontroller.cpp
    controller/accountcontroller.cpp
    controller/transactioncontroller.cpp
    controller/admincontroller.cpp
    snowflake.h
    metricsmanager.h
)

set_target_properties(BankServer PROPERTIES AUTOMOC ON)

target_link_libraries(BankServer
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Sql
)
