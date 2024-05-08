/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:20:29 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/08 19:06:56 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
  private:
    int client_socket_;
    std::string username_;
    std::string realname_;
    std::string nickname_;
    std::string message_;

  public:
    Client(void);
    Client(int client_socket);
    Client(const Client &ref);
    ~Client(void);

    Client &operator=(const Client &ref);

    int getClientSocket(void) const;
    const std::string &getUsername(void) const;
    const std::string &getRealname(void) const;
    const std::string &getNickname(void) const;
    const std::string &getMessage(void) const;

    void setUsername(const std::string &str);
    void setRealname(const std::string &str);
    void setNickname(const std::string &str);
    void setMessage(const std::string &str);
};

#endif
