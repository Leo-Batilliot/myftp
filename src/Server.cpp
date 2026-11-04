/*
** EPITECH PROJECT, 2026
** G-NWP-400-LIL-4-1-myftp-26
** File description:
** Server
*/

#include "Server.hpp"
#include "Config.hpp"
#include <iostream> 
#include <algorithm> 
#include <cstdio> 
#include <cstring> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <stdlib.h>

Server::Server(int port, const std::string& path)
{
    char actualPath[MAX_PATH_SIZE];
    if (realpath(path.c_str(), actualPath) == NULL) {
        perror("realpath");
        exit(84);
    }
    _rootPath = std::string(actualPath);
    
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(84);
    }
    listen(_serverFd, 10);
    _pollFds.push_back({_serverFd, POLLIN, 0});
}

Server::~Server()
{
    close(_serverFd);
}

void Server::run()
{
    while (true) {
        while (waitpid(-1, NULL, WNOHANG) > 0);

        if (poll(_pollFds.data(), _pollFds.size(), -1) < 0) {
            if (errno == EINTR) 
                continue;
            break;
        }

        for (size_t i = 0; i < _pollFds.size(); ++i) {
            if (_pollFds[i].revents & POLLIN) {
                if (_pollFds[i].fd == _serverFd) 
                    NewConnection();
                else 
                    ReadClient(i);
            }
        }
    }
}

void Server::NewConnection()
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int newSock = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    
    if (newSock != -1) {
        if (_pollFds.size() >= MAX_CLIENTS) {
            const char *msg = "421 Service not available.\r\n";
            write(newSock, msg, strlen(msg));
            close(newSock);
            return;
        }
        _pollFds.push_back({newSock, POLLIN, 0});
        _clients[newSock] = std::make_unique<Client>(newSock, _rootPath);
        _clients[newSock]->sendResponse(220, "Service ready for new user.");
    }
}

void Server::ReadClient(int index)
{
    char buffer[BUFFER_SIZE];
    int fd = _pollFds[index].fd;
    ssize_t n = read(fd, buffer, BUFFER_SIZE);

    if (n <= 0) {
        _clients.erase(fd);
        _pollFds.erase(_pollFds.begin() + index);
        return;
    }

    Client& client = *_clients[fd];
    client.internalBuffer.append(buffer, n);

    size_t pos;
    while ((pos = client.internalBuffer.find("\n")) != std::string::npos)
    {
        std::string command = client.internalBuffer.substr(0, pos);
        client.internalBuffer.erase(0, pos + 1);
        if (!command.empty() && command.back() == '\r')
            command.pop_back();
        if (!command.empty())
            processCommand(client, command);
    }
}

void Server::processCommand(Client& client, std::string buffer) {
    if (buffer.empty())
        return;

    size_t spacePos = buffer.find(' ');
    std::string cmd = buffer.substr(0, spacePos);
    std::string arg = (spacePos != std::string::npos) ? buffer.substr(spacePos + 1) : "";
    while (!arg.empty() && (arg.back() == '\r' || arg.back() == '\n'))
        arg.pop_back();

    if (cmd == "USER") cmdUser(client, arg);
    else if (cmd == "PASS") cmdPass(client);
    else if (cmd == "QUIT") cmdQuit(client);
    else if (cmd == "NOOP") client.sendResponse(200, "NOOP ok.");
    else if (cmd == "HELP") client.sendResponse(214, "Commands: USER PASS PWD CWD CDUP PASV RETR STOR DELE LIST QUIT NOOP HELP");
    else if (cmd == "PWD") cmdPwd(client);
    else if (cmd == "CWD") cmdCwd(client, arg);
    else if (cmd == "CDUP") cmdCwd(client, "..");
    else if (cmd == "PORT") cmdPort(client, arg);
    else if (cmd == "PASV") cmdPasv(client);
    else if (cmd == "RETR") cmdRetr(client, arg);
    else if (cmd == "STOR") cmdStor(client, arg);
    else if (cmd == "DELE") cmdDele(client, arg);
    else if (cmd == "LIST") cmdList(client, arg);
    else client.sendResponse(500, "Unknown command.");
}

