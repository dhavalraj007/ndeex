
#include <iostream>
#include "Engine.hpp"
#include <print>

int main()
{
    try
    {
        Core::Engine engine;
        engine.gameloop();
    }
    catch (std::exception const &exception)
    {
        std::println("exception thrown was not handled: \n what():{}", exception.what());
    }
    return 0;
}