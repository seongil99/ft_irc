/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:12 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/10 15:41:29 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

#include "Channel.hpp"

Channel::Channel(void) {}

Channel::Channel(const Channel &ref) { *this = ref; }

Channel::Channel(const std::string &channel_name) {
    this->channel_name_ = channel_name;
}

Channel::~Channel(void) {}

Channel &Channel::operator=(const Channel &ref) {
    if (this == &ref)
        return *this;
    this->channel_name_ = ref.channel_name_;
    this->clients_ = ref.clients_;
    return *this;
}

void Channel::AddClient(Client &client) {
    int client_socket = client.getClientSocket();
    std::map<int, Client *>::iterator it = clients_.find(client_socket);

    if (it == clients_.end()) {
        client.AddJoinedChannel(this->channel_name_);
        clients_[client_socket] = &client;
    }
}

void Channel::RemoveClient(const Client &client) {
    clients_.erase(client.getClientSocket());
}

void Channel::RemoveClient(int client_socket) {
    clients_[client_socket]->RemoveJoinedChannel(this->channel_name_);
    clients_.erase(client_socket);
}

void Channel::SendMessageToAllClients(const std::string &message) {
    std::map<int, Client *>::iterator it = clients_.begin();
    for (; it != clients_.end(); it++) {
        (*it).second->PushSendQueue(message);
    }
}

const std::string &Channel::getChannelName(void) const { return channel_name_; }
Client *Channel::getOwner(void) const { return owner_; }

void Channel::setChannelName(const std::string &channel_name) {
    this->channel_name_ = channel_name;
}
void Channel::setOwner(Client *client) { this->owner_ = client; }

//return : true = that client joined this channel
//return : false = that client didn't join this channe
bool	Channel::DidJoinClient(int client_socket)
{
	std::map<int, Client *>::iterator it = clients_.find(client_socket);
	return it != clients_.end();
}

bool	Channel::DidJoinClient(const std::string &nickname)
{
	std::map<int, Client *>::iterator it = clients_.begin();
	while (it != clients_.end())
	{
		if (it->second->getNickname() == nickname)
			return true;
		it++;
	}
	return false;
}

Client	*Channel::getJoinedClient(const std::string &nickname)
{
	if (DidJoinClient(nickname))
	{
		std::map<int, Client *>::iterator it = clients_.begin();
		while (it != clients_.end())
		{
			if (it->second->getNickname() == nickname)
				return it->second;
		}
	}
	return 0;
}