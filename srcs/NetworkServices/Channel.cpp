/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:12 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/09 15:01:35 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

void Channel::RemoveClient(int client_socket) { clients_.erase(client_socket); }

void Channel::SendMessageToAllClients(const std::string &message) {
    std::map<int, Client *>::iterator it = clients_.begin();
    for (; it != clients_.end(); it++) {
        (*it).second->PushSendQueue(message);
    }
}

const std::string &Channel::getChannelName(void) const { return channel_name_; }

void Channel::setChannelName(const std::string &channel_name) {
    this->channel_name_ = channel_name;
}
