/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 18:03:26 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/17 16:32:17 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "Server.hpp"
#include "utils.hpp"

Server::Server(void) : cmd(this), started_time_(irc_utils::getTimeOfNow()) {
    server_socket_ = 0;
    kq_ = 0;
    std::memset(&server_addr_, 0, sizeof(server_addr_));
}

Server::~Server(void) {}

/**
 * Init Server
 * @param port port number
 */
void Server::Init(int port, std::string passwd) {
    passwd_ = passwd;
    server_socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_ == -1)
        irc_utils::ExitWithError("socket() error");
    server_addr_.sin_family = AF_INET;                // IPV4 protocol
    server_addr_.sin_addr.s_addr = htonl(INADDR_ANY); // 32bit IPV4 addr
    server_addr_.sin_port = htons(port);              // port number
    int bind_result = bind(server_socket_, (struct sockaddr *)(&server_addr_),
                           sizeof(server_addr_));
    if (bind_result == -1)
        irc_utils::ExitWithError("bind() error");
    kq_ = kqueue();
    if (kq_ == -1)
        irc_utils::ExitWithError("kqueue() error");
    int fcntl_result = fcntl(server_socket_, F_SETFL, O_NONBLOCK);
    if (fcntl_result == -1)
        irc_utils::ExitWithError("fcntl() error");
    std::cout << "server port " << port << std::endl;
}

/**
 * Listen tcp connection from client
 * call Init() before Listen()
 */
void Server::Listen(void) {
    if (server_socket_ == 0)
        return;
    int listen_result = listen(server_socket_, BACKLOG_SIZE);
    if (listen_result == -1)
        irc_utils::ExitWithError("listen() error");
    ChangeEvents(server_socket_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    std::cout << "irc server started" << std::endl;

    /* main loop */
    int new_events;
    struct kevent *curr_event;
    while (true) {
        /*  apply changes and return new events(pending events) */
        new_events = kevent(kq_, &change_list_[0], change_list_.size(),
                            event_list_, 8, NULL);
        if (new_events == -1)
            irc_utils::ExitWithError("kevent() error\n");

        change_list_.clear(); // clear change_list_ for new changes

        for (int i = 0; i < new_events; ++i) {
            curr_event = &event_list_[i];

            /* check error event return */
            if (curr_event->flags & EV_ERROR) {
                EventError(curr_event);
            } else if (curr_event->filter == EVFILT_READ) {
                EventRead(curr_event);
            } else if (curr_event->filter == EVFILT_WRITE) {
                EventWrite(curr_event);
            }
        }
    }
}

void Server::EventError(struct kevent *curr_event) {
    if (curr_event->ident == (unsigned long)server_socket_)
        irc_utils::ExitWithError("server socket error");
    else {
        std::cerr << "client socket error" << std::endl;
        RemoveClientFromServer(curr_event->ident);
    }
}

void Server::EventRead(struct kevent *curr_event) {
    if (curr_event->ident == (unsigned long)server_socket_) {
        /* accept new client */
        int client_socket;
        if ((client_socket = accept(server_socket_, NULL, NULL)) == -1)
            irc_utils::ExitWithError("accept() error\n");
        std::cout << "accept new client: " << client_socket << std::endl;
        int fcntl_result = fcntl(client_socket, F_SETFL, O_NONBLOCK);
        if (fcntl_result == -1)
            irc_utils::ExitWithError("fcntl() error");

        /* add event for client socket - add read && write event */
        ChangeEvents(client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
                     NULL);
        ChangeEvents(client_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0,
                     NULL);
        clients_[client_socket] = Client(client_socket);
    } else if (clients_.find(curr_event->ident) != clients_.end()) {
        /* read data from client */
        char buf[BUF_SIZE];
        int n = read(curr_event->ident, buf, sizeof(buf));

        if (n <= 0) {
            if (errno == EWOULDBLOCK)
                return;
            else if (n < 0)
                std::cerr << "client read error!" << std::endl;
            RemoveClientFromServer(curr_event->ident);
        } else {
            buf[n] = '\0';
            ProcessReceivedData(curr_event->ident, buf, n);
        }
    }
}

void Server::EventWrite(struct kevent *curr_event) {
    /* send data to client */
    clients_iter it = clients_.find(curr_event->ident);
    if (it != clients_.end() && (*it).second.getSendQueueSize()) {
        std::string str_to_client = (*it).second.PopSendQueue();
        int n = write(curr_event->ident, str_to_client.c_str(),
                      str_to_client.size());
        if (n == -1) {
            std::cerr << "client write error!" << std::endl;
            RemoveClientFromServer(curr_event->ident);
        }
    }
}

void Server::ProcessReceivedData(int client_socket, char buf[BUF_SIZE], int n) {
    (void)n;
    // 메시지 받고
    std::string temp(buf);
    clients_iter it = clients_.find(client_socket);
    (*it).second.AppendMessage(temp);

    if (temp.rfind("\r\n") == std::string::npos) {
        return;
    }

    // 서버 콘솔에 출력==================================================
    std::cout << "received data from " << client_socket << ": ";
    std::string str = (*it).second.getMessage();
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '\r')
            std::cout << "\\r";
        else if (str[i] == '\n')
            std::cout << "\\n";
        else
            std::cout << str[i];
    }
    std::cout << '\n' << std::endl;
    //=================================================================

    if (cmd.excute(&((*it).second), (*it).second.getMessage()) == false) {
        std::cerr << "Something horrible received!" << std::endl;
    }

    // execute 이후 client 지워질 수 있음
    it = clients_.find(client_socket);
    if (it != clients_.end())
        (*it).second.setMessage(""); // 버퍼 초기화
}

