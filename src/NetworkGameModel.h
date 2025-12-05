#pragma once

#include "Model.h"

namespace SeaBattle
{
    // Simplified game model for network play
    // Each player only manages their own field
    class NetworkGameModel
    {
    public:
        NetworkGameModel()
        {
            // Place ships on our field
            if (!ShipPlacer::autoPlaceShips(myField))
            {
                throw std::runtime_error("Failed to place ships");
            }
        }

        // Process opponent's shot on our field
        // Returns true if hit, false if miss
        bool processOpponentShot(int row, int col, bool& destroyed)
        {
            if (row < 0 || row >= GameField::SIZE || col < 0 || col >= GameField::SIZE)
            {
                return false;
            }

            CellState state = myField.getCellState(row, col);
            
            // Can't shoot already shot cells
            if (state == CellState::Miss || state == CellState::Hit || state == CellState::Destroyed)
            {
                return false;
            }

            bool hit = myField.shoot(row, col);
            
            if (hit)
            {
                // Check if ship was destroyed
                CellState newState = myField.getCellState(row, col);
                destroyed = (newState == CellState::Destroyed);
            }
            else
            {
                destroyed = false;
            }

            return hit;
        }

        // Check if all our ships are destroyed (we lost)
        bool allShipsDestroyed() const
        {
            return myField.allShipsDestroyed();
        }

        // Get our ships (for display)
        const std::vector<Ship>& getMyShips() const
        {
            return myField.getShips();
        }

        // Check if coordinates are within valid range
        // Note: This only validates boundaries, not whether the cell was already shot
        // Enemy field state is tracked separately in MainWindow
        bool isValidCoordinates(int row, int col) const
        {
            if (row < 0 || row >= GameField::SIZE || col < 0 || col >= GameField::SIZE)
            {
                return false;
            }
            return true;
        }

        CellState getMyCellState(int row, int col) const
        {
            return myField.getCellState(row, col);
        }

    private:
        GameField myField;
    };

} // namespace SeaBattle
