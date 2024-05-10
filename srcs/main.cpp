/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 17:59:23 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/10 15:06:08 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
// #include <sstream>

#include "Server.hpp"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./ircserv <port> <passwd>" << std::endl;
        return 1;
    }
	//포트번호 확인하는 작업이 필요한가?
	//1. 기타 잡것이 없이 숫자만 입력하였는가?
	//2. 포트 번호의 조건에 부합하는 숫자를 입력하였는가?
	// std::istringstream	iss(argv[1]);
	// int port;
	// iss >> port;
	// if (iss.fail() || iss.get() != -1 || port < 0)
	// {
		// std::cerr << "Invalid Port!" << std::endl;
	// 	return 1;
	// }
    Server server;
    server.Init(std::atoi(argv[1]), std::string(argv[2]));
    server.Listen();
    return 0;
}