/**
 * change properties of the event
 * @param ident client socket descriptor
 * @param filter 어떤 이벤트를 설정할 것인지 READ, WRITE 등
 * @param flags 필터를 설정하는 플래그 ADD, ENABLE, ONSHOT 등
 */
void Server::ChangeEvents(uintptr_t ident, int16_t filter, uint16_t flags,
                          uint32_t fflags, intptr_t data, void *udata) {
    struct kevent temp_event;
    EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
    change_list_.push_back(temp_event);
}

/**
 * Close connection of the client
 * @param client_fd client socket descriptor
 */
void Server::CloseClient(int client_fd) {
    std::cout << "client disconnected: " << client_fd << std::endl;
    close(client_fd);
    clients_.erase(client_fd);
}

bool Server::HasPassword(void) const { return passwd_ != ""; }

void Server::CreateChannel(const std::string &channel_name) {
    if (!HasChannel(channel_name)) {
        Channel new_channel(channel_name);
        channels_[channel_name] = new_channel;
    }
}

void Server::AddClientToChannel(Client &client,
                                const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        (*it).second.AddClient(client);
}

void Server::RemoveClientFromChannel(int client_socket,
                                     const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        (*it).second.RemoveClient(client_socket);
        if ((*it).second.getClientCount() == 0)
            channels_.erase(it);
    }
}

void Server::RemoveClientFromChannel(
    int client_socket,
    std::map<std::string, Channel>::reverse_iterator channel_iter) {
    (*channel_iter).second.RemoveClient(client_socket);
    if ((*channel_iter).second.getClientCount() == 0)
        channels_.erase((*channel_iter).first);
}

void Server::AddChannelOwner(Client &client, const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        (*it).second.AddOwner(&client);
    }
}

void Server::AddChannelOwner(const std::string &client_nickname,
                             const std::string &channel_name) {
    clients_iter client = FindClientByNickname(client_nickname);
    channels_iter it = channels_.find(channel_name);
    if (client != clients_.end() && it != channels_.end()) {
        (*it).second.AddOwner(&(*client).second);
    }
}

void Server::RemoveChannelOwner(Client &client,
                                const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        (*it).second.RemoveOwner(client.getClientSocket());
    }
}

void Server::RemoveChannelOwner(const std::string &client_nickname,
                                const std::string &channel_name) {
    clients_iter client = FindClientByNickname(client_nickname);
    channels_iter it = channels_.find(channel_name);
    if (client != clients_.end() && it != channels_.end()) {
        (*it).second.RemoveOwner((*client).first);
    }
}

void Server::PushSendQueueClient(int client_socket,
                                 const std::string &message) {
    clients_iter it = clients_.find(client_socket);
    if (it != clients_.end()) {
        (*it).second.PushSendQueue(message);
    }
}

/**
 * 서버에 password가 설정 되어있지 않은 경우 "" 와 비교
 */
bool Server::CheckPassword(const std::string &password_input) const {
    return this->passwd_ == password_input;
}

bool Server::HasDuplicateNickname(const std::string &nickname) const {
    const_clients_iter it = clients_.begin();
    for (; it != clients_.end(); it++) {
        if ((*it).second.getNickname() == nickname)
            return true;
    }
    return false;
}

bool Server::HasChannel(const std::string &channel_name) const {
    const_channels_iter it = channels_.find(channel_name);
    if (it == channels_.end())
        return false;
    return true;
}

bool Server::HasClientInChannel(int client_socket,
                                const std::string &channel_name) const {
    const_channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        return (*it).second.HasClient(client_socket);
    }
    return false;
}

bool Server::HasClientInChannel(const std::string &client_nickname,
                                const std::string &channel_name) {
    clients_iter client = FindClientByNickname(client_nickname);
    const_channels_iter it = channels_.find(channel_name);
    if (client != clients_.end() && it != channels_.end()) {
        return (*it).second.HasClient((*client).first);
    }
    return false;
}

