# IRC Server Project

A lightweight Internet Relay Chat (IRC) server implementation in C++98, featuring a robust network infrastructure with non-blocking I/O using kqueue for efficient connection handling.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Building the Project](#building-the-project)
- [Usage](#usage)
- [Supported Commands](#supported-commands)
- [Channel Modes](#channel-modes)
- [Project Structure](#project-structure)
- [Implementation Details](#implementation-details)
- [Contributing](#contributing)

## Overview

This project implements a simple but functional IRC server compatible with standard IRC clients. The server allows multiple clients to connect, join channels, send messages, and perform various IRC operations. The implementation follows the RFC standards for IRC protocol while being written in C++98.

## Features

- Server password protection
- Multiple channel support
- Private messaging between users
- Channel operator privileges
- Various channel modes (invite-only, topic restrictions, etc.)
- User authentication and registration
- Efficient connection handling with kqueue

## Requirements

- C++98 compatible compiler
- UNIX-like operating system with kqueue support (MacOS, FreeBSD)
- Make

## Building the Project

Clone the repository and build using the provided Makefile:

```bash
git clone <repository-url>
cd ircserv
make
```

## Usage

Start the server with an optional password:

```bash
./ircserv <port> [password]
```

- `<port>`: Port number on which the server will listen (1-65535)
- `[password]`: Optional server password for client authentication

Example:
```bash
./ircserv 6667 secretpassword
```

## Supported Commands

The server implements the following IRC commands:

- **PING**: Keep-alive mechanism
- **PASS**: Authenticate with server password
- **NICK**: Set or change your nickname
- **USER**: Register your username, hostname, and real name
- **JOIN**: Join a channel or create a new one
- **PART**: Leave a channel
- **PRIVMSG**: Send messages to users or channels
- **KICK**: Remove a user from a channel (operator only)
- **INVITE**: Invite a user to a channel (operator only)
- **TOPIC**: View or change a channel's topic
- **LIST**: List available channels
- **WHO**: List users in a channel
- **MODE**: Change channel or user modes
- **QUIT**: Disconnect from the server

## Channel Modes

The server supports the following channel modes:

- **i** (invite-only): Users must be invited to join the channel
- **t** (topic protection): Only operators can change the topic
- **k** (key): Channel requires a password to join
- **o** (operator): Grants channel operator status to a user
- **l** (limit): Sets a limit on the number of users in the channel

## Project Structure

```
.
├── Makefile
├── srcs
│   ├── Command             # Command handling
│   │   ├── Command.cpp
│   │   ├── Command.hpp
│   │   ├── reply.cpp
│   │   └── reply.hpp
│   ├── NetworkServices     # Core network functionality
│   │   ├── Channel.cpp
│   │   ├── Channel.hpp
│   │   ├── Client.cpp
│   │   ├── Client.hpp
│   │   ├── Server.cpp
│   │   └── Server.hpp
│   ├── utils               # Utility functions
│   │   ├── utils.cpp
│   │   └── utils.hpp
│   └── main.cpp            # Entry point
```

## Implementation Details

### Server Architecture

The server uses a non-blocking I/O model with kqueue for efficient event handling. This allows it to handle multiple client connections simultaneously without using threads.

### Command Processing

Commands are parsed and processed in the Command class, which interprets IRC protocol messages and takes appropriate actions based on the command and parameters.

### Channel Management

Channels are represented by the Channel class, which maintains user lists, topics, and various mode settings. Each channel can have operators with elevated privileges.

### Client Handling

Each connected client is managed by the Client class, which tracks user information, channel participation, and message queues for sending and receiving data.
