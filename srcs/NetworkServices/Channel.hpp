/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:16 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/13 16:08:00 by seonyoon         ###   ########.fr       */
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
    Client *owner_;
    std::set<char> mode_;
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

    bool HasClient(int client_socket);
    bool HasClient(const std::string &nickname);
    bool HasMode(char mode);

    void AddMode(char c);
    void RemoveMode(char c);

    /* Getter */

    const std::string &getChannelName(void) const;
    Client *getOwner(void) const;
    int getClientCount(void) const;
    const std::string getModes(void) const;
    const std::string &getTopic(void) const;

    /* Setter */

    void setChannelName(const std::string &channel_name);
    void setOwner(Client *client);
    void setTopic(const std::string &topic);

    /**
     * 사이드이펙트 발생 가능성이 있어서 사용하지 않는 것이 좋아보임.
     * 이걸 사용해야 하는 로직이 있다면 Server 메소드로 추가할 예정.
     */
    Client *getJoinedClient(const std::string &nickname);
};

#endif
