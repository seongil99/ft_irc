#ifndef REPLY
# define REPLY

# include <string>

enum Reply
{
	RPL_WELCOME = 1,
	RPL_YOURHOST,
	RPL_CREATED,
	RPL_MYINFO,
	RPL_AWAY = 301,
	RPL_CHANNELMODEIS = 324,
	RPL_NOTOPIC = 331,
	RPL_TOPIC,
	RPL_INVITING = 341,
	RPL_YOUREOPER = 381,
	ERR_NOSUCHNICK = 401,
	ERR_NOSUCHCHANNEL = 403,
	ERR_CANNOTSENDTOCHAN,
	ERR_TOOMANYCHANNELS,
	ERR_TOOMANYTARGETS = 407,
	ERR_NORECIPIENT = 411,
	ERR_NOTEXTTOSEND,
	ERR_NOTOPLEVEL,
	ERR_WILDTOPLEVEL,
	ERR_NONICKNAMEGIVEN = 431,
	ERR_ERRONEUSNICKNAME,
	ERR_NICKNAMEINUSE,
	ERR_NOTONCHANNEL = 442,
	ERR_USERONCHANNEL,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTRED,
	ERR_PASSWDMISMATCH = 464,
	ERR_KEYSET = 467,
	ERR_CHANNELISFULL = 471,
	ERR_UNKNOWNMODE,
	ERR_INVITEONLYCHAN,
	ERR_BADCHANNELKEY = 475,
	ERR_BADCHANMASK,
	ERR_CHANOPRIVSNEEDED = 482,
	ERR_NOOPERHOST = 491
};

std::string get_reply_number(const Reply n);
std::string	get_reply_str(const Reply n);
std::string	get_reply_str(const Reply n, std::string s1);
std::string	get_reply_str(const Reply n, std::string s1, std::string s2);
std::string	get_reply_str(const Reply n, std::string s1, std::string s2, std::string s3);
std::string	get_reply_str(const Reply n, std::string s1, std::string s2, std::string s3, std::string s4);

#endif