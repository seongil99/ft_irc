/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 17:59:41 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/08 18:59:05 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <sys/event.h>

#include <map>
#include <string>
#include <vector>

#include "Client.hpp"

// 보류 중인 연결 큐의 최대 길이
#define BACKLOG_SIZE 5

class Server {
  private:
    int server_socket_;
    int kq_;
    struct sockaddr_in server_addr_;
    std::map<int, Client> clients_;
    std::vector<struct kevent> change_list_;
    struct kevent event_list_[8];

    void ChangeEvents(uintptr_t ident, int16_t filter, uint16_t flags,
                      uint32_t fflags, intptr_t data, void *udata);
    void CloseClient(int client_fd);
    void EventRead(struct kevent *cur_event);
    void EventWrite(struct kevent *cur_event);
    void EventError(struct kevent *cur_event);

    Server(const Server &ref);
    Server &operator=(const Server &ref);

  public:
    Server(void);
    ~Server(void);

    void Init(int port);
    void Listen(void);
};

#endif
