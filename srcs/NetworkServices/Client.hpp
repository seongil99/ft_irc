/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:20:29 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/13 18:31:31 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <queue>
#include <set>
#include <string>

class Client {
  private:
    int client_socket_;
    std::string username_;
    std::string realname_;
    std::string nickname_;
    std::string message_;
    std::queue<std::string> send_q_;
    std::queue<std::string> recv_q_;
    std::set<std::string> joined_chanels_;

  public:
    Client(void);
    Client(int client_socket);
    Client(const Client &ref);
    ~Client(void);

    Client &operator=(const Client &ref);

    void AddJoinedChannel(const std::string &channel_name);
    void RemoveJoinedChannel(const std::string &channel_name);

    void PushSendQueue(const std::string &message);
    std::string PopSendQueue(void);

    int getClientSocket(void) const;
    const std::string &getUsername(void) const;
    const std::string &getRealname(void) const;
    const std::string &getNickname(void) const;
    const std::string &getMessage(void) const;
    size_t getSendQueueSize(void) const;

    void setUsername(const std::string &str);
    void setRealname(const std::string &str);
    void setNickname(const std::string &str);
    void setMessage(const std::string &str);
};

#endif
