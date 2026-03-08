# Client executable
qt_add_executable(BankApp
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    loginwidget.cpp
    loginwidget.h
    transferwidget.cpp
    transferwidget.h
    adminwidget.cpp
    adminwidget.h
    protocolutils.cpp
    protocolutils.h
    tcpclient.cpp
    tcpclient.h
)

target_link_libraries(BankApp
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
)

include(GNUInstallDirs)

install(TARGETS BankApp
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET BankApp
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
