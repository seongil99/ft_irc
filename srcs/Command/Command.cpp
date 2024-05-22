#include <sstream>

#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <unistd.h>
#include <ctime>
#include <climits>

Command::Command(Server *server) : serv(server), rn("\r\n")
{
	cmd_ft["PING"] = &Command::ping;
	cmd_ft["PRIVMSG"] = &Command::privmsg;
	cmd_ft["PASS"] = &Command::pass;
	cmd_ft["NICK"] = &Command::nick;
	cmd_ft["USER"] = &Command::user;
	cmd_ft["JOIN"] = &Command::join;
	cmd_ft["LIST"] = &Command::list;
	cmd_ft["QUIT"] = &Command::quit;
	cmd_ft["KICK"] = &Command::kick;
	cmd_ft["INVITE"] = &Command::invite;
	cmd_ft["TOPIC"] = &Command::topic;
	cmd_ft["MODE"] = &Command::mode;
	cmd_ft["WHO"] = &Command::who;
	cmd_ft["PART"] = &Command::part;
	cmd_ft["CAP"] = &Command::cap;
}

Command::~Command() {}

/**
 * @param clinet 클라이언트 포인터
 * @param str 클라이언트가 입력한 글
 * @returns
 * return true : user typed cmd.
 * return false : user typed wrong cmd. str will be abandoned
 * @note QUIT 명령어일수도 있으니(해당 Client클래스가 없어져버리므로) 이 명령어 아래에 해당 클라이언트 포인터를 사용하면 안됨!
 */
bool Command::excute(Client *client, std::string str)
{
	// set default=============================
	bool ret = false;
	if (str.size() < 4)
		return ret;
	std::string temp("");
	private_msg = str;
	int	prevented_idx = 0;
	std::map<std::string, cmd_fts>::iterator it;
	//==========================================================
	// step 1 : split string by ' '
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] == ' ')
		{
			if (temp.empty())
				continue;
			else
			{
				cmd.push_back(temp);
				temp.clear();
			}
		}
		else if (str[i] == '\n')
		{//=====================================================
			if (cmd.empty() || i == 0)
				continue;
			else if (i != str.size() - 1 && i > 0)
			{
				if (str[i - 1] == '\r')
				{
					if (temp.empty() == false)
						cmd.push_back(temp);
					private_msg = str.substr(prevented_idx, i - prevented_idx + 1);
					it = cmd_ft.find(cmd[0]);
					if (it != cmd_ft.end())
					{
						ret = true;
						(this->*(it->second))(client);
					}
					else
					{
						if (client->getHostname().empty())
							continue;
						else if (cmd.size())
							client->PushSendQueue(":irc.local 421 " + client->getNickname() + " " + irc_utils::ft_uppercase(cmd[0]) + " :Unknown command" + rn);
					}
					// DebugFtForCmdParssing();//주석 처리할 거면 할 것!
					temp.clear();
					cmd.clear();
					prevented_idx = i + 1;
					private_msg = str.substr(prevented_idx);
				}
			}
		}//=====================================================
		else if (str[i] != '\r' && str[i] != '\n')
			temp += str[i];
	}
	if (temp.empty() == false)
		cmd.push_back(temp);
	//==========================================================
	// step 2 : figure it out is this cmd or not
	if (cmd.empty())
		return ret;
	it = cmd_ft.find(cmd[0]);
	if (it != cmd_ft.end())
	{
		ret = true;
		(this->*(it->second))(client);
	}
	else if (cmd.size() && client->getHostname().empty() == false)
		client->PushSendQueue(":irc.local 421 " + client->getNickname() + " " + irc_utils::ft_uppercase(cmd[0]) + " :Unknown command" + rn);
	//==========================================================
	// DebugFtForCmdParssing();//주석 처리할 거면 할 것!
	// client 삭제를 대비해서 밑에 그 어느것도 있으면 안됨!!
	cmd.clear();
	return ret;
}

/**
 * @note 이 명령어는 채널 입장용 비번 명령어가 아닌 서버 입장용 비번 명령어임.
 * @note PASS <password>
 * @note 비번에 관한 경우의 3가지
 * @note 1. 서버 비번 없음 => 비번 입력하든 말든 프리패스 해야함
 * @note 2. 서버 비번 있음
 * @note 2-1 비번 맞음 => 통과
 * @note 2-2 비번 틀림 => 통과 못함
*/
void Command::pass(Client *client)
{
	if (cmd.size() == 1)
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
	else if(client->getPassword() == false && serv->CheckPassword(cmd[1]))
		serv->CorrectPassword(client);
}

