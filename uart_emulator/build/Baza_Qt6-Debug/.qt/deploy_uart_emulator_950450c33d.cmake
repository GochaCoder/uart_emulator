include("D:/CCode/QtProjects/uart_emulator/uart_emulator/build/Baza_Qt6-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/uart_emulator-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase;qtserialport")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/CCode/QtProjects/uart_emulator/uart_emulator/build/Baza_Qt6-Debug/uart_emulator.exe"
    GENERATE_QT_CONF
)
