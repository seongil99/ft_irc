/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 17:59:23 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/17 19:13:00 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sstream>

#include "Server.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <passwd>" << std::endl;
        return 1;
    }
    std::istringstream iss(argv[1]);
    int port;
    iss >> port;
    if (iss.fail() || iss.get() != -1 || port < 0 || port > 65535) {
        std::cerr << "Invalid Port!" << std::endl;
        return 1;
    }
    Server server;
    std::string passwd = (argc == 3) ? std::string(argv[2]) : "";
    server.Init(port, passwd);
    try {
        server.Listen();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