/**
 * @note 닉네임 변경하는 명령어
*/
void Command::nick(Client *client)
{ // NICK <nickname>
	std::string nickname = client->getNickname();

	if (cmd.size() == 1) {
		if (nickname.empty())  //닉네임 없이 입장했을 때
			client->PushSendQueue(get_reply_number(ERR_NONICKNAMEGIVEN) + get_reply_str(ERR_NONICKNAMEGIVEN));
		// else
		// 	client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
	}
	else
	{
		// 닉네임 중복 여부 판단
		if (serv->HasDuplicateNickname(cmd[1]))
		{
			if (client->getNickname().empty() && client->getHostname().empty())
			{ //최초 접속시 중복된 닉네임을 원함.
				client->PushSendQueue("ERROR :Closing link: (root@192.168.65.2) [Access denied by dup-Nickname issue]\nPlease access again with other nickname!" + rn);
				return;//연결 종료
			}
			client->PushSendQueue(get_reply_number(ERR_NICKNAMEINUSE) + nickname + " " + cmd[1] + " :Nickname is already in use." + rn);
		}
		else// 닉네임 중복이 안되었으니 닉네임 변경
		{
			if (!client->getRealname().empty())
				client->PushSendQueue(":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " NICK :" + cmd[1] + rn);
			client->setNickname(cmd[1]);
		}
	}
}

/**
 * @note 최초 입장시 호스트, 실명, 입장여부를 정하는 명령어
 * @note USER <username> <hostname> <servername> :<realname>
 * @note 연결이 시작될 때 사용자의 사용자명, 실명 지정에 사용
 * @note 실명 매개변수는 공백 문자를 포함할 수 있도록 마지막 매개변수여야 하며, :을 붙여 인식하도록 함
 * @note 5번째 인자가 :으로 시작해야함. 없하면 무시하기로
*/
void Command::user(Client *client)
{
	if (client->getNickname().empty())//이럴 경우는 접속할때 중복된 닉네임으로 접속시도한 경우이거나 막 입력한 것
		return;
	if (cmd.size() < 5)
	{ // 인자를 적게 넣었을 경우
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + client->getNickname() + " " + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return;
	}
	if (client->getRealname().size() != 0) // 사용자가 USER 명령어를 내렸을 경우 또는 최초 호출인데 뭔가 잡것이 있는 경우
	{
		client->PushSendQueue(get_reply_number(ERR_ALREADYREGISTRED) + client->getNickname() + " " + get_reply_str(ERR_ALREADYREGISTRED));
		return;
	}
	if (cmd[4][0] != ':') //realname 맨 앞에 ':'이 없음 -> 양식을 무시했으니 무시
		return;
	if (serv->CheckPassword(""))//서버가 비번이 설정안되어 있으면 패스
		serv->CorrectPassword(client);
	if (client->getPassword() == false)// 클라이언트가 비번 못 맞춘 경우
	{
		std::string temp;
		for (size_t i = 4; i < cmd.size(); i++)
		{
			temp += cmd[i];
			if (i != cmd.size() - 1)
				temp += " ";
		}
		client->PushSendQueue("ERROR :Closing link: (" + temp.substr(1, temp.size() - 1) + "@192.168.65.2) [Access denied by configuration]" + rn);
		return ;
	}
	std::string nickname = client->getNickname();
	// username 만들기
	std::string temp = cmd[1];
	if (temp[temp.size() - 1] == '\n')
		temp = temp.substr(0, temp.size());
	client->setUsername(temp);
	temp.clear();
	// realname 만들기
	for (size_t i = 4; i < cmd.size(); i++)
	{
		temp += cmd[i];
		if (i != cmd.size() - 1)
			temp += " ";
	}
	client->setRealname(temp.substr(1, temp.size() - 1));
	client->setHostname("192.168.65.2"); // 임시로 이렇게 넣은거니 확실해지면 추가할 것!
	std::string realname = client->getRealname();
	std::string hostname = client->getHostname();

	client->PushSendQueue(":irc.local NOTICE " + nickname + " :*** Could not resolve your hostname: Request timed out; using your IP address (" + hostname + ") instead." + rn);

	client->PushSendQueue(get_reply_number(RPL_WELCOME) + nickname + get_reply_str(RPL_WELCOME, nickname, realname, hostname));
	client->PushSendQueue(get_reply_number(RPL_YOURHOST) + nickname + get_reply_str(RPL_YOURHOST, "irc.local", "ft_irc"));
	client->PushSendQueue(get_reply_number(RPL_CREATED) + nickname + get_reply_str(RPL_CREATED, serv->getStartedTime()));
	client->PushSendQueue(get_reply_number(RPL_MYINFO) + get_reply_str(RPL_MYINFO, nickname, "irc.local", "OUR", "FT_IRC"));
	client->PushSendQueue(":irc.local 005 " + nickname + " Enjoy our ft_irc." + rn);
}

/**
 * @note 서버 입장 및 채널 입장하는 명령어
 * @note Join (ch1,ch2,...chn) (pw1,pw2,...,pwn)
*/
void Command::join(Client *client)
{
	if (cmd.size() < 2)
	{
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return ;
	}
	if (cmd.size() == 2 && cmd[1] == ":" && client->getHostname().empty())
	{
		client->PushSendQueue(":irc.local 451 * JOIN :You have not registered." + rn);
		return;
	}
	std::vector<std::string> channel, pw;
	std::string hostname = client->getHostname();
	if (hostname.empty() || client->getPassword() == false)
		return;
	channel = irc_utils::Split(cmd[1], ',');
	if (cmd.size() > 2)
		pw = irc_utils::Split(cmd[2], ',');
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	int socket = client->getClientSocket();

	for (size_t i = 0; i < channel.size(); i++)
	{ // 채널이 없으면 채널을 만든다
		if (!serv->HasChannel(channel[i]))
			serv->CreateChannel(channel[i]);
		else
		{ // 채널이 있는 상태 -> 권한, 비밀번호 확인
			if (!serv->IsInvitedChannel(socket, channel[i]))
			{
				client->PushSendQueue(":irc.local 473 " + nickname + " " + channel[i] + " :Cannot join channel (invite only)" + rn);
				continue;
			}
			else if (serv->HasChannelPassword(channel[i]))
			{
				if (i + 1 > pw.size() || pw[i].empty() || !serv->CheckChannelPassword(pw[i], channel[i]))
				{
					client->PushSendQueue(":irc.local 475 " + nickname + " " + channel[i] +
										  " :Cannot join channel (incorrect channel key)" + rn);
					continue;
				}
			}
			else if (serv->IsOverUsersLimitChannel(channel[i]))
			{
				client->PushSendQueue(":irc.local 471 " + nickname + " " + channel[i] +
									  " :Cannot join channel (channel is full)" + rn);
				continue;
			}
		}
		if (serv->HowManyChannelsJoined(socket) >= 10) // 채널 10개 이상 속했을 때
			client->PushSendQueue("irc.local 405 " + nickname + " " + get_reply_str(ERR_TOOMANYCHANNELS, channel[i]));
		else
		{
			serv->AddClientToChannel(*client, channel[i]);
			client->PushSendQueue(":" + nickname + "!" + realname + "@" + \
								  hostname + " JOIN :" + channel[i] + rn);
			if (serv->HasTopicInChannel(channel[i]))
			{
				client->PushSendQueue(":irc.local 332 " + nickname + " " + channel[i] + " :" + serv->GetTopicInChannel(channel[i]));
				client->PushSendQueue(":irc.local 333 " + nickname + " " + channel[i] + " " +
									  serv->WhoDidTopicInChannel(channel[i]) + " :" + serv->WhatTimeChannelMade(channel[i]) + rn);
			}
			client->PushSendQueue(":irc.local 353 " + nickname + " = " +
								  channel[i] + " :" + serv->ClientsInChannelList(channel[i]) + rn);
			client->PushSendQueue(":irc.local 366 " + nickname + " " + channel[i] + " :End of /NAMES list." + rn);
			serv->SendMessageToOthersInChannel(socket, channel[i], ":" + nickname + "!" + realname + "@" + hostname + " JOIN :" + channel[i] + rn);
			serv->RemoveInviteClient(channel[i], nickname);
		}
	}
}

/**
 * @note PART {#channel} {leave msg for last channel}
 * @note 입장했던 채널에서 나가는 명령어
 * @note 채널에서 마지막 한 명이 나갈때 채널 삭제 필요
 * @note 3번째 인자가 있다면 :으로 시작해야함. 없으면 무시하기로
*/
void Command::part(Client *client)
{
	if (cmd.size() < 2)
	{
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return ;
	}
	else if (cmd.size() == 3)
	{
		if (cmd[2][0] != ':')
			return;
	}
	if (client->getHostname().empty() || client->getPassword() == false)
		return;
	std::vector<std::string> channel = irc_utils::Split(cmd[1], ',');
	std::string nick_name = client->getNickname();
	int	socket = client->getClientSocket();
	std::string msg = irc_utils::getForm(client, cmd[0] + " :" + cmd[1] + rn);
	if (cmd.size() != 2)//나가는 메시지를 남김
		msg =irc_utils::getForm(client, private_msg); // private_msg의 마지막에 \r\n이 있어서 따로 추가 안해도 됨
	for (std::vector<std::string>::iterator it = channel.begin(); it != channel.end(); it++)
	{
		if (serv->HasChannel(*it) == false)
			client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nick_name, *it));
		else if (serv->HasClientInChannel(nick_name, *it) == false) //채널 있기는 한데 클라이언트가 미참여 -> 오류 메시지만 보내고 끝
			client->PushSendQueue(get_reply_number(ERR_NOTONCHANNEL) + get_reply_str(ERR_NOTONCHANNEL, nick_name, *it));
		else
		{
			serv->SendMessageToAllClientsInChannel(*it, msg);
			serv->RemoveClientFromChannel(socket, *it);
		}
	}
}

