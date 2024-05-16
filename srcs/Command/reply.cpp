#include "reply.hpp"
#include <sstream>

//if Reply number is 1 then return would be ":irc.local 001 "
//if Reply number is 10 then return would be ":irc.local 010 "
//if Reply number is 100 then return would be ":irc.local 100 "
std::string get_reply_number(const Reply n)
{
	std::string ret(":irc.local ");
	std::stringstream	iss;
	iss << n;
	std::string temp = iss.str();
	if (temp.size() == 1)
		ret += "00";
	else if (temp.size() == 2)
		ret += "0";
	return ret + temp + " ";
}

//004 RPL_MYINFO : servername, version, available user mode, available channel mode
std::string	get_reply_str(const Reply n, std::string s1, std::string s2, std::string s3, std::string s4)
{
	std::string	ret;
	switch (n)
	{
	case RPL_MYINFO://s1 is servername, s2 is version, s3 is available user mode, s4 is available channel mode
		ret = s1 + " " + s2 + " " + s3 + " " + s4;
		break;
	
	default:
		ret = "";
		break;
	}
	return ret + "\r\n";
}
//001 RPL_WELCOME : nick, user, host
//324 RPL_CHANNELMODEIS : channel, mode, mode parameter
std::string	get_reply_str(const Reply n, std::string s1, std::string s2, std::string s3)
{
	std::string	ret;
	switch (n)
	{
	case RPL_WELCOME://
		ret = " :Welcome to our ft_irc project s.t. Internet Relay Network " + s1 + "!" + s2 + "@" + s3;
		break;
	case RPL_CHANNELMODEIS://s1 is channel s2 is mode s3 is mode parameter
		ret = s1 + " " + s2 + " " + s3;
		break;
	default:
		ret = "";
		break;
	}
	return ret + "\r\n";
}

//002 RPL_YOURHOST : server name, version
//301 RPL_AWAY : nickname, away message
//332 RPL_TOPIC : channel, topic
//341 RPL_INVITING : channel, nick
//401 ERR_NOSUCHNICK: nickname, target
//403 ERR_NOSUCHCHANNEL: nick, channel
// 442 ERR_NOTONCHANNEL: channel name
//443 ERR_USERONCHANNEL : user, channel
//482 ERR_CHANOPRIVSNEEDED: nick, channel
std::string	get_reply_str(const Reply n, std::string s1, std::string s2)
{
	std::string	ret;
	switch (n)
	{
	case RPL_YOURHOST://s1 is servername, s2 is version
		ret = " :Your host is " + s1 + ", running version " + s2;
		break;
	case RPL_AWAY://si is nickname s2 is away message
		ret = s1 + " :" + s2;
		break;
	case RPL_TOPIC://s1 is channel, s2 is topic
		ret = s1 + " :" + s2;
		break;
	case RPL_INVITING://s1 is channel s2 is nick
		ret = s1 + " " + s2;
		break;
	case ERR_USERONCHANNEL://s1 is user, s2 is channel
		ret = s1 + " " + s2 + " :is already on channel";
		break;
	case ERR_CHANOPRIVSNEEDED:
		ret = s1 + " " + s2 + " :You must be a channel op or higher to change the topic.";
		break;
	case ERR_NOSUCHNICK://s1 is nickname
		ret = s1 + " " + s2 + " :No such nick/channel";
		break;
	case ERR_NOSUCHCHANNEL://s1 is nickname s2 is channel name
		ret = s1 + " " + s2 + " :No such channel";
		break;
	case ERR_NOTONCHANNEL://s1 is channel name
		ret = s1 + " :You're not on that channel";
		break;
	default:
		ret = "";
		break;
	}
	return ret + "\r\n";
}

/*
003 RPL_CREATED:  data
331 RPL_NOTOPIC: channel name
404 ERR_CANNOTSENDTOCHAN: channel name
405 ERR_TOOMANYCHANNELS: channel name
407 ERR_TOOMANYTARGETS: target....??
411 ERR_NORECIPIENT: command
413 ERR_NOTOPLEVEL: mask
414 ERR_WILDTOPLEVEL: mask
432 ERR_ERRONEUSNICKNAME: nickname
433 ERR_NICKNAMEINUSE: nickname
461 ERR_NEEDMOREPARAMS: command
472 ERR_UNKNOWNMODE: char
476 ERR_BADCHANMASK: channel
*/
std::string	get_reply_str(const Reply n, std::string s1)
{
	std::string	ret;
	switch (n)
	{
	case RPL_CREATED://s1 is data
		ret = " :This server was created " + s1;
		break;
	case RPL_NOTOPIC://s1 is channel name
		ret = s1 + " :No topic is set";
		break;
	case ERR_CANNOTSENDTOCHAN://s1 is channel name
		ret = s1 + " :You cannot send to channel";
		break;
	case ERR_TOOMANYCHANNELS://s1 is channel name
		ret = s1 + " :You have joined too many channels";
		break;
	case ERR_TOOMANYTARGETS://s1 is target....??
		ret = s1 + " :Duplicate recipients. No message delivered";
		break;
	case ERR_NORECIPIENT://s1 is command
		ret = ":No recipient given " + s1;
		break;
	case ERR_NOTOPLEVEL://s1 is mask
		ret = s1 + " :No toplevel domain specified";
		break;
	case ERR_WILDTOPLEVEL://s1 is mask
		ret = s1 + " :Wildcard in toplevel domain";
		break;
	case ERR_ERRONEUSNICKNAME://s1 is nickname
		ret = s1 + " :Erroneus nickname";
		break;
	case ERR_NICKNAMEINUSE://s1 is nickname
		ret = s1 + " :Nickname is already in use";
		break;
	case ERR_NEEDMOREPARAMS://s1 is command
		ret = s1 + " :Not enough parameters";
		break;
	case ERR_UNKNOWNMODE://s1 is char
		ret = s1 + " :is unknown mode char to me";
		break;
	case ERR_BADCHANMASK://s1 is channel
		ret = s1 + " :Bad Channel Mask";
		break;
	default:
		break;
	}
	return ret + "\r\n";
}

//381 RPL_YOUREOPER
//412 ERR_NOTEXTTOSEND
//431 ERR_NONICKNAMEGIVEN
//462 ERR_ALREADYREGISTRED
//464 ERR_PASSWDMISMATCH
//491 ERR_NOOPERHOST
std::string	get_reply_str(const Reply n)
{
	std::string ret;
	switch (n)
	{
	case RPL_YOUREOPER:
		ret = ":You are now an IRC operator";
		break;
	case ERR_NOTEXTTOSEND:
		ret = ":No text to send";
		break;
	case ERR_NONICKNAMEGIVEN:
		ret = ":No nickname given";
	case ERR_ALREADYREGISTRED:
		ret = ":You may not reregister";
		break;
	case ERR_PASSWDMISMATCH:
		ret = ":Password incorrect";
		break;
	case ERR_NOOPERHOST:
		ret = ":No O-lines for your host";
		break;
	default:
		ret = "";
		break;
	}
	return ret + "\r\n";
}