void Server::SendMessageToAllClientsInChannel(const std::string &channel_name,
                                              const std::string &message) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        (*it).second.SendMessageToAllClients(message);
    }
}

void Server::SendMessageToOthersInChannel(int client_socket,
                                          const std::string &channel_name,
                                          const std::string &message) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        (*it).second.SendMessageToOthers(client_socket, message);
    }
}

void Server::SendMessageToOtherClient(int sender_socket,
                                      const std::string &receiver_nickname,
                                      const std::string &message) {
    clients_iter it = FindClientByNickname(receiver_nickname);
    if (sender_socket == (*it).first)
        return;
    if (it != clients_.end()) {
        (*it).second.PushSendQueue(message);
    }
    if (sender_socket)
        return;
}

// 채널 password, invite only 관련 함수 추가
bool Server::HasChannelPassword(const std::string &channel_name) const {
    const_channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        if (!(*it).second.HasPassword())
            return true;
    }
    return false;
}

bool Server::CheckChannelPassword(const std::string &password_input,
                                  const std::string &channel_name) const {
    const_channels_iter it = channels_.find(channel_name);
    if (it != channels_.end() && HasChannelPassword(channel_name))
        return (*it).second.CheckPassword(password_input);
    return true;
}

clients_iter Server::FindClientByNickname(const std::string &nickname) {
    clients_iter it = clients_.begin();
    for (; it != clients_.end(); it++) {
        if ((*it).second.getNickname() == nickname)
            return it;
    }
    return clients_.end();
}

bool Server::IsInvitedChannel(int client_socket,
                              const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end() && (*it).second.HasMode('i'))
        return (*it).second.IsInvited(client_socket);
    return true;
}

bool Server::IsOverUsersLimitChannel(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end() && (*it).second.HasMode('l'))
        return HowManyClientsAreInChannel((*it).second.getChannelName()) >=
               IsOverUsersLimitChannel((*it).second.getChannelName());
    return false;
}

bool Server::HasModeInChannel(const char mode,
                              const std::string &channel_name) const {
    const_channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        return (*it).second.HasMode(mode);
    return false;
}

void Server::SetModeToChannel(const char mode,
                              const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        (*it).second.AddMode(mode);
}

void Server::RemoveModeFromChannel(const char mode,
                                   const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        (*it).second.RemoveMode(mode);
}

void Server::SetPasswordInChannel(const std::string &passwd,
                                  const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        (*it).second.setPassword(passwd);
}

void Server::SetUsersLimitInChannel(size_t limit,
                                    const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        (*it).second.setUsersLimit(limit);
}

bool Server::IsChannelOwner(int client_socket,
                            const std::string &channel_name) const {
    const_channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        return (*it).second.IsOwner(client_socket);
    return false;
}

bool Server::HasTopicInChannel(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end() && (*it).second.getTopic().size() != 0)
        return true;
    return false;
}

std::string Server::GetTopicInChannel(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end() && HasTopicInChannel(channel_name))
        return (*it).second.getTopic();
    return 0;
}

std::string Server::WhoDidTopicInChannel(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end() && HasTopicInChannel(channel_name))
        return (*it).second.getTopicWhoDid();
    return 0;
}

std::string Server::WhatTimeChannelMade(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        return (*it).second.getTopicSetTime();
    return 0;
}

/**
 * @return All channel name delimited by comma ','
 */
const std::string Server::getAllChannelName() const {
    std::string ret("");
    const_channels_iter it = channels_.begin();
    if (it == channels_.end())
        return ret;
    while (true) {
        ret += it->second.getChannelName();
        it++;
        if (it == channels_.end())
            break;
        else
            ret += ",";
    }
    return ret;
}

// return 0 = there is no channel in this server
Channel *Server::getChannel(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it == channels_.end())
        return NULL;
    return &(it->second);
}

/**
 * 클라이언트가 서버에서 나갔을떄
 * 서버에서 제발로 나갔거나, 강퇴당했거나등등
 * 서버 및 모든 곳에서 지우니, 최대한 마지막에 호출 할 것!
 */
void Server::RemoveClientFromServer(int client_socket) {
    std::map<std::string, Channel>::reverse_iterator channel_iter =
        channels_.rbegin();
    // 들어가있는 모든 채널에서 없애야한다.
    while (channel_iter != channels_.rend()) {
        // 소켓 번호로 동일 인물로 판별. 지금 참가한 채널의 운영자임.
        // 혼자 있었으면 채널도 없어져야됨 -> 주석 처리
        // 새로운 관리자 -> 일단 Channel::clients_.begin()
        RemoveClientFromChannel(client_socket, channel_iter);
        channel_iter++;
    }
    // 서버에서도 지우자
    clients_.erase(client_socket);
    // disconnect tcp connection
    CloseClient(client_socket);
}