/**
 * @note 메시지 보내는 명령어
 * @note PRIVMSG (user1,user2,...,usern) <text to be sent>
 * @note 3번째 인자가 :으로 시작해야함. 없으면 무시하기로
*/
void Command::privmsg(Client *client)
{
	std::vector<std::string> target;
	if (cmd.size() < 3)
	{
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return ;
	}
	else if (cmd[2][0] != ':')
		return;
	std::string hostname = client->getHostname();
	if (hostname.empty() || client->getPassword() == false)
		return;
	target = irc_utils::Split(cmd[1], ',');
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	int socket = client->getClientSocket();
	size_t msg_start = cmd[0].size();
	while (private_msg[msg_start] == ' ')
		msg_start++;
	msg_start += cmd[1].size();
	while (private_msg[msg_start] == ' ')
		msg_start++;
	std::string msg = private_msg.substr(msg_start + 1);

	for (size_t i = 0; i < target.size(); i++) {
		if (target[i][0] == '#') {
			if (serv->HasChannel(target[i]) && serv->HasClientInChannel(socket, target[i]))
				serv->SendMessageToOthersInChannel(socket, target[i], \
					":" + nickname + "!" + realname + "@" + hostname + " PRIVMSG " + target[i] + " :" + msg);
			else if (serv->HasChannel(target[i]))
				client->PushSendQueue(":irc.local 404 " + client->getNickname() + " " + get_reply_str(ERR_CANNOTSENDTOCHAN, target[i]));
			else
				client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nickname, target[i]));
		}
		else {
			if (!serv->HasDuplicateNickname(target[i]))
				client->PushSendQueue(":irc.local 401 " + nickname + " " + target[i] + " :No such nick" + rn);
			else {
				serv->SendMessageToOtherClient(socket, target[i], \
				":" + nickname + "!" + realname + "@" + hostname + " PRIVMSG " + target[i] + " :" + msg);
			}
		}
	}
}

