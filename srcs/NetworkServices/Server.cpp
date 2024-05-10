/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 18:03:26 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/09 15:35:54 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Server.hpp"
#include "utils.hpp"

Server::Server(void) : cmd(this) {
    server_socket_ = 0;
    kq_ = 0;
    memset(&server_addr_, 0, sizeof(server_addr_));
}

Server::~Server(void) {}

/**
 * Init Server
 * @param port port number
 */
void Server::Init(int port) {
    server_socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_ == -1)
        irc_utils::ExitWithError("socket() error");
    server_addr_.sin_family = AF_INET;                // IPV4 protocol
    server_addr_.sin_addr.s_addr = htonl(INADDR_ANY); // 32bit IPV4 addr
    server_addr_.sin_port = htons(port);              // port number
    int bind_result = bind(server_socket_, (struct sockaddr *)(&server_addr_),
                           sizeof(server_addr_));
    if (bind_result == -1)
        irc_utils::ExitWithError("bind() error");
    kq_ = kqueue();
    if (kq_ == -1)
        irc_utils::ExitWithError("kqueue() error");
    int fcntl_result = fcntl(server_socket_, F_SETFL, O_NONBLOCK);
    if (fcntl_result == -1)
        irc_utils::ExitWithError("fcntl() error");
    std::cout << "server port " << port << std::endl;

    /* Test Default Channel */
    channels_["default"] = Channel("default");
}

/**
 * Listen tcp connection from client
 * call Init() before Listen()
 */
void Server::Listen(void) {
    if (server_socket_ == 0)
        return;
    int listen_result = listen(server_socket_, BACKLOG_SIZE);
    if (listen_result == -1)
        irc_utils::ExitWithError("listen() error");
    ChangeEvents(server_socket_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    std::cout << "echo server started" << std::endl;

    /* main loop */
    int new_events;
    struct kevent *curr_event;
    while (true) {
        /*  apply changes and return new events(pending events) */
        new_events = kevent(kq_, &change_list_[0], change_list_.size(),
                            event_list_, 8, NULL);
        if (new_events == -1)
            irc_utils::ExitWithError("kevent() error\n");

        change_list_.clear(); // clear change_list_ for new changes

        for (int i = 0; i < new_events; ++i) {
            curr_event = &event_list_[i];

            /* check error event return */
            if (curr_event->flags & EV_ERROR) {
                EventError(curr_event);
            } else if (curr_event->filter == EVFILT_READ) {
                EventRead(curr_event);
            } else if (curr_event->filter == EVFILT_WRITE) {
                EventWrite(curr_event);
            }
        }
    }
}

void Server::EventError(struct kevent *curr_event) {
    if (curr_event->ident == (unsigned long)server_socket_)
        irc_utils::ExitWithError("server socket error");
    else {
        std::cerr << "client socket error" << std::endl;
        CloseClient(curr_event->ident);
    }
}

void Server::EventRead(struct kevent *curr_event) {
    if (curr_event->ident == (unsigned long)server_socket_) {
        /* accept new client */
        int client_socket;
        if ((client_socket = accept(server_socket_, NULL, NULL)) == -1)
            irc_utils::ExitWithError("accept() error\n");
        std::cout << "accept new client: " << client_socket << std::endl;
        fcntl(client_socket, F_SETFL, O_NONBLOCK);

        /* add event for client socket - add read && write event */
        ChangeEvents(client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
                     NULL);
        ChangeEvents(client_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0,
                     NULL);
        clients_[client_socket] = Client(client_socket);
        /* Test add to default channel */
        channels_["default"].AddClient(clients_[client_socket]);
        channels_["default"].SendMessageToAllClients(
            "new client to default channel!");
    } else if (clients_.find(curr_event->ident) != clients_.end()) {
        /* read data from client */
        char buf[BUF_SIZE];
        int n = read(curr_event->ident, buf, sizeof(buf));

        if (n <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                return;
            else if (n < 0)
                std::cerr << "client read error!" << std::endl;
            CloseClient(curr_event->ident);
        } else {
            buf[n] = '\0';
            ProcessReceivedData(curr_event->ident, buf, n);
        }
    }
}

void Server::EventWrite(struct kevent *curr_event) {
    /* send data to client */
    std::map<int, Client>::iterator it = clients_.find(curr_event->ident);
    if (it != clients_.end() && (*it).second.getSendQueueSize()) {
        std::string str_to_client = (*it).second.PopSendQueue();
        int n = write(curr_event->ident, str_to_client.c_str(),
                      str_to_client.size());
        if (n == -1) {
            std::cerr << "client write error!" << std::endl;
            CloseClient(curr_event->ident);
        }
    }
}

void Server::ProcessReceivedData(int client_socket, char buf[BUF_SIZE], int n) {
    (void)n;
	//메시지 받고
    clients_[client_socket].setMessage(std::string("") + buf);
	
	if (cmd.excute(&clients_[client_socket], std::string(buf)) == false)
	{
		//일반 채팅문일 경우
		//메시지 앞에 추가 문장 달고
    	std::cout << "received data from " << client_socket << ": "
              << clients_[client_socket].getMessage() << std::endl;

		//일단은 모든 클라이언트한테 쏴주기
    	channels_["default"].SendMessageToAllClients(
	        clients_[client_socket].getMessage());
	}
 	//===================================================================================================

    clients_[client_socket].setMessage("");//버퍼 초기화
}

/**
 * change properties of the event
 * @param ident client socket descriptor
 * @param filter 어떤 이벤트를 설정할 것인지 READ, WRITE 등
 * @param flags 필터를 설정하는 플래그 ADD, ENABLE, ONSHOT 등
 */
void Server::ChangeEvents(uintptr_t ident, int16_t filter, uint16_t flags,
                          uint32_t fflags, intptr_t data, void *udata) {
    struct kevent temp_event;
    EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
    change_list_.push_back(temp_event);
}

/**
 * Close connection of the client
 * @param client_fd client socket descriptor
 */
void Server::CloseClient(int client_fd) {
    std::cout << "client disconnected: " << client_fd << std::endl;
    close(client_fd);
    clients_.erase(client_fd);
}

void Server::CreateChannel(const std::string &channel_name) {
    Channel new_channel(channel_name);
    channels_[channel_name] = new_channel;
}

void Server::AddClientToChannel(Client &client,
                                const std::string &channel_name) {
    std::map<std::string, Channel>::iterator it;
    it = channels_.find(channel_name);
    if (it != channels_.end())
        (*it).second.AddClient(client);
}
