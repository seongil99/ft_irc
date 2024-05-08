/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/08 16:05:12 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/08 20:25:57 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel(void) {}

Channel::Channel(const Channel &ref) { (void)ref; }

Channel::~Channel(void) {}

Channel &Channel::operator=(const Channel &ref) {
    (void)ref;
    return *this;
}
