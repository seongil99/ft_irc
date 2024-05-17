/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/07 14:17:31 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/07 14:32:42 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <ctime>

#include "Client.hpp"
#include "utils.hpp"

void irc_utils::ExitWithError(const char *msg) {
    std::cerr << "Error: " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

std::vector<std::string> irc_utils::Split(std::string str, char delim) {
	std::vector<std::string> res;
	std::string tmp("");

	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == delim) {
			if (!tmp.empty())
				res.push_back(tmp);
			tmp.clear();
		}
		else
			tmp += str[i];
	}
	if (!tmp.empty())
		res.push_back(tmp);
	return res;
}


/** 
 * @param client 클라이언트 포인터
 * @param origin 뒤에 붙일 문자열. 뒤에 \\r\\n을 붙여서 안보내니 뒤에 붙어서 대입해야 한다!!
 * @return irc전송 폼에 맞춰서 반환해줌
 * @note 닉네임:test, 실명:root, 호스트:127.0.0.1일때 
 * :test!root@127.0.0.1 (전송문)
 * 을 보내야한다. 이 함수는 해당 폼을 쉽게 만들기 위해 만들었음
*/
std::string	irc_utils::getForm(Client *client, std::string origin)
{
	std::string temp(":");
	temp += client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " " + origin;
	return temp;
}


/** @return 양식에 맞춘 현재 시간*/
std::string	irc_utils::getTimeOfNow()
{
	std::time_t now = std::time(0);
	std::tm *localTime = std::localtime(&now);
	char buffer[80];
	std::strftime(buffer, 80, "%H:%M:%S %b %d %Y", localTime);
	return std::string(buffer);
}