void Server::cmdUser(Client& client, std::string arg) {
    if (arg.empty()) {
        client.sendResponse(501, "Syntax error in parameters or arguments.");
        return;
    }
    client.username = arg;
    client.userEntered = true;
    client.sendResponse(331, "User name okay, need password.");
}

void Server::cmdPass(Client& client)
{
    if (client.userEntered && client.username == "Anonymous") {
        client.isAuthenticated = true;
        client.sendResponse(230, "User logged in, proceed.");
    } else {
        client.sendResponse(530, "Login incorrect.");
    }
}

void Server::cmdPasv(Client& client) {
    if (!client.isAuthenticated)
        return client.sendResponse(530, "Please login.");
    if (client.dataFd != -1)
        close(client.dataFd);

    client.dataFd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    bind(client.dataFd, (struct sockaddr*)&addr, len);
    listen(client.dataFd, 1);
    getsockname(client.dataFd, (struct sockaddr*)&addr, &len);

    int p = ntohs(addr.sin_port);
    client.isPassive = true;
    client.isActive = false;
    client.sendResponse(227, "Entering Passive Mode (127,0,0,1," + std::to_string(p/256) + "," + std::to_string(p%256) + ").");
}

void Server::cmdPort(Client& client, std::string arg) {
    if (!client.isAuthenticated)
        return client.sendResponse(530, "Please login.");
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(arg.c_str(), "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6)
        return client.sendResponse(501, "Syntax error.");

    client.isActive = true;
    client.isPassive = false;
    client.activeAddr.sin_family = AF_INET;
    std::string ip = std::to_string(h1)+"."+std::to_string(h2)+"."+std::to_string(h3)+"."+std::to_string(h4);
    client.activeAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    client.activeAddr.sin_port = htons(p1 * 256 + p2);
    client.sendResponse(200, "PORT command successful.");
}

int Server::establishDataConnection(Client& client) {
    if (client.isPassive) {
        int fd = accept(client.dataFd, NULL, NULL);
        close(client.dataFd); client.dataFd = -1;
        return fd;
    } else if (client.isActive) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(fd, (struct sockaddr*)&client.activeAddr, sizeof(client.activeAddr)) < 0) {
            close(fd); return -1;
        }
        return fd;
    }
    return -1;
}

void Server::cmdRetr(Client& client, std::string arg) {
    if (!client.isAuthenticated)
        return client.sendResponse(530, "Please login.");
    std::string path = client.getAbsolutePath(arg);
    struct stat s;
    if (stat(path.c_str(), &s) != 0 || S_ISDIR(s.st_mode) || access(path.c_str(), R_OK) != 0)
        return client.sendResponse(550, "File error.");

    client.sendResponse(150, "Opening binary mode data connection.");
    if (fork() == 0) {
        signal(SIGPIPE, SIG_IGN);
        int dSock = establishDataConnection(client);
        int fileFd = open(path.c_str(), O_RDONLY);
        if (dSock != -1 && fileFd != -1) {
            char buf[BUFFER_SIZE];
            ssize_t r;
            while ((r = read(fileFd, buf, BUFFER_SIZE)) > 0) {
                if (send(dSock, buf, r, 0) < 0)
                    break;
            }
        }
        if (fileFd != -1)
            close(fileFd);
        if (dSock != -1)
            close(dSock);
        client.sendResponse(226, "Transfer complete.");
        _exit(0);
    }
    if (client.dataFd != -1) {
        close(client.dataFd);
        client.dataFd = -1;
    }
}

