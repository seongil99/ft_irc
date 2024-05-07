/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: seonyoon <seonyoon@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/06 17:59:23 by seonyoon          #+#    #+#             */
/*   Updated: 2024/05/07 14:59:08 by seonyoon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

#include "Server.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        // todo: add password input => ./ircserv <port> <passwd>
        std::cerr << "Usage: ./ircserv <port>" << std::endl;
        return 1;
    }
    Server server;
    server.Init(std::atoi(argv[1]));
    server.Listen();
    return 0;
}
