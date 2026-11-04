/*
** EPITECH PROJECT, 2026
** G-NWP-400-LIL-4-1-myftp-26
** File description:
** Client
*/

#pragma once
#include <string>
#include <netinet/in.h>

class Client {
public:
    Client(int fd, const std::string& root);
    ~Client();

    int fd;
    int dataFd;
    bool isAuthenticated;
    bool userEntered;
    std::string username;
    std::string currentPath;
    std::string rootPath;
    struct sockaddr_in dataAddr;
    bool isPassive;
    struct sockaddr_in activeAddr;
    bool isActive = false;
    std::string internalBuffer;

    void sendResponse(int code, const std::string& message);
    std::string getAbsolutePath(const std::string& arg);
};