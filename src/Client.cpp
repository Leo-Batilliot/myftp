/*
** EPITECH PROJECT, 2026
** G-NWP-400-LIL-4-1-myftp-26
** File description:
** Client
*/

#include "Client.hpp"
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "Config.hpp"

Client::Client(int f, const std::string& root) : 
    fd(f), dataFd(-1), isAuthenticated(false), userEntered(false), currentPath("/"), rootPath(root), isPassive(false), isActive(false)
{}

Client::~Client()
{
    if (fd != -1)
        close(fd);
    if (dataFd != -1)
        close(dataFd);
}

void Client::sendResponse(int code, const std::string& message)
{
    std::string res = std::to_string(code) + " " + message + "\r\n";
    write(fd, res.c_str(), res.length());
}

std::string Client::getAbsolutePath(const std::string& arg) {
    std::string base;

    if (arg.empty() || arg == ".") {
        base = rootPath + currentPath;
    } else if (arg[0] == '/') {
        if (arg.find(rootPath) == 0)
            base = arg;
        else
            base = rootPath + arg;
    } else {
        base = rootPath + currentPath;
        if (base.back() != '/') base += "/";
        base += arg;
    }

    for (size_t doubleSlash; (doubleSlash = base.find("//")) != std::string::npos; base.erase(doubleSlash, 1));

    char resolved[MAX_PATH_SIZE];
    if (realpath(base.c_str(), resolved) == NULL) {
        if (base.find(rootPath) == 0)
            return base;
        return ""; 
    }

    std::string finalPath(resolved);
    if (finalPath.compare(0, rootPath.length(), rootPath) != 0)
        return "";
    return finalPath;
}