/* Getter*/

/** @return 서버 시작한 시간*/
const std::string &Server::getStartedTime() const { return started_time_; }

/** @return 채널의 개수 */
size_t Server::HowManyChannelsAre() const { return channels_.size(); }
size_t Server::HowManyChannelsJoined(int client_socket) {
    clients_iter it = clients_.find(client_socket);
    if (it != clients_.end())
        return (*it).second.getJoinedChannelsCount();
    return 0;
}

/** @return 클라이언트 개수 */
size_t Server::HowManyClientsAre() const { return clients_.size(); }
size_t
Server::HowManyClientsAreInChannel(const std::string &channel_name) const {
    const_channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        return (*it).second.getClientCount();
    return 0;
}

size_t Server::GetUsersLimitInChannel(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        return (*it).second.getUsersLimit();
    return 0;
}

const std::string
Server::ClientsInChannelList(const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end())
        return (*it).second.ClientsList();
    return "";
}

/*
        return -1 = 클라이언트가 없음
*/
int Server::getClientSocket(const std::string &nick_name) {
    clients_iter it = FindClientByNickname(nick_name);
    if (it == clients_.end())
        return -1; // 애초에 없었음.
    return it->first;
}

/**
 * @param chaanel_name 초대한 채널 이름
 * @param nick_name 초대받은 유저 닉네임
 * @note 이 함수 내에서 유효성 검사를 안하니 호출전에 유효성 검사를 할 것!!
 */
void Server::AddInviteClient(const std::string &channel_name,
                             const std::string &nick_name) {
    Channel *channel = &(channels_.find(channel_name)->second);

    channel->AddInvitedList(&(FindClientByNickname(nick_name)->second));
}

/**
 * @param client_socket 대상 클라이언트 소켓 넘버
 * @param message 보낼 메시지. 양식을 꼭 지켜야함.\r\n으로 끝나야됨
 * @note QUIT 명령어를 위해 만든 함수. 자신이 참가한 채널에 자신을 제외한
 * 모든 클라이언트에게 메시지를 보내야함.
 */
void Server::SendMessageToAllJoinedChannel(int client_socket,
                                           const std::string &message) {
    for (channels_iter it = channels_.begin(); it != channels_.end(); it++) {
        if (HasClientInChannel(client_socket, it->first))
            SendMessageToOthersInChannel(client_socket, it->first, message);
    }
}

/**
 * 양식 => :irc.local 322 <nick> <channel> <참가 인원수> :<모드> {topic}
 * @param client_socket list 명령어를 보낸 클라이언트 소켓
 * @note 단일 명령어를 내려서 현재 존재하는 모든 채널의 정보를 보내야 함.
 */
void Server::ActivateList(Client *client) {
    std::string nickname = client->getNickname();
    std::string joined_client_number;
    for (channels_iter it = channels_.begin(); it != channels_.end(); it++) {
        std::stringstream iss;
        iss << it->second.getClientCount();
        joined_client_number = iss.str();
        client->PushSendQueue(":irc.local 322 " + nickname + " " + it->first +
                              " " + joined_client_number + " :[+" +
                              it->second.getModes() + "] " +
                              it->second.getTopic() + "\r\n");
    }
}

/**
 * 양식 => :irc.local 322 <nick> <channel> <참가 인원수> :<모드> {topic}
 * @param client_socket list 명령어를 보낸 클라이언트 소켓
 * @param channel_name 정보를 보고 싶은 채널 이름
 * @note 해당 채널의 정보를 보내야 함.
 */
void Server::ActivateList(Client *client, const std::string &channel_name) {
    channels_iter it = channels_.find(channel_name);
    if (it != channels_.end()) {
        std::string nickname = client->getNickname();
        std::stringstream iss;
        iss << it->second.getClientCount();
        std::string joined_client_number = iss.str();
        client->PushSendQueue(":irc.local 322 " + nickname + " " + it->first +
                              " " + joined_client_number + " :[+" +
                              it->second.getModes() + "] " +
                              it->second.getTopic() + "\r\n");
    }
}

/**
 * @param channel_name 추가할 채널 이름
 * @param topic 추가할 토픽
 * @param who_did 누가 설정했는지 양식에 맞춰서 넣을 것
 * @note 유효성 검사를 진행하지 않으니 호출 전에 유효성 검사를 다 해볼것!!
 */
void Server::SetTopicInChannel(const std::string &channel_name,
                               const std::string &topic,
                               const std::string &who_did) {
    channels_iter it = channels_.find(channel_name);
    Channel *channel = &(it->second);
    channel->setTopic(topic, who_did);
}

/**
 * @param client_socket 틀린 명령어를 보낸 클라이언트 소켓 넘버
 */
void Server::CorrectPassword(Client *client) { client->setPassword(true); }
