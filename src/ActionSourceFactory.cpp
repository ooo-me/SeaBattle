#include "IActionSource.h"
#include "LocalActionSource.h"
#include "NetworkActionSource.h"

namespace SeaBattle
{
    std::unique_ptr<IActionSource> ActionSourceFactory::create(SourceType type, GameModel* model)
    {
        switch (type)
        {
        case SourceType::Local:
            return std::make_unique<LocalActionSource>(model);
        case SourceType::Network:
            return std::make_unique<NetworkActionSource>(model);
        default:
            return std::make_unique<LocalActionSource>(model);
        }
    }

} // namespace SeaBattle
