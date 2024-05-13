/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:16 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/11 18:10:41 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <string>

#include <iostream>

class Client;

class Channel {
  private:
    std::string channel_name_;
    std::map<int, Client *> clients_;
    Client *owner_;
	std::map<int, Client *> owners_; //한 채널에 운영자가 여러명 있을 수 있어 map으로 바꿔야 할 듯
	bool invite_only_;
	std::map<int, Client *> invited_clients_; //운영자가 초대 -> 초대받은 사람이 join으로 가입
	bool topic_limit_;
	std::string passwd_;
	int users_limit_;

  public:
    Channel(void);
    Channel(const Channel &ref);
    Channel(const std::string &channel_name);
    ~Channel(void);

    Channel &operator=(const Channel &ref);

    void AddClient(Client &client);
    void RemoveClient(const Client &client);
    void RemoveClient(int client_socket);

    void SendMessageToAllClients(const std::string &message);
    void SendMessageToOthers(int sender_socket, const std::string &message);

    const std::string &getChannelName(void) const;
    Client *getOwner(void) const;

    void setChannelName(const std::string &channel_name);
    void setOwner(Client *client);

    bool HasClient(int client_socket);
    bool HasClient(const std::string &nickname);
    /**
     * 사이드이펙트 발생 가능성이 있어서 사용하지 않는 것이 좋아보임.
     * 이걸 사용해야 하는 로직이 있다면 Server 메소드로 추가할 예정.
     */
    Client *getJoinedClient(const std::string &nickname);

	//channel mode 확인 관련 함수
	bool IsInviteOnly();
	bool HasTopicLimit();
	std::string getPassword();
	int	getUsersLimit();
	bool IsInvited(int client_socket);
};

#endif
