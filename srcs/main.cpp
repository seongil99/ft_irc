/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 17:59:23 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/10 18:01:13 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

#include "Server.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <passwd>" << std::endl;
        return 1;
    }
    Server server;
    std::string passwd = (argc == 3) ? std::string(argv[2]) : "";
    server.Init(std::atoi(argv[1]), passwd);
    server.Listen();
    return 0;
}
