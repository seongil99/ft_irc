/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:16 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/21 19:17:16 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <set>
#include <string>

class Client;

const std::string kChannelModes = "itkol";

class Channel {
  private:
    std::string channel_name_;
    std::map<int, Client *> clients_;
    std::map<int, Client *> owners_;
    std::map<int, Client *> invited_clients_;
    std::string passwd_;
    size_t users_limit_;
    std::set<char> mode_; // i t k o l
    std::string topic_;
    std::string topic_set_time_;
    std::string topic_who_did_;
    std::string started_time_;

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
    bool HasPassword(void) const;

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
    const std::string &getTopicWhoDid(void) const;
    const std::string &getTopicSetTime(void) const;
    const std::string &getStartedTime(void) const;

    /* Setter */

    void setChannelName(const std::string &channel_name);
    void setUsersLimit(size_t limit);
    void setTopic(const std::string &topic, const std::string &who_did);
    void setPassword(const std::string &passwd);
    bool CheckPassword(const std::string &passwd) const;

    // channel mode 확인 관련 함수
    size_t getUsersLimit() const;
    bool IsInvited(int client_socket) const;

    bool IsOwner(int client_socket) const;
    const std::string ClientsList(void) const;
};

#endif
