/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:20:37 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/16 13:01:01 by seonyoon         ###   ########.fr       */
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
    this->joined_chanels_.push_back(channel_name);
}
void Client::RemoveJoinedChannel(const std::string &channel_name) {
    std::vector<std::string>::iterator it;
    it =
        std::find(joined_chanels_.begin(), joined_chanels_.end(), channel_name);
    if (it != joined_chanels_.end())
        joined_chanels_.erase(it);
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

void Client::AppendMessage(const std::string &message) { message_ += message; }

int Client::getClientSocket(void) const { return client_socket_; }
const std::string &Client::getUsername(void) const { return username_; }
const std::string &Client::getRealname(void) const { return realname_; }
const std::string &Client::getNickname(void) const { return nickname_; }
const std::string &Client::getHostname(void) const{ return hostname_; }
const std::string &Client::getMessage(void) const { return message_; }
size_t Client::getSendQueueSize(void) const { return send_q_.size(); }
size_t Client::getJoinedChannelsCount(void) const { return joined_chanels_.size(); };

void Client::setNickname(const std::string &str) { this->nickname_ = str; }
void Client::setRealname(const std::string &str) { this->realname_ = str; }
void Client::setUsername(const std::string &str) { this->username_ = str; }
void Client::setHostname(const std::string &str) { this->hostname_ = str; }
void Client::setMessage(const std::string &str) { this->message_ = str; }

/**
 * @return 가장 마지막으로 참여했던 채널 이름, 비어있는 경우 "" 반환
 */
const std::string &Client::getLastJoinedChannelName(void) const {
    if (joined_chanels_.size())
		return *(joined_chanels_.rbegin());
    static const std::string empty_string;
    return empty_string;
}
