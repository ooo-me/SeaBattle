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
	"Protocol.h"
	"NetworkSession.h"
	"ServerSession.h"
	"ClientSession.h"
	"NetworkAdapter.h"
	"ServerSettingsDialog.h"
	"ClientSettingsDialog.h"
)

target_precompile_headers(SeaBattle PRIVATE
	pch.h
)

target_link_libraries(SeaBattle PRIVATE
	Qt::Core
	Qt::Widgets
	Boost::headers
	nlohmann_json::nlohmann_json
)

install(TARGETS SeaBattle
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

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
