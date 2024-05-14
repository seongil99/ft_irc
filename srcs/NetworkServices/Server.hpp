/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 17:59:41 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/14 13:50:01 by seonyoon         ###   ########.fr       */
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

    void RemoveClientFromChannel(
        int client_socket,
        std::map<std::string, Channel>::iterator channel_iter);

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

    void RemoveClientFromServer(int client_socket);

    /* Channel functions */

    void CreateChannel(const std::string &channel_name);
    void AddClientToChannel(Client &client, const std::string &channel_name);
    void RemoveClientFromChannel(int client_socket,
                                 const std::string &channel_name);
    void AddChannelOwner(Client &client, const std::string &channel_name);
	void AddChannelOwner(const std::string &client_name, 
						 const std::string &channel_name);
    void RemoveChannelOwner(Client &client, const std::string &channel_name);
    bool HasChannel(const std::string &channel_name);
    bool HasClientInChannel(int client_socket, const std::string &channel_name);
    void SendMessageToAllClientsInChannel(const std::string &channel_name,
                                          const std::string &message);
    void SendMessageToOthersInChannel(int client_socket,
                                      const std::string &channel_name,
                                      const std::string &message);
    void SendMessageToOtherClient(int sender_socket,
                                  const std::string &receiver_nickname,
                                  const std::string &message);
    // 채널 password, invite only 관련 함수 추가
    bool HasChannelPassword(const std::string &channel_name);
    bool CheckChannelPassword(const std::string &password_input,
                              const std::string &channel_name);
    bool IsInvitedChannel(int client_socket, const std::string &channel_name);
    bool IsOverUsersLimitChannel(const std::string &channel_name);
	bool HasModeInChannel(const char mode, const std::string &channel_name);
    void SetModeToChannel(const char mode, const std::string &channel_name);
    void RemoveModeFromChannel(const char mode,
                               const std::string &channel_name);
	void SetPasswordInChannel(const std::string &passwd, 
							const std::string &channel_name);
	void SetUsersLimitInChannel(size_t limit, 
								const std::string &channel_name);
    bool IsChannelOwner(int client_socket, const std::string &channel_name);

    const std::string getAllChannelName();
    /**
     * 사이드이펙트 발생 가능성이 있어서 사용하지 않는 것이 좋아보임.
     * 이걸 사용해야 하는 로직이 있다면 Server 메소드로 추가할 예정.
     */
    Channel *getChannel(const std::string &channel_name);

    /* Client functions */

    void PushSendQueueClient(int client_socket, const std::string &message);
    bool HasDuplicateNickname(const std::string &nickname);
    size_t HowManyChannelsAre() const;
	size_t HowManyChannelsJoined(int client_socket);
    size_t HowManyClientsAre() const;
    size_t HowManyClientsAreInChannel(const std::string &channel_name);
    size_t GetUsersLimitInChannel(const std::string &channel_name);
	const std::string ClientsInChannelList(const std::string &channel_name);

    /* Getter */

    const std::string &getStartedTime() const;
};

#endif