/**
 * @note 현재 있는 채널을 조회하는 명령어
 * @note LIST [<channel>{,<channel>}
*/
void Command::list(Client *client)
{
	std::string nickname = client->getNickname();
	if (client->getHostname().empty() || client->getPassword() == false)
		return;
	std::vector<std::string> target;
	client->PushSendQueue(":irc.local 321 " + nickname + " Channel :Users Name" + rn);
	if (cmd.size() == 1)
		serv->ActivateList(client);// 하나만 입력하면 사용 가능한 모든 채널 열람
	else
	{
		if (cmd[1].find(",") == std::string::npos)
			target.push_back(cmd[1]); // 하나만 입력함
		else						  //,가 있음 두개 이상 입력했을 가능성
			target = irc_utils::Split(cmd[1], ',');
		for (std::vector<std::string>::iterator it = target.begin(); it != target.end(); it++)
			serv->ActivateList(client, *it);
	}
	client->PushSendQueue(":irc.local 323 " + nickname + " :End of channel list." + rn);
}

/**
 * @note 클라이언트가 서버가 열려있는지 확인하는 매크로 명령어
 * @note 클라이어트가 서버로 PING 메시지를 보내면, 서버는 PONG 메시지로 응답해 연결이 활성 상태임을 알려줌
*/
void Command::ping(Client *client) { client->PushSendQueue(":irc.local PONG irc.local :irc.local" + rn); }

/**
 * @note 클라이언트를 종료하는 명령어
 * @note QUIT [<Quit message>]
 * @note 2번째 인자가 :으로 시작해야함. 없으면 무시하기로
*/
void Command::quit(Client *client)
{
	if (cmd.size() == 1)
	{
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return;
	}
	if (cmd[1][0] != ':')
		return;//두번째 인자가 :으로 시작안함. 무시
	if (client->getHostname().empty() || client->getPassword() == false)
		return;
	size_t msg_start = cmd[0].size();
	while (private_msg[msg_start] == ' ')
		msg_start++;
	std::string quit_msg = irc_utils::getForm(client, "QUIT :Quit: " + private_msg.substr(msg_start + 1));
	int socket = client->getClientSocket();
	std::string real = client->getRealname();
	std::string host = client->getHostname();
	client->PushSendQueue("ERROR :Closing link: (" + real + "@" + host + ") [Quit: " + cmd[1].substr(1) + rn);
	serv->SendMessageToAllJoinedChannel(socket, quit_msg);
	serv->RemoveClientFromServer(socket);
}

