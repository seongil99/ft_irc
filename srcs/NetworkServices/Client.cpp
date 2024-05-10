/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:20:37 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/10 15:39:34 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

#include <iostream>

Client::Client(void) {}

Client::Client(const Client &ref) { *this = ref; }

Client::Client(int client_socket) : client_socket_(client_socket) {}

Client::~Client(void) {}

Client &Client::operator=(const Client &ref) {
    if (this == &ref)
        return *this;
    this->client_socket_ = ref.client_socket_;
    this->message_ = ref.message_;
    this->nickname_ = ref.nickname_;
    this->realname_ = ref.realname_;
    this->username_ = ref.username_;
    return *this;
}

void Client::AddJoinedChannel(const std::string &channel_name) {
    this->joind_chanels_.push_back(channel_name);
}
void Client::RemoveJoinedChannel(const std::string &channel_name) {
    std::vector<std::string>::iterator it;
    it = std::find(joind_chanels_.begin(), joind_chanels_.end(), channel_name);
    if (it != joind_chanels_.end())
        joind_chanels_.erase(it);
}

void Client::PushSendQueue(const std::string &message) {
    send_q_.push(message);
}

std::string Client::PopSendQueue(void) {
    if (!send_q_.size())
        return "";
    std::string ret = send_q_.front();
    send_q_.pop();
    return ret;
}

int Client::getClientSocket(void) const { return client_socket_; }
const std::string &Client::getUsername(void) const { return username_; }
const std::string &Client::getRealname(void) const { return realname_; }
const std::string &Client::getNickname(void) const { return nickname_; }
const std::string &Client::getMessage(void) const { return message_; }
size_t Client::getSendQueueSize(void) const { return send_q_.size(); }

void Client::setNickname(const std::string &str) { this->nickname_ = str; }
void Client::setRealname(const std::string &str) { this->realname_ = str; }
void Client::setUsername(const std::string &str) { this->username_ = str; }
void Client::setMessage(const std::string &str) { this->message_ = str; }
