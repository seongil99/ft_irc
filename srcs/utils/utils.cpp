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
