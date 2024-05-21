/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:20:37 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/21 15:07:44 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

#include <iostream>
#include <sstream>

static const std::string KDelim = "\r\n";

Client::Client(void) : password_(false) {}

Client::Client(const Client &ref) { *this = ref; }

Client::Client(int client_socket)
    : client_socket_(client_socket), password_(false) {}

Client::~Client(void) {}

Client &Client::operator=(const Client &ref) {
    if (this == &ref)
        return *this;
    this->client_socket_ = ref.client_socket_;
    this->message_ = ref.message_;
    this->nickname_ = ref.nickname_;
    this->realname_ = ref.realname_;
    this->username_ = ref.username_;
    this->password_ = ref.password_;
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

void Client::PushRecvQueue(const std::string &message) {
    recv_q_.push(message);
}

std::string Client::PopRecvQueue(void) {
    if (!recv_q_.size())
        return "";
    std::string ret = recv_q_.front();
    recv_q_.pop();
    return ret;
}

std::string Client::getLine(void) {
    size_t idx = message_.rfind(KDelim);
    if (idx == std::string::npos) {
        return ""; // \r\n이 없으면 빈 문자열 반환
    }
    // \r\n 까지 포함해서 리턴
    std::string ret = message_.substr(0, idx + KDelim.size());
    message_ = message_.substr(idx + KDelim.size());
    return ret;
}

/**
 * @param message 해당 클라이언트가 입력한 메시지
 * @note \\n이 있을 때 앞에 \\r이 없으면 넣어주고 있으면 그대로 넘어가는게 맞는
것인가? -> irc 클라이언트는 알아서 잘 넣어주고, nc명령어로 깜빡하고 잘못입력하면
사용자 책임이라고 생각.
 * @note 아래 주석은 기껏만든 코드 버리기 아까워서 주석처리....ㅠㅠ
* @note
        // std::string parse("");
        // for (size_t i = 0; i < message.size(); i++)
        // {
        // 	if (message[i] == '\n')
        // 	{
        // 		if (i == 0)//\n으로 시작하는 메시지면?
        // 		{
        // 			parse += "\r\n";
        // 			continue;
        // 		}
        // 		else if (message[i - 1] != '\r')
        // 			parse += '\r';
        // 	}
        // 	parse += message[i];
        // }
        // message_ += parse;
*/
void Client::AppendMessage(const std::string &message) { message_ += message; }

int Client::getClientSocket(void) const { return client_socket_; }
const std::string &Client::getUsername(void) const { return username_; }
const std::string &Client::getRealname(void) const { return realname_; }
const std::string &Client::getNickname(void) const { return nickname_; }
const std::string &Client::getHostname(void) const { return hostname_; }
const std::string &Client::getMessage(void) const { return message_; }
size_t Client::getSendQueueSize(void) const { return send_q_.size(); }
size_t Client::getRecvQueueSize(void) const { return recv_q_.size(); }
size_t Client::getJoinedChannelsCount(void) const {
    return joined_chanels_.size();
};
bool Client::getPassword() const { return password_; }

void Client::setNickname(const std::string &str) { this->nickname_ = str; }
void Client::setRealname(const std::string &str) { this->realname_ = str; }
void Client::setUsername(const std::string &str) { this->username_ = str; }
void Client::setHostname(const std::string &str) { this->hostname_ = str; }
void Client::setMessage(const std::string &str) { this->message_ = str; }
void Client::setPassword(bool passed) { this->password_ = passed; }

/**
 * @return 현재까지 저장된 명령어에 \\r\\n이 포함 안되었는가에 대한 진리값
 */
bool Client::IsCmdCompleted() {
    return message_.rfind("\r\n") == std::string::npos;
}