/**
 * @note 채널 운영자가 특정 참가인원을 강퇴하는 명령어
 * @note KICK <channel> <user> [<comment>]
 * @note 4번째 인자가 :으로 시작해야함. 없으면 무시하기로
*/
void Command::kick(Client *client)
{
	if (cmd.size() < 4)
	{
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return;
	}
	if (cmd[3][0] != ':')
		return;
	if (client->getHostname().empty() || client->getPassword() == false)
		return;
	std::string nickname = client->getNickname();
	std::string channel = cmd[1];
	std::string target = cmd[2];
	int	socket = client->getClientSocket();
	int target_socket = serv->getClientSocket(target);
	if (serv->HasDuplicateNickname(target) == false)
	{ // 유저 없음
		client->PushSendQueue(":irc.local 401 " + nickname + " " + target + " :No such nick" + rn);
		return;
	}
	else if (serv->HasChannel(channel) == false)
	{ // 채널이 없음
		client->PushSendQueue(":irc.local 442 " + nickname + " " + channel + " :You're not on that channel!" + rn);
		return;
	}
	else if (serv->HasClientInChannel(target_socket, channel) == false)
	{// 채널은 존재하는데 그곳에 해당 유저가 없음
		client->PushSendQueue(":irc.local 441 " + nickname + " " + target + " " + channel + " :They are not on that channel" + rn);
		return;
	}
	else if (serv->IsChannelOwner(socket, channel) == false)
	{//권한이 없음
		client->PushSendQueue(":irc.local 482 "+ nickname + " " + channel + " :You must be a channel op or higher to kick a more privileged user." + rn);
		return;
	}
	serv->SendMessageToAllClientsInChannel(channel, irc_utils::getForm(client, private_msg));
	serv->RemoveClientFromChannel(target_socket, channel);
}

/**
 * @note 운영자가 특정 채널에 사용자를 초대하는 명령어
 * @note INVITE <nickname> <channel>
 * @note 특이사항
 * @note 하나만 입력하면 채널을 알아서 넣음. 그때는 클라이언트 닉네임을 넣어야함.
 * @note 두개를 입력하면 그대로 들어감. 입력할때 채널은 #을 붙여서 입력해야 제대로 인식함
 * @note 세개 이상 입력하면 알아서 인자 두개만 넣어서 보내고 나머지는 버림 => 즉 무조건 인자가 3개로 들어오는게 보장됨
*/
void	Command::invite(Client *client)
{
	if (cmd.size() < 2)
	{
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return ;
	}
	if (client->getHostname().empty() || client->getPassword() == false)
		return;
	std::string channel_name = cmd[2];
	std::string nick_name = client->getNickname();
	std::string invited = cmd[1];
	if (serv->HasChannel(cmd[2]) == false)
	{//해당 채널이 없음.
		client->PushSendQueue(":irc.local 403 " + nick_name + " " + channel_name + " :No such channel" + rn);
		return;
	}
	else if (serv->HasDuplicateNickname(invited) == false)
	{//해당 닉네임 없음 = 해당 유저 없음
		client->PushSendQueue(":irc.local 401 " + nick_name + " " + invited + " :No such nick" + rn);
		return;
	}
	else if (serv->HasClientInChannel(serv->getClientSocket(invited), channel_name))
	{//이미 있는 사람 초대했으면??
		client->PushSendQueue(":irc.local 443 " + nick_name + " " + invited + " " + channel_name + " :is already on channel" + rn);
		return;
	}
	else if (serv->IsChannelOwner(client->getClientSocket(), channel_name) == false)
	{//권한이 없음
		client->PushSendQueue(":irc.local 482 " + invited + " " + channel_name + " :You must be a channel op or higher to send an invite." + rn);
		return;
	}
	client->PushSendQueue(":irc.local 341 " + nick_name + " " + invited + " :" + channel_name + rn);
	serv->PushSendQueueClient(serv->getClientSocket(invited), irc_utils::getForm(client, "INVITE " + invited + " :" + channel_name + rn));
	serv->AddInviteClient(channel_name, invited);
}

