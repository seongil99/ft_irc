#ifndef COMMAND
# define COMMAND

# include "reply.hpp"
# include <iostream>
# include <string>
# include <vector>

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
	typedef void (Command::*cmd_ft_arr)(int);
	cmd_ft_arr cmd_ft[15];
	void	clean_cmd();
	Server *serv;
	//cmd=======================================
	void	pass(int client_socket);
	void	nick(int client_socket);
	void	user(int client_socket);
	void	join(int client_socket);
	void	part(int client_socket);
	void	privmsg(int client_socket);
	void	oper(int client_socket);
	void	list(int client_socket);
	void	ping(int client_socket);
	void	quit(int client_socket);
	void	kick(int client_socket);
	void	invite(int client_socket);
	void	topic(int client_socket);
	void	mode(int client_socket);
	void	notice(int client_socket);
	void	pong(int client_socket);
	//cmd=======================================

public :
	Command(Server *server);
	~Command();
	bool	excute(int client_socket, std::string str);

};

#endif