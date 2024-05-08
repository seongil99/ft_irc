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
