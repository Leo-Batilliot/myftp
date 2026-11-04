/*
** EPITECH PROJECT, 2026
** G-NWP-400-LIL-4-1-myftp-26
** File description:
** main
*/

#include "Server.hpp"
#include <iostream>
#include <string>

int main(int ac, char **av)
{
    if (ac != 3) {
        std::cerr << "USAGE: ./myftp port path" << std::endl;
        return 84;
    }

    try {
        Server server(std::stoi(av[1]), av[2]);
        server.run();
    } catch (...) {
        return 84;
    }
    return 0;
}