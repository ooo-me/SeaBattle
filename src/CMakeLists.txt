# GUI application (only if Qt6 is found)
if(Qt6_FOUND)
    qt_add_executable(SeaBattle WIN32
        "BattleField.cpp"
        "BattleField.h"
        "GameScreen.cpp"
        "GameScreen.h"
        "main.cpp"
        "MainWindow.cpp"
        "MainWindow.h"
        "Model.h"
        "ModelAdapter.cpp"
        "ModelAdapter.h"
        "pch.h"
        "WelcomeScreen.cpp"
        "WelcomeScreen.h"
    )

    target_precompile_headers(SeaBattle PRIVATE
        pch.h
    )

    target_link_libraries(SeaBattle PRIVATE
        Qt::Core
        Qt::Widgets
    )

    install(TARGETS SeaBattle
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Server executable (no GUI/Qt dependencies)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(SeaBattleServer
	"GameServer.cpp"
	"GameServer.h"
	"GameProtocol.h"
	"Model.h"
	"server_main.cpp"
)

target_link_libraries(SeaBattleServer PRIVATE
	Boost::system
)

target_compile_features(SeaBattleServer PRIVATE cxx_std_20)

install(TARGETS SeaBattleServer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(Qt6_FOUND)
    set(deploy_tool_options_arg
        --no-system-d3d-compiler
        --no-system-dxc-compiler
    )

    qt_generate_deploy_app_script(
        TARGET SeaBattle
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
    )
    install(SCRIPT ${deploy_script})
endif()