/**
 * @note 운영자가 채널의 주제를 정하는 명령어
 * @note 입력 양식 : TOPIC [<topic>]
 * @note 전송 양식 : TOPIC <channel> [<topic>]
 * @note 3번째 인자가 :으로 시작해야함. 없으면 무시하기로
*/
void	Command::topic(Client *client)
{
	std::string nickname = client->getNickname();
	int socket = client->getClientSocket();
	if (cmd.size() < 3)
	{
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return ;
	}
	if (cmd[2][0] != ':')
		return;//:으로 시작 안하니 무시
	if (client->getHostname().empty() || client->getPassword() == false)
		return;
	std::string channel = cmd[1];
	if (serv->HasChannel(channel) == false)
	{//채널이 없음
		client->PushSendQueue(":irc.local 403 " + nickname + " " + channel + " :No such channel" + rn);
	}
	else if (serv->HasModeInChannel('t', channel) == false || serv->IsChannelOwner(socket, channel))
	{//채널에 t 모드가 없거나 권한이 있으니 topic 설정 가능
		size_t msg_start = cmd[0].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		msg_start += cmd[1].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		serv->SetTopicInChannel(channel, private_msg.substr(msg_start + 1),  client->getRealname() + "@" + client->getHostname());
		serv->SendMessageToAllClientsInChannel(channel, irc_utils::getForm(client, private_msg));
	}
	else
		client->PushSendQueue(get_reply_number(ERR_CHANOPRIVSNEEDED) + get_reply_str(ERR_CHANOPRIVSNEEDED, nickname, cmd[1]));
}

