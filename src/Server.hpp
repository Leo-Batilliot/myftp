/*
** EPITECH PROJECT, 2026
** G-NWP-400-LIL-4-1-myftp-26
** File description:
** Server
*/

#pragma once
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <poll.h>
#include "Client.hpp"

class Server {
public:
    Server(int port, const std::string& path);
    ~Server();
    void run();

private:
    void NewConnection();
    void ReadClient(int index);
    void processCommand(Client& client, std::string cmd);
    int establishDataConnection(Client& client);

    void cmdUser(Client& client, std::string arg);
    void cmdPass(Client& client);
    void cmdPwd(Client& client);
    void cmdCwd(Client& client, std::string arg);
    void cmdPasv(Client& client);
    void cmdPort(Client& client, std::string arg);
    void cmdRetr(Client& client, std::string arg);
    void cmdList(Client& client, std::string arg);
    void cmdQuit(Client& client);
    void cmdDele(Client& client, std::string arg);
    void cmdStor(Client& client, std::string arg);

    int _serverFd;
    std::string _rootPath;
    std::vector<struct pollfd> _pollFds;
    std::map<int, std::unique_ptr<Client>> _clients;
};