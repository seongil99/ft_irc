/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:16 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/09 14:58:02 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <string>

#include <iostream>

#include "Client.hpp"

class Channel {
  private:
    std::string channel_name_;
    std::map<int, Client *> clients_;

  public:
    Channel(void);
    Channel(const Channel &ref);
    Channel(const std::string &channel_name);
    ~Channel(void);

    Channel &operator=(const Channel &ref);

    void AddClient(Client &client);
    void RemoveClient(const Client &client);
    void RemoveClient(int client_socket);

    void SendMessageToAllClients(const std::string &message);

    const std::string &getChannelName(void) const;

    void setChannelName(const std::string &channel_name);
};

#endif