/**
 * @note 채널 운영자가 채널의 상태를 변경하는 명령어
 * @note MODE <channel> {[+|-]|i|t|k|o|l} [<limit>] [<user>] [<ban mask>]
 * @note 없는 옵션 빼려고 할 때, 있는 옵션 등록하려 할 때는 서버에서 반응 x
 * @note - i: 초대 전용 채널로 설정 및 해제
 * @note - t: 채널 관리자가 TOPIC 명령어 제한 설정 및 해제 -> TOPIC 명령어를 운영자만 사용할 수 있는지 여부
 * @note - k: 채널 비밀번호 설정 및 해제
 * @note - o: 채널 관리자 특권 부여 및 제거
 * @note - l: 채널에서 유저 제한 설정 및 해제
*/
void Command::mode(Client *client)
{
	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return ;
	}
	std::string hostname = client->getHostname();
	if (hostname.empty() || client->getPassword() == false)
		return;
	std::string channel = cmd[1];
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	int socket = client->getClientSocket();
	std::string str = (":" + nickname + "!" + realname + "@" + hostname + " MODE " + channel + " ");
	std::vector<std::string> args;

	if (cmd.size() == 3 && cmd[1][0] != '#' && cmd[2] == "+i") //사용자가 서버에 입장할 때 사용자의 모드를 +i로 바꾸는 작업이 있음
		client->PushSendQueue(":" + nickname + "!" + realname + "@" + hostname + " MODE " + nickname + " :+i" + rn);
	else if (cmd[1][0] == '#' && !serv->HasChannel(channel))
		client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nickname, channel));
	else if (!serv->HasChannel(channel))
			client->PushSendQueue(get_reply_number(ERR_NOSUCHNICK) + get_reply_str(ERR_NOSUCHNICK, nickname, channel));
	else if (cmd.size() == 2)
	{
		client->PushSendQueue(":irc.local 324 " + nickname + " " + channel + " :+" + serv->GetModeFromChannel(channel) + rn);
		client->PushSendQueue(":irc.local 329 " + nickname + " " + channel + " :" + serv->GetChannelStartedTime(channel) + rn);
	}
	else if (cmd[2][0] == 'b')
		client->PushSendQueue(":irc.local 368 " + nickname + " " + channel + " :End of channel ban list" + rn);
	else if (cmd[2][0] == '+')
	{
		size_t idx = 3;
		std::string options = "";
		for (size_t i = 1; i < cmd[2].size(); i++)
		{
			char mode = cmd[2][i];
			switch (mode)
			{
			case 'i':
				if (!serv->IsChannelOwner(socket, channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to set channel mode i (inviteonly)." + rn);
				else if (!serv->HasModeInChannel('i', channel)) {
					serv->SetModeToChannel('i', channel);
					options += "i";
				}
				break;
			case 't':
				if (!serv->IsChannelOwner(socket, channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to unset channel mode t (topiclock)." + rn);
				else if (!serv->HasModeInChannel('t', channel)) {
					serv->SetModeToChannel('t', channel);
					options += "t";
				}
				break;
			case 'k':
				if (cmd.size() < idx + 1)
					client->PushSendQueue(":irc.local 696 " + nickname + " " + channel + " k * :You must specify a parameter for the key mode. Syntax: <key>." + rn);
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to unset channel mode k (key)." + rn);
				else if (!serv->HasModeInChannel('k', channel))
				{
					serv->SetModeToChannel('k', channel);
					serv->SetPasswordInChannel(cmd[idx], channel);
					options += "k";
					args.push_back(cmd[idx]);
					idx++;
				}
				break;
			case 'o':
				if (cmd.size() < idx + 1)
					client->PushSendQueue(":irc.local 696 " + nickname + " " + channel + " o * :You must specify a parameter for the key mode. Syntax: <nick>." + rn);
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to unset channel mode o (op)." + rn);
				else if (!serv->HasClientInChannel(cmd[idx], channel))
					client->PushSendQueue(":irc.local 401 " + nickname + " " + cmd[idx] + " :No such nick" + rn);
				else
				{
					serv->AddChannelOwner(cmd[idx], channel);
					options += "o";
					args.push_back(cmd[idx]);
					idx++;
				}
				break;
			case 'l':
				if (cmd.size() < idx + 1)
					client->PushSendQueue(":irc.local 696 " + nickname + " " + channel + " l * :You must specify a parameter for the key mode. Syntax: <limit>." + rn);
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to set channel mode l (limit)." + rn);
				else if (!serv->HasModeInChannel('l', channel))
				{
					if (cmd[idx].size() > 9) {
						client->PushSendQueue(":irc.local 696 "+ nickname + " " + channel + " l " + cmd[idx] + " :Invalid limit mode parameter. Syntax: <limit>." + rn);
						break;
					}
					std::istringstream ss(cmd[idx]);
					int limit;
					ss >> limit;
					if (limit < 0 || limit > INT_MAX)
						client->PushSendQueue(":irc.local 696 "+ nickname + " " + channel + " l " + cmd[idx] + " :Invalid limit mode parameter. Syntax: <limit>." + rn);
					else {
						serv->SetModeToChannel('l', channel);
						serv->SetUsersLimitInChannel(static_cast<size_t>(limit), channel);
						options += "l";
						args.push_back(cmd[idx]);
						idx++;
					}
				}
				break;
			default:
				client->PushSendQueue(":irc.local 472 " + nickname + " " + mode + " :is not a recognised channel mode." + rn);
				break;
			}
		}
		if (idx == 3)
			str += ":";
		if (options.length() > 0)
			str += "+";
		str += options + " ";
		for (size_t i = 3; i < idx; i++)
		{
			if (idx == i + 1)
				str += ":";
			if (args.size()) {
				str += (args.front() + " ");
				args.erase(args.begin());
			}
		}
		if (options.length() > 0)
			serv->SendMessageToAllClientsInChannel(channel, str + rn);
	}
	else if (cmd[2][0] == '-')
	{
		size_t idx = 3;
		std::string options = "";
		for (size_t i = 1; i < cmd[2].size(); i++)
		{
			char mode = cmd[2][i];
			switch (mode)
			{
			case 'i':
				if (!serv->IsChannelOwner(socket, channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to set channel mode i (inviteonly)." + rn);
				else if (serv->HasModeInChannel('i', channel)) {
					serv->RemoveModeFromChannel('i', channel);
					options += "i";
				}
				break;
			case 't':
				if (!serv->IsChannelOwner(socket, channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to unset channel mode t (topiclock)." + rn);
				else if (serv->HasModeInChannel('t', channel)) {
					serv->RemoveModeFromChannel('t', channel);
					options += "t";
				}
				break;
			case 'k':
				if (!serv->IsChannelOwner(socket, channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to unset channel mode k (key)." + rn);
				else if (serv->HasModeInChannel('k', channel))
				{
					serv->RemoveModeFromChannel('k', channel);
					serv->SetPasswordInChannel("", channel);
					options += "k";
					args.push_back(cmd[idx]);
					idx++;
				}
				break;
			case 'o':
				if (cmd.size() < idx + 1)
					client->PushSendQueue(":irc.local 696 " + nickname + " " + channel + " o * :You must specify a parameter for the key mode. Syntax: <nick>." + rn);
				else if (!serv->IsChannelOwner(socket, channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to unset channel mode o (op)." + rn);
				else
				{
					serv->RemoveChannelOwner(cmd[idx], channel);
					options += "o";
					args.push_back(cmd[idx]);
					idx++;
				}
				break;
			case 'l':
				if (!serv->IsChannelOwner(socket, channel))
					client->PushSendQueue(":irc.local 482 " + nickname + " " + channel + " :You must be a channel op or higher to set channel mode l (limit)." + rn);
				else if (serv->HasModeInChannel('l', channel))
				{
					serv->RemoveModeFromChannel('l', channel);
					serv->SetUsersLimitInChannel(0, channel);
					options += "l";
				}
				break;
			default:
				client->PushSendQueue(":irc.local 472 " + nickname + " " + mode + " :is not a recognised channel mode." + rn);
				break;
			}
		}
		if (idx == 3)
			str += ":";
		if (options.length() > 0)
			str += "-";
		str += options + " ";
		for (size_t i = 3; i < idx; i++)
		{
			if (idx == i + 1)
				str += ":";
			if (args.size()) {
				str += (args.front() + " ");
				args.erase(args.begin());
			}
		}
		if (options.length() > 0)
			serv->SendMessageToAllClientsInChannel(channel, str + rn);
	}
}

/**
 * @note 채널 입장 시 join -> mode -> who 순으로 동작
*/
void Command::who(Client *client)
{
	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, cmd[0]));
		return ;
	}
	std::string hostname = client->getHostname();
	if (hostname.empty() || client->getPassword() == false)
		return;
	std::string channel = cmd[1];
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	std::string clients_list =  serv->ClientsInChannelList(channel);
	std::vector<std::string> clients_vec = irc_utils::Split(clients_list, ' ');

	if (!serv->HasChannel(channel)) {
		client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nickname, channel));
	}

	if ((cmd.size() == 2 && cmd[1][0] == '#')) {
		for (size_t i = 0; i < clients_vec.size(); i++) {
			if (clients_vec[i][0] == '@') {
				client->PushSendQueue(":irc.local 352 " + nickname + " " + cmd[1] + " " + realname +\
									  " " + hostname + " irc.local " + clients_vec[i].substr(1) + " H@ :0 " + realname + rn);
			}
			else
				client->PushSendQueue(":irc.local 352 " + nickname + " " + cmd[1] + " " + realname + \
									  " " + hostname + " irc.local " + clients_vec[i] + " H :0 " + realname + rn);
		}
		client->PushSendQueue(":irc.local 315 " + nickname + " " + channel + " :End of /WHO list." + rn);
	}

	if (cmd.size() > 3) {
		std::vector<std::string> args = irc_utils::Split(cmd[2], ',');
		if (args[1] == "743") {
			for (size_t i = 0; i < clients_vec.size(); i++) {
				if (clients_vec[i][0] == '@') {
					client->PushSendQueue(":irc.local 354 " + nickname + " " + cmd[1] + " " + realname +\
										" " + hostname + " " + clients_vec[i].substr(1) + " H@ 0 0 :" + realname + rn);
				}
				else
					client->PushSendQueue(":irc.local 354 " + nickname + " " + cmd[1] + " " + realname + \
										" " + hostname + " " + clients_vec[i] + " H 0 0 :" + realname + rn);
			}
			client->PushSendQueue(":irc.local 315 " + nickname + " " + channel + " :End of /WHO list." + rn);
		}
		else if (args[1] == "745") {
			client->PushSendQueue(":irc.local 354 " + nickname + " 745 " + cmd[1] + " :0" + rn);
			client->PushSendQueue(":irc.local 315 " + nickname + " " + channel + " :End of /WHO list." + rn);
		}
	}
}

/**
 * @note 클라이언트가 최초 접속시 날리는 명령어
*/
void Command::cap(Client *client)
{
	if (client->getHostname().empty() && client->getNickname().empty())
		client->PushSendQueue(":irc.local Connecting..." + rn);
}

/**
 * @note cmd와 private_msg가 잘 처리가 되었는지 확인해주는 디버깅용 함수
 * @note 디버깅용 함수이니 나중에 주석 처리하든 말든 결정 할 것!
*/
void Command::DebugFtForCmdParssing()
{
	std::cout << "let's checkout result :" ;
	std::string str;
	for (std::vector<std::string>::iterator it = cmd.begin(); it != cmd.end(); it++)
	{
		str = *it;
		if (it == cmd.begin())
			std::cout << " \"";
		else
			std::cout << " ";
		for (size_t i = 0; i < str.size(); i++)
		{
			if (str[i] == '\r')
				std::cout << "\\r";
			else if (str[i] == '\n')
				std::cout << "\\n";
			else
				std::cout << str[i];
		}
	}
	std::cout << "\"\nAnd private_msg is : \"";
	for (size_t i = 0; i < private_msg.size(); i++)
	{
		if (private_msg[i] == '\r')
			std::cout << "\\r";
		else if (private_msg[i] == '\n')
			std::cout << "\\n";
		else
			std::cout << private_msg[i];
	}
	std::cout << "\"\nI hope you well done it!\n" << std::endl;
}
