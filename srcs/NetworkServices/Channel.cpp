/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:12 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/18 15:15:07 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sstream>

#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(void) {
    passwd_ = "";
    users_limit_ = 0;
    mode_.insert('t');
}

Channel::Channel(const Channel &ref) { *this = ref; }

Channel::Channel(const std::string &channel_name) {
    this->channel_name_ = channel_name;
    passwd_ = "";
    users_limit_ = 0;
}

Channel::~Channel(void) {}

Channel &Channel::operator=(const Channel &ref) {
    if (this == &ref)
        return *this;
    this->channel_name_ = ref.channel_name_;
    this->clients_ = ref.clients_;
    this->mode_ = ref.mode_;
    this->owners_ = ref.owners_;
    this->topic_ = ref.topic_;
    return *this;
}

void Channel::AddClient(Client &client) {
    int client_socket = client.getClientSocket();
    std::map<int, Client *>::iterator it = clients_.find(client_socket);
    if (!clients_.size()) {
        owners_[client_socket] = &client;
    }
    if (it == clients_.end()) {
        client.AddJoinedChannel(this->channel_name_);
        clients_[client_socket] = &client;
    }
}

/**
 * if client is owner, clients.begin() is set as channel owner after deletion
 */
void Channel::RemoveClient(int client_socket) {
    std::map<int, Client *>::iterator it = clients_.find(client_socket);
    if (it != clients_.end()) {
        (*it).second->RemoveJoinedChannel(this->channel_name_);
        clients_.erase(client_socket);
        if (owners_.find(client_socket) != owners_.end())
            RemoveOwner(client_socket);
        if (invited_clients_.find(client_socket) != invited_clients_.end())
            RemoveInvitedList(client_socket);
        if (!owners_.size() && clients_.size())
            AddOwner((*clients_.begin()).second);
    }
}

void Channel::SendMessageToAllClients(const std::string &message) {
    std::map<int, Client *>::iterator it = clients_.begin();
    for (; it != clients_.end(); it++) {
        (*it).second->PushSendQueue(message);
    }
}

void Channel::SendMessageToOthers(int sender_socket,
                                  const std::string &message) {
    std::map<int, Client *>::iterator it = clients_.begin();
    for (; it != clients_.end(); it++) {
        if ((*it).first == sender_socket)
            continue;
        (*it).second->PushSendQueue(message);
    }
}

const std::string &Channel::getChannelName(void) const { return channel_name_; }

void Channel::AddOwner(Client *client) {
    int client_socket = client->getClientSocket();
    if (owners_.find(client_socket) == owners_.end()) {
        owners_[client_socket] = client;
    }
}
void Channel::RemoveOwner(int client_socket) {
    if (owners_.find(client_socket) != owners_.end()) {
        owners_.erase(client_socket);
    }
}

void Channel::AddInvitedList(Client *client) {
    int client_socket = client->getClientSocket();
    if (invited_clients_.find(client_socket) == invited_clients_.end()) {
        invited_clients_[client_socket] = client;
    }
}
void Channel::RemoveInvitedList(int client_socket) {
    if (invited_clients_.find(client_socket) != invited_clients_.end()) {
        invited_clients_.erase(client_socket);
    }
}

void Channel::setChannelName(const std::string &channel_name) {
    this->channel_name_ = channel_name;
}
// return : true = that client joined this channel
// return : false = that client didn't join this channe
bool Channel::HasClient(int client_socket) const {
    std::map<int, Client *>::const_iterator it = clients_.find(client_socket);
    return it != clients_.end();
}

bool Channel::HasClient(const std::string &nickname) const {
    std::map<int, Client *>::const_iterator it = clients_.begin();
    while (it != clients_.end()) {
        if (it->second->getNickname() == nickname)
            return true;
        it++;
    }
    return false;
}

bool Channel::HasMode(char mode) const {
    return mode_.find(mode) != mode_.end();
}

bool Channel::HasPassword(void) const { return passwd_ != ""; }

void Channel::AddMode(char mode) {
    if (kChannelModes.find(mode) != std::string::npos)
        mode_.insert(mode);
}
void Channel::RemoveMode(char mode) {
    if (mode_.find(mode) != mode_.end())
        mode_.erase(mode);
}

const std::string Channel::getModes(void) const {
    std::string ret = "";
    for (std::set<char>::iterator it = mode_.begin(); it != mode_.end(); it++)
        ret += (*it);
    return ret;
}

int Channel::getClientCount(void) const { return clients_.size(); }

/**
 * @param topic 설정할 토픽
 * @param who_did 누가 토픽을 설정했는지 <realname>@<hostname> 양식으로 넣을 것!
 */
void Channel::setTopic(const std::string &topic, const std::string &who_did) {
    topic_ = topic;
    topic_who_did_ = who_did;
    std::stringstream ss;
    ss << std::time(0);
    topic_set_time_ = ss.str();
}

const std::string &Channel::getTopic(void) const { return topic_; }
const std::string &Channel::getTopicSetTime(void) const {
    return topic_set_time_;
}
const std::string &Channel::getTopicWhoDid(void) const {
    return topic_who_did_;
}

Client *Channel::getJoinedClient(const std::string &nickname) {
    std::map<int, Client *>::iterator it = clients_.begin();
    while (it != clients_.end()) {
        if (it->second->getNickname() == nickname)
            return it->second;
        it++;
    }
    return NULL;
}

void Channel::setUsersLimit(size_t limit) { this->users_limit_ = limit; };

void Channel::setPassword(const std::string &passwd) { this->passwd_ = passwd; }

bool Channel::CheckPassword(const std::string &passwd) const {
    return this->passwd_ == passwd;
}

size_t Channel::getUsersLimit() const { return users_limit_; };
bool Channel::IsInvited(int client_socket) const {
    return invited_clients_.find(client_socket) != invited_clients_.end();
}

/**
 * @param client_socket 확인할 클라이언트
 *  @return 해당 클라이언트가 이 채널에서 권한이 있는지 확인
 */
bool Channel::IsOwner(int client_socket) const {
    return owners_.find(client_socket) != owners_.end();
}

const std::string Channel::ClientsList(void) const {
    std::string list("");
    for (std::map<int, Client *>::const_iterator it = clients_.begin();
         it != clients_.end(); it++) {
        if (IsOwner(it->first))
            list += "@";
        list += it->second->getNickname();
        list += " ";
    }
    return list;
}
