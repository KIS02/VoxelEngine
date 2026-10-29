#pragma once


namespace ve
{
    enum class GameMode {
        Creative,
        Survival
    };


    constexpr const char* gameModeName( GameMode mode ) {
        return mode == GameMode::Creative ? "Creative" : "Survival";
    }

    constexpr bool allowsFlight( GameMode mode ) {
        return mode == GameMode::Creative;
    }

    constexpr bool allowsInstantBreak( GameMode mode ) {
        return mode == GameMode::Creative;
    }
}