void Server::cmdStor(Client& client, std::string arg) {
    if (!client.isAuthenticated)
        return client.sendResponse(530, "Please login.");
    if (arg.empty())
        return client.sendResponse(501, "No filename.");

    std::string path = client.getAbsolutePath(arg);
    struct stat s;

    if (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
        return client.sendResponse(550, "Target is a directory.");

    if (access(path.c_str(), F_OK) == 0 && access(path.c_str(), W_OK) != 0)
        return client.sendResponse(550, "Existing file is read-only.");

    std::string dir = path.substr(0, path.find_last_of('/'));
    if (dir.empty())
        dir = ".";
    if (access(dir.c_str(), W_OK) != 0)
        return client.sendResponse(550, "Permission denied in parent directory.");

    client.sendResponse(150, "Ok to send data.");

    if (fork() == 0) {
        signal(SIGPIPE, SIG_IGN);
        int dSock = establishDataConnection(client);
        int fileFd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dSock != -1 && fileFd != -1) {
            char buf[BUFFER_SIZE];
            ssize_t r;
            while ((r = read(dSock, buf, BUFFER_SIZE)) > 0) {
                if (write(fileFd, buf, r) < 0)
                    break;
            }
        }
        if (fileFd != -1)
            close(fileFd);
        if (dSock != -1)
            close(dSock);
        client.sendResponse(226, "Transfer complete.");
        _exit(0);
    }
    if (client.dataFd != -1) {
        close(client.dataFd);
        client.dataFd = -1;
    }
}

void Server::cmdList(Client& client, std::string arg) {
    if (!client.isAuthenticated)
        return client.sendResponse(530, "Please login.");
    if (!client.isPassive && !client.isActive)
        return client.sendResponse(425, "Use PASV or PORT first.");

    std::string path = client.getAbsolutePath(arg);
    struct stat s;
    if (stat(path.c_str(), &s) != 0 || !S_ISDIR(s.st_mode))
        return client.sendResponse(550, "Directory not found.");

    client.sendResponse(150, "Here comes the directory listing.");
    if (fork() == 0) {
        signal(SIGPIPE, SIG_IGN);
        int dSock = establishDataConnection(client);
        if (dSock != -1) {
            std::string cmd = "ls -la " + path;
            FILE *pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                char lsBuf[BUFFER_SIZE];
                ssize_t n;
                while ((n = fread(lsBuf, 1, sizeof(lsBuf), pipe)) > 0) {
                    if (send(dSock, lsBuf, n, 0) < 0)
                        break;
                }
                pclose(pipe);
            }
            close(dSock);
        }
        client.sendResponse(226, "Directory send OK.");
        _exit(0);
    }
    if (client.dataFd != -1) {
        close(client.dataFd);
        client.dataFd = -1;
    }
    client.isPassive = false;
    client.isActive = false;
}


void Server::cmdDele(Client& client, std::string arg)
{
    if (!client.isAuthenticated) {
        client.sendResponse(530, "Please login with USER and PASS.");
        return;
    }
    std::string path = client.getAbsolutePath(arg);

    if (remove(path.c_str()) == 0)
        client.sendResponse(250, "File deleted.");
    else
        client.sendResponse(550, "Delete operation failed.");
}


void Server::cmdPwd(Client& client)
{
    if (!client.isAuthenticated) {
        client.sendResponse(530, "Please login with USER and PASS.");
        return;
    }
    client.sendResponse(257, "\"" + client.currentPath + "\" is the current directory.");
}


void Server::cmdCwd(Client& client, std::string arg) {
    if (!client.isAuthenticated)
        return client.sendResponse(530, "Please login.");

    if (arg.empty())                             
        return client.sendResponse(501, "Syntax error in parameters.");

    if ((arg == ".." || arg == "../") && client.currentPath == "/")
        return client.sendResponse(250, "Already at root.");

    std::string target = client.getAbsolutePath(arg);
    struct stat s;

    if (!target.empty() && stat(target.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) {
        if (access(target.c_str(), X_OK) != 0)
            return client.sendResponse(550, "Permission denied.");

        std::string virtualPath = target;
        if (virtualPath.find(_rootPath) == 0)
            virtualPath.erase(0, _rootPath.length());
        if (virtualPath.empty() || virtualPath[0] != '/')
            virtualPath = "/" + virtualPath;
        client.currentPath = virtualPath;
        client.sendResponse(250, "Directory successfully changed.");
    } else {
        client.sendResponse(550, "Failed to change directory.");
    }
}

void Server::cmdQuit(Client& client)
{
    client.sendResponse(221, "Goodbye.");
    shutdown(client.fd, SHUT_WR);
}