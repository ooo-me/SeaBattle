add_executable(server
    main.cpp
    GameModel.cpp
)

# Boost and nlohmann_json already found in top-level CMakeLists
target_link_libraries(server PRIVATE
    Boost::headers
    nlohmann_json::nlohmann_json
)
