#pragma once

namespace SeaBattle::Network
{
    /**
     * @brief Protocol constants and configuration values
     */
    namespace ProtocolConstants
    {
        // Protocol version
        constexpr const char* PROTOCOL_VERSION = "1.0";

        // Game field constants
        constexpr int FIELD_SIZE = 10;
        constexpr int MIN_COORDINATE = 0;
        constexpr int MAX_COORDINATE = FIELD_SIZE - 1;

        // Player constants
        constexpr int MIN_PLAYER_ID = 0;
        constexpr int MAX_PLAYER_ID = 1;
        constexpr int MAX_PLAYERS = 2;

        // Message constraints
        constexpr int MAX_CHAT_MESSAGE_LENGTH = 500;

        // Network constants
        constexpr int DEFAULT_MAX_CONNECTIONS = MAX_PLAYERS;
        constexpr int DEFAULT_PORT = 7777;

        /**
         * @brief Check if coordinate is valid
         * @param coord Coordinate to check
         * @return true if coordinate is within valid range
         */
        constexpr bool isValidCoordinate(int coord)
        {
            return coord >= MIN_COORDINATE && coord <= MAX_COORDINATE;
        }

        /**
         * @brief Check if player ID is valid
         * @param playerId Player ID to check
         * @return true if player ID is valid
         */
        constexpr bool isValidPlayerId(int playerId)
        {
            return playerId >= MIN_PLAYER_ID && playerId <= MAX_PLAYER_ID;
        }

    } // namespace ProtocolConstants

} // namespace SeaBattle::Network
