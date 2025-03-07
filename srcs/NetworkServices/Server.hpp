/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 17:59:41 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/21 19:11:58 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <sys/event.h>

#include <map>
#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Command.hpp"

// 보류 중인 연결 큐의 최대 길이
#define BACKLOG_SIZE 5
#define BUF_SIZE 1024

/* typedefs */

typedef std::map<int, Client>::iterator clients_iter;
typedef std::map<std::string, Channel>::iterator channels_iter;
typedef std::map<int, Client>::const_iterator const_clients_iter;
typedef std::map<std::string, Channel>::const_iterator const_channels_iter;

class Server {
  private:
    int server_socket_;
    int kq_;
    struct sockaddr_in server_addr_;
    std::string passwd_;

    std::vector<struct kevent> change_list_;
    struct kevent event_list_[8];

    std::map<std::string, Channel> channels_;
    std::map<int, Client> clients_;

    Command cmd;
    std::string started_time_;

    /* Events functions */

    void ChangeEvents(uintptr_t ident, int16_t filter, uint16_t flags,
                      uint32_t fflags, intptr_t data, void *udata);
    void CloseClient(int client_fd);
    void EventRead(struct kevent *cur_event);
    void EventWrite(struct kevent *cur_event);
    void EventError(struct kevent *cur_event);

    /* Channel functions */

    bool RemoveClientFromChannel(
        int client_socket,
        std::map<std::string, Channel>::iterator &channel_iter);

    /* Client functions */

    clients_iter FindClientByNickname(const std::string &nickname);

    /* Process Data received from tcp */
    void ProcessReceivedData(int client_socket, char buf[BUF_SIZE], int n);

    /* Unnecessary functions */

    Server(const Server &ref);
    Server &operator=(const Server &ref);

  public:
    Server(void);
    ~Server(void);

    /* Server functions */

    void Init(int port, std::string passwd);
    void Listen(void);

    bool CheckPassword(const std::string &password_input) const;
    bool HasPassword(void) const;

    void RemoveClientFromServer(int client_socket);

    /* Channel functions */

    void CreateChannel(const std::string &channel_name);
    void AddClientToChannel(Client &client, const std::string &channel_name);
    void RemoveClientFromChannel(int client_socket,
                                 const std::string &channel_name);
    void AddChannelOwner(Client &client, const std::string &channel_name);
    void AddChannelOwner(const std::string &client_nickname,
                         const std::string &channel_name);
    void RemoveChannelOwner(Client &client, const std::string &channel_name);
    void RemoveChannelOwner(const std::string &client_nickname,
                            const std::string &channel_name);
    bool HasChannel(const std::string &channel_name) const;
    bool HasClientInChannel(int client_socket,
                            const std::string &channel_name) const;
    bool HasClientInChannel(const std::string &client_nickname,
                            const std::string &channel_name);
    void SendMessageToAllClientsInChannel(const std::string &channel_name,
                                          const std::string &message);
    void SendMessageToOthersInChannel(int client_socket,
                                      const std::string &channel_name,
                                      const std::string &message);
    void SendMessageToOtherClient(int sender_socket,
                                  const std::string &receiver_nickname,
                                  const std::string &message);
    void SendMessageToAllJoinedChannel(int client_socket,
                                       const std::string &message);

    bool HasChannelPassword(const std::string &channel_name) const;
    bool CheckChannelPassword(const std::string &password_input,
                              const std::string &channel_name) const;
    bool IsInvitedChannel(int client_socket, const std::string &channel_name);
    bool IsOverUsersLimitChannel(const std::string &channel_name);
    bool HasModeInChannel(const char mode,
                          const std::string &channel_name) const;
    void SetModeToChannel(const char mode, const std::string &channel_name);
    void RemoveModeFromChannel(const char mode,
                               const std::string &channel_name);
    const std::string GetModeFromChannel(const std::string &channel_name);
    void SetPasswordInChannel(const std::string &passwd,
                              const std::string &channel_name);
    void SetUsersLimitInChannel(size_t limit, const std::string &channel_name);
    bool IsChannelOwner(int client_socket,
                        const std::string &channel_name) const;
    void AddInviteClient(const std::string &channel_name,
                         const std::string &nick_name);
    void RemoveInviteClient(const std::string &channel_name,
                            const std::string &nick_name);
    std::string GetChannelStartedTime(const std::string &channel_name);

    // 채널 Topic 관련 함수
    bool HasTopicInChannel(const std::string &channel_name);
    std::string GetTopicInChannel(const std::string &channel_name);
    std::string WhoDidTopicInChannel(const std::string &channel_name);
    std::string WhatTimeChannelMade(const std::string &channel_name);
    void SetTopicInChannel(const std::string &channel_name,
                           const std::string &topic,
                           const std::string &who_did);

    // list 관련 함수
    void ActivateList(Client *client);
    void ActivateList(Client *client, const std::string &channel_name);

    const std::string getAllChannelName() const;

    /* Client functions */

    void PushSendQueueClient(int client_socket, const std::string &message);
    bool HasDuplicateNickname(const std::string &nickname) const;
    size_t HowManyChannelsAre() const;
    size_t HowManyChannelsJoined(int client_socket);
    size_t HowManyClientsAre() const;
    size_t HowManyClientsAreInChannel(const std::string &channel_name) const;
    size_t GetUsersLimitInChannel(const std::string &channel_name);
    const std::string ClientsInChannelList(const std::string &channel_name);
    void CorrectPassword(Client *client);

    /* Getter */

    const std::string &getStartedTime() const;
    int getClientSocket(const std::string &nick_name);
};

#endif
