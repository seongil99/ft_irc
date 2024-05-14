/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:16 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/14 15:08:30 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <set>
#include <string>

#include <iostream>

class Client;

const std::string kChannelModes = "itkol";

class Channel {
  private:
    std::string channel_name_;
    std::map<int, Client *> clients_;
    // 한 채널에 운영자가 여러명 있을 수 있어 map으로 바꿔야 할 듯
    std::map<int, Client *> owners_;
    // 운영자가 초대 -> 초대받은 사람이 join으로 가입
    std::map<int, Client *> invited_clients_;
    std::string passwd_;
    size_t users_limit_;
    std::set<char> mode_; // i t k o l
    std::string topic_;

  public:
    Channel(void);
    Channel(const Channel &ref);
    Channel(const std::string &channel_name);
    ~Channel(void);

    Channel &operator=(const Channel &ref);

    void AddClient(Client &client);
    void RemoveClient(int client_socket);

    void SendMessageToAllClients(const std::string &message);
    void SendMessageToOthers(int sender_socket, const std::string &message);

    bool HasClient(int client_socket) const;
    bool HasClient(const std::string &nickname) const;
    bool HasMode(char mode) const;

    void AddMode(char mode);
    void RemoveMode(char mode);

    void AddOwner(Client *client);
    void RemoveOwner(int client_socket);

    void AddInvitedList(Client *client);
    void RemoveInvitedList(int client_socket);

    /* Getter */

    const std::string &getChannelName(void) const;
    int getClientCount(void) const;
    const std::string getModes(void) const;
    const std::string &getTopic(void) const;

    /* Setter */

    void setChannelName(const std::string &channel_name);
    void setTopic(const std::string &topic);
	void setUsersLimit(size_t limit);
    void setPassword(const std::string &passwd);
	bool CheckPassword(const std::string &passwd) const;

    /**
     * 사이드이펙트 발생 가능성이 있어서 사용하지 않는 것이 좋아보임.
     * 이걸 사용해야 하는 로직이 있다면 Server 메소드로 추가할 예정.
     */
    Client *getJoinedClient(const std::string &nickname);

    // channel mode 확인 관련 함수
    int getUsersLimit() const;
    bool IsInvited(int client_socket) const;

    bool IsOwner(int client_socket) const;
    const std::string ClientsList(void) const;
};

#endif
