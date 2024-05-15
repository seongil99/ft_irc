#ifndef COMMAND_HPP
# define COMMAND_HPP

# include "reply.hpp"
# include <iostream>
# include <string>
# include <vector>
# include <sstream>

class Client;
class Server;
// class Protocol;
// class Client;

class Command
{
private :
	//[OCCF]===================================
	Command();
	Command(const Command &origin);
	Command	&operator=(const Command &origin);
	//[OCCF]===================================
	std::vector<std::string>	cmd_list;
	std::vector<std::string>	cmd;
	std::string	private_msg;
	typedef void (Command::*cmd_ft_arr)(Client*);
	cmd_ft_arr cmd_ft[17];
	void	clean_cmd();
	Server *serv;
	//cmd=======================================
	void	pass(Client *client);
	void	nick(Client *client);
	void	user(Client *client);
	void	join(Client *client);
	void	part(Client *client);
	void	privmsg(Client *client);
	void	oper(Client *client);
	void	list(Client *client);
	void	ping(Client *client);
	void	quit(Client *client);
	void	kick(Client *client);
	void	invite(Client *client);
	void	topic(Client *client);
	void	mode(Client *client);
	void	who(Client *client);
	void	cap(Client *client);
	//cmd=======================================

public :
	Command(Server *server);
	~Command();
	bool	excute(Client *client, std::string str);

	//cmd=======================================
	void	pong(Client *client);
	//cmd=======================================
};

#endif
