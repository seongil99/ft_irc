/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:12 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/13 16:57:30 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

#include "Channel.hpp"

Channel::Channel(void) { 
	owner_ = NULL;
	passwd_ = "";
	users_limit_ = 0;
}

Channel::Channel(const Channel &ref) { *this = ref; }

Channel::Channel(const std::string &channel_name) {
    this->channel_name_ = channel_name;
    owner_ = NULL;
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
    this->owner_ = ref.owner_;
    this->topic_ = ref.topic_;
    return *this;
}

void Channel::AddClient(Client &client) {
    int client_socket = client.getClientSocket();
    std::map<int, Client *>::iterator it = clients_.find(client_socket);
    if (!clients_.size()) {
		owners_[client_socket] = &client;
        owner_ = &client;
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
        if (owner_ && owner_->getClientSocket() == client_socket)
            owner_ = NULL;
        if (owner_ == NULL && clients_.size())
            setOwner((*clients_.begin()).second);
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
Client *Channel::getOwner(void) const { return owner_; }

void Channel::setChannelName(const std::string &channel_name) {
    this->channel_name_ = channel_name;
}
void Channel::setOwner(Client *client) { this->owner_ = client; }

// return : true = that client joined this channel
// return : false = that client didn't join this channe
bool Channel::HasClient(int client_socket) {
    std::map<int, Client *>::iterator it = clients_.find(client_socket);
    return it != clients_.end();
}

bool Channel::HasClient(const std::string &nickname) {
    std::map<int, Client *>::iterator it = clients_.begin();
    while (it != clients_.end()) {
        if (it->second->getNickname() == nickname)
            return true;
        it++;
    }
    return false;
}

bool Channel::HasMode(char mode) { return mode_.find(mode) != mode_.end(); }

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

void Channel::setTopic(const std::string &topic) { topic_ = topic; }

const std::string &Channel::getTopic(void) const { return topic_; }

Client *Channel::getJoinedClient(const std::string &nickname) {
    std::map<int, Client *>::iterator it = clients_.begin();
    while (it != clients_.end()) {
        if (it->second->getNickname() == nickname)
            return it->second;
        it++;
    }
    return NULL;
}

std::string Channel::getPassword() { return passwd_; };
int	Channel::getUsersLimit() { return users_limit_; };
bool Channel::IsInvited(int client_socket) {
	std::map<int, Client *>::iterator it = invited_clients_.find(client_socket);
    return it != clients_.end();
}

bool Channel::IsOwner(int client_socket) { return owners_.find(client_socket) != owners_.end(); }

const std::string Channel::ClientsList(void) {
	std::string list("");
	for (std::map<int, Client *>::iterator it = clients_.begin(); it!= clients_.end(); it++) {
		if (IsOwner(it->first))
			list += "@";
		list += it->second->getNickname();
		list += " ";
	}
	std::cout << list << std::endl;
	return list;
}
