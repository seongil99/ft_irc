/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:16 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/08 20:24:11 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <string>

#include "Client.hpp"

class Channel {
  private:
    std::string channel_name_;
    std::map<int, Client> clients_;

  public:
    Channel(void);
    Channel(const Channel &ref);
    ~Channel(void);

    Channel &operator=(const Channel &ref);
};

#endif
