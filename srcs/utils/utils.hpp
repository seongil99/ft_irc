/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/07 13:54:58 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/07 14:32:35 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <string>

/*
	콘솔에서 \r\n입력하는 방법
	ctrl+v+m = \r
	ctrl+v+j = \n
*/

class Client;

namespace irc_utils {

void ExitWithError(const char *msg);
std::vector<std::string> Split(std::string str, char delim);
std::string	getForm(Client *client, std::string origin);
std::string getTimeOfNow();
void show_string_r_and_n(const std::string &str);
std::string	ft_uppercase(std::string str);

};

#endif
