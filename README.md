# myftp

Epitech project (G-NWP-400). An FTP server written in C++20 on top of raw sockets.

## Build

```
make
```

## Usage

```
./myftp port path
```

`port` is the TCP port to listen on and `path` is the directory the clients are confined to.

## Supported commands

`USER`, `PASS`, `CWD`, `CDUP`, `PWD`, `LIST`, `RETR`, `STOR`, `DELE`, `PORT`, `PASV`, `HELP`, `NOOP`, `QUIT`. Both active (`PORT`) and passive (`PASV`) data transfers are handled.

## Layout

- `src/Server.cpp`: connection handling and command dispatch
- `src/Client.cpp`: per-client state
