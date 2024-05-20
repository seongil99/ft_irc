#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <unistd.h>

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
	if (str.size() < 3)
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
			if (temp.empty() == false)
				cmd.push_back(temp);
			temp.clear();
		}
		else if (str[i] == '\n' && i != str.size() - 1 && i > 0)
		{//=====================================================
			/*
				이 코드의 존재 이유
				클라이언트가 비번을 쏘며 접속 시도하면 NICK과 USER를 한 번에 쏴버립니다....
				그것 때문에 어쩔 수 없이 이런 논리를 사용할 수 밖에 없었습니다....
			*/
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
				// DebugFtForCmdParssing();//주석 처리할 거면 할 것!
				temp.clear();
				cmd.clear();
				prevented_idx = i + 1;
				private_msg = str.substr(prevented_idx);
			}
		}//=====================================================
		else if (str[i] != '\r' && str[i] != '\n')
			temp += str[i];
	}
	if (temp.empty() == false)
		cmd.push_back(temp);
	//==========================================================
	// step 2 : figure it out is this cmd or not
	it = cmd_ft.find(cmd[0]);
	if (it != cmd_ft.end())
	{
		ret = true;
		(this->*(it->second))(client);
	}
	//==========================================================
	// DebugFtForCmdParssing();//주석 처리할 거면 할 것!
	// client 삭제를 대비해서 밑에 그 어느것도 있으면 안됨!!
	cmd.clear();
	return ret;
}

/**
 * @note 이 명령어는 채널 입장용 비번 명령어가 아닌 서버 입장용 비번 명령어임.
 * @note PASS <password>
*/
void Command::pass(Client *client)
{ // PASS <password>
	/*
	비번에 관한 경우의 3가지
	1. 서버 비번 없음 => 비번 입력하든 말든 프리패스 해야함
	2. 서버 비번 있음
	2-1 비번 맞음 => 통과
	2-2 비번 틀림 => 통과 못함
	*/
	if (cmd.size() == 1)
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "PASS"));
	else
	{
		if (serv->CheckPassword(""))
			serv->CorrectPassword(client);
		else if (serv->CheckPassword(cmd[1]))
			serv->CorrectPassword(client);
	}
}

/**
 * @note 닉네임 변경하는 명령어
*/
void Command::nick(Client *client)
{ // NICK <nickname>
	std::string nickname = client->getNickname();

	if (cmd.size() == 1) {
		if (nickname.empty())  //닉네임 없이 입장했을 때
			client->PushSendQueue(get_reply_number(ERR_NONICKNAMEGIVEN) + get_reply_str(ERR_NONICKNAMEGIVEN) + "\r\n");
	}
	else
	{
		// 닉네임 중복 여부 판단
		if (serv->HasDuplicateNickname(cmd[1]))
		{ 
			if (client->getNickname().empty() && client->getHostname().empty())
			{
				//최초 접속시 중복된 닉네임을 원함.
				client->PushSendQueue("ERROR :Closing link: (root@192.168.65.2) [Access denied by dup-Nickname issue]\nPlease access again with other nickname!" + rn);
				return;
			}
			client->PushSendQueue(get_reply_number(ERR_NICKNAMEINUSE) + nickname + " " + cmd[1] + " :Nickname is already in use." + rn);
		}
		else// 닉네임 중복이 안되었으니 닉네임 변경
		{
			/*
			127.000.000.001.54498-127.000.000.001.06667: NICK test

			127.000.000.001.06667-127.000.000.001.54498: :upper!root@127.0.0.1 NICK :test
			*/
			if (!client->getRealname().empty())
				client->PushSendQueue(":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " NICK :" + cmd[1] + rn);
			client->setNickname(cmd[1]);
		}
	}
	// NICK aaa bbb ccc ddd 이런 식으로 여러개 쳤을때는 닉네임이 aaa로 바뀌고 다른 반응 없음
}

/**
 * @note 최초 입장시 호스트, 실명, 입장여부를 정하는 명령어
*/
void Command::user(Client *client)
{ // USER <username> <hostname> <servername> :<realname>
	// 연결이 시작될 때 사용자의 사용자명, 실명 지정에 사용
	// 실명 매개변수는 공백 문자를 포함할 수 있도록 마지막 매개변수여야 하며, :을 붙여 인식하도록 함
	if (cmd.size() < 5)
	{ // 인자를 적게 넣었을 경우
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + client->getNickname() + " " + get_reply_str(ERR_NEEDMOREPARAMS, "USER"));
		return;
	}
	if (client->getRealname().size() != 0) // 사용자가 USER 명령어를 내렸을 경우 또는 최초 호출인데 뭔가 잡것이 있는 경우
	{
		client->PushSendQueue(get_reply_number(ERR_ALREADYREGISTRED) + client->getNickname() + " " + get_reply_str(ERR_ALREADYREGISTRED));
		return;
	}
	if (cmd[4][0] != ':')
	{//realname 맨 앞에 ':'이 없음
		client->PushSendQueue("ERROR :Closing link: (root@192.168.65.2) [Access denied by Realname issue]\nrealname doesn't start with \':\'!" + rn);
		return;
	}
	//이럴 경우는 접속할때 중복된 닉네임으로 접속시도한 경우이므로 여기서 멈춰야됨
	if (client->getNickname() == "")
		return ;
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
	if ((serv->CheckPassword("") || client->getPassword()) == false)//서버가 비번이 설정 되어 있으며, 클라이언트가 비번 못 맞춘 경우
	{
		//ERROR :Closing link: (root@192.168.65.2) [Access denied by configuration]
		client->PushSendQueue("ERROR :Closing link: (" + realname + "@" + hostname + ") [Access denied by configuration]" + rn);
		return ;
	}

	client->PushSendQueue(get_reply_number(RPL_WELCOME) + nickname + get_reply_str(RPL_WELCOME, nickname, realname, hostname));
	client->PushSendQueue(get_reply_number(RPL_YOURHOST) + nickname + get_reply_str(RPL_YOURHOST, "irc.local", "ft_irc"));
	client->PushSendQueue(get_reply_number(RPL_CREATED) + nickname + get_reply_str(RPL_CREATED, serv->getStartedTime()));
	client->PushSendQueue(get_reply_number(RPL_MYINFO) + get_reply_str(RPL_MYINFO, nickname, "irc.local", "OUR", "FT_IRC"));
	// client->PushSendQueue(":irc.local 005 " + nickname + " AWAYLEN=200 CASEMAPPING=rfc1459 CHANLIMIT=#:20 CHANMODES=b,k,l,imnpst CHANNELLEN=64 CHANTYPES=# ELIST=CMNTU HOSTLEN=64 KEYLEN=32 KICKLEN=255 LINELEN=512 MAXLIST=b:100 :are supported by this server" + rn);
	// client->PushSendQueue(":irc.local 005 " + nickname + " MAXTARGETS=20 MODES=20 NAMELEN=128 NETWORK=Localnet NICKLEN=30 PREFIX=(ov)@+ SAFELIST STATUSMSG=@+ TOPICLEN=307 USERLEN=10 USERMODES=,,s,iow WHOX :are supported by this server" + rn);
	client->PushSendQueue(":irc.local 005 " + nickname + " Enjoy our ft_irc." + rn);
}

/**
 * @note 서버 입장 및 채널 입장하는 명령어
 * @note Join (ch1,ch2,...chn) (pw1,pw2,...,pwn)
*/
void Command::join(Client *client)
{
	std::vector<std::string> channel, pw;

	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "JOIN"));
		return ;
	}

	if (cmd.size() == 2 && cmd[1] == ":")
	{
		client->PushSendQueue(":irc.local 451 * JOIN :You have not registered." + rn);
		return;
	}

	channel = irc_utils::Split(cmd[1], ',');
	pw = irc_utils::Split(cmd[2], ',');
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	std::string hostname = client->getHostname();
	int socket = client->getClientSocket();

	for (size_t i = 0; i < channel.size(); i++)
	{
		// 채널이 없으면 채널을 만든다
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
			client->PushSendQueue("irc.local 405 " + nickname + " " + get_reply_str(ERR_TOOMANYCHANNELS, channel[i]) + rn);
		else
		{
			serv->AddClientToChannel(*client, channel[i]);
			client->PushSendQueue(":" + nickname + "!" + realname + "@" + \
								  hostname + " JOIN :" + channel[i] + rn);
			if (serv->HasTopicInChannel(channel[i]))
			{
				client->PushSendQueue(":irc.local 332 " + nickname + " " + channel[i] + " :" + serv->GetTopicInChannel(channel[i]) + rn);
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
*/
void Command::part(Client *client)
{
	/*
	/part good bye ->라고 입력하면
	:<nick>!<real>@<host> PART #hi :good bye ->  이걸 클라이언트전부에게 뿌림

	log
	127.000.000.001.33312-127.000.000.001.06667: PART #hi :good bye // <= 이건 서버가 받은 메시지

	127.000.000.001.06667-127.000.000.001.33312: :test2!root@127.0.0.1 PART #hi :good bye //<= 이건 같은 방에 있던 클라이언트

	127.000.000.001.06667-127.000.000.001.33310: :test2!root@127.0.0.1 PART #hi :good bye //<= 이건 part친 본인

	현재 결과
	172.017.000.002.58140-192.168.065.002.08080: PART #hi :good bye

	192.168.065.002.08080-172.017.000.002.58140: :klha!root@127.0.0.1 PART #hi :good bye
	*/
	std::vector<std::string> channel = irc_utils::Split(cmd[1], ',');
	std::string nick_name = client->getNickname();
	int	socket = client->getClientSocket();
	std::string msg = irc_utils::getForm(client, cmd[0] + " :" + cmd[1] + rn);
	if (cmd.size() != 2)//나가는 메시지를 남김
		msg =irc_utils::getForm(client, private_msg);
	// private_msg의 마지막에 \r\n이 있어서 따로 추가 안해도 됨

	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "PART"));
		return ;
	}

	for (std::vector<std::string>::iterator it = channel.begin(); it != channel.end(); it++)
	{
		if (serv->HasChannel(*it) == false)
		{//그런 채널 없는데? -> 그럼 현재 채널 나가
		/* 현재 1이란 채널에서 /pass get out 사용
		-> 그럼 클라이언트가 /pass #1 :get out으로 던짐 이건 여기에 안들어와짐
		여기에 들어오는 건 status에서 똑같은 명령어를 쓴다면 이렇게 됨
		127.000.000.001.49828-127.000.000.001.06667: PART get :out
		127.000.000.001.06667-127.000.000.001.49828: :irc.local 403 upper get :No such channel
		*/
			client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nick_name, *it));
		}
		else if (serv->HasClientInChannel(nick_name, *it) == false)
		{//채널 있기는 한데 넌 안들어가 있는데? -> 오류 메시지만 보내고 끝
		/*
		127.000.000.001.49828-127.000.000.001.06667: PART #4 :hwy?
		127.000.000.001.06667-127.000.000.001.49828: :irc.local 442 upper #4 :You're not on that channel
		*/
			client->PushSendQueue(get_reply_number(ERR_NOTONCHANNEL) + get_reply_str(ERR_NOTONCHANNEL, nick_name, *it));
		}
		/*
		나간 사람이 마직막 운영자면 -> 해당 채널에 운영자가 없는 채널이 되어 버림....
		나간 사람이 마지막 사람이라면 채널 자체를 없애버려야 한다.
		*/
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
*/
void Command::privmsg(Client *client)
{
	std::vector<std::string> target; // 귓말 받을 사람 모음

	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "PRIVMSG"));
		return ;
	}

	target = irc_utils::Split(cmd[1], ',');
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	std::string hostname = client->getHostname();
	int socket = client->getClientSocket();
	size_t msg_start = cmd[0].size();
	while (private_msg[msg_start] == ' ')
		msg_start++;
	msg_start += cmd[1].size();
	while (private_msg[msg_start] == ' ')
		msg_start++;
	std::string msg = private_msg.substr(msg_start + 1);
	msg.erase(msg.size() - 2);

	for (size_t i = 0; i < target.size(); i++) {
		if (target[i][0] == '#') {
			if (serv->HasChannel(target[i]) && serv->HasClientInChannel(socket, target[i]))
				serv->SendMessageToOthersInChannel(socket, target[i], \
					":" + nickname + "!" + realname + "@" + hostname + " PRIVMSG " + target[i] + " :" + msg + rn);
			else if (serv->HasChannel(target[i]))
				client->PushSendQueue(":irc.local 404 " + client->getNickname() + " " + get_reply_str(ERR_CANNOTSENDTOCHAN, target[i]));
			else
				client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nickname, target[i]));
		}
		else {
			if (!serv->HasDuplicateNickname(target[i]))//:irc.local 401 middle 2 :No such nick
				client->PushSendQueue(":irc.local 401 " + nickname + " " + target[i] + " :No such nick" + rn);
			else {
				serv->SendMessageToOtherClient(socket, target[i], \
				":" + nickname + "!" + realname + "@" + hostname + " PRIVMSG " + target[i] + " :" + msg + rn);
			}
		}
	}
}

/**
 * @note 현재 있는 채널을 조회하는 명령어
 * @note LIST [<channel>{,<channel>} [<server>]]
*/
void Command::list(Client *client)
{ 
	std::string nickname = client->getNickname();
	std::vector<std::string> target;
	client->PushSendQueue(":irc.local 321 " + nickname + " Channel :Users Name" + rn);
	if (cmd.size() == 1)
	{ // 하나만 입력하면 사용 가능한 모든 채널 열람
		/*
		127.000.000.001.49916-127.000.000.001.06667: LIST
		127.000.000.001.06667-127.000.000.001.49916: :irc.local 321 lower Channel :Users Name
		:irc.local 322 lower #1 1 :[+nt]
		:irc.local 322 lower #2 1 :[+nt]
		:irc.local 322 lower #3 1 :[+nt]
		:irc.local 322 lower #4 1 :[+nt]
		:irc.local 322 lower #5 1 :[+nt]
		:irc.local 322 lower #6 1 :[+nt]
		양식 => :irc.local 322 <nick> <channel> <참가 인원수> :<모드> {topic}
		:irc.local 323 lower :End of channel list.
		*/
		serv->ActivateList(client);
	}
	else
	{
		if (cmd[1].find(",") == std::string::npos)
			target.push_back(cmd[1]); // 하나만 입력함
		else						  //,가 있음 두개 이상 입력했을 가능성
			target = irc_utils::Split(cmd[1], ',');
		for (std::vector<std::string>::iterator it = target.begin(); it != target.end(); it++)
			serv->ActivateList(client, *it);
	}
	// 세번째 인자는 아마 다수의 서버에 대한 처리겠지만 우린 우리 서버 하나니까 무시
	client->PushSendQueue(":irc.local 323 " + nickname + " :End of channel list." + rn);
}

/**
 * @note 클라이언트가 서버가 열려있는지 확인하는 매크로 명령어
*/
void Command::ping(Client *client) // ping을 받은 상황
{								   // PING <server1> [<server2>]
	client->PushSendQueue(":irc.local PONG irc.local :irc.local" + rn);
	// 클라이어트가 서버로 PING 메시지를 보내면, 서버는 PONG 메시지로 응답해 연결이 활성 상태임을 알려줌
}

/**
 * @note 클라이언트를 종료하는 명령어
 * @note QUIT [<Quit message>]
*/
void Command::quit(Client *client)
{
	// 나갈때는 모두 다 보내면 되지 않나
	std::string quit_msg;
	if (cmd.size() == 1)
	{ // QUIT만 침
		// 원본:lower!root@127.0.0.1 QUIT :Quit: leaving
		quit_msg = irc_utils::getForm(client, "QUIT :Quit: leaving" + rn);
	}
	else
	{ // 내보낼때 메시지도 같이 침
		/*
		127.000.000.001.33378-127.000.000.001.06667: QUIT :bye

		127.000.000.001.06667-127.000.000.001.33378: ERROR :Closing link: (root@127.0.0.1) [Quit: bye] //-> 요건 일단 보류

		127.000.000.001.06667-127.000.000.001.33376: :lower!root@127.0.0.1 QUIT :Quit: bye
		*/
		quit_msg = irc_utils::getForm(client, "QUIT :Quit: " + private_msg.substr(6));
	}
	int socket = client->getClientSocket();
	std::string real = client->getRealname();
	std::string host = client->getHostname();
	// quit 명령어를 내렸다는 것을 참가했던 모든 채널의 클라이언트에게 본인제외하고 다 보내야함.
	client->PushSendQueue("ERROR :Closing link: (" + real + "@" + host + ") [Quit: " + cmd[1].substr(1));
	serv->SendMessageToAllJoinedChannel(socket, quit_msg);
	// 아래는 할 것 다하고 호출!
	serv->RemoveClientFromServer(socket);
	// 클라이언트가 삭제되기에 더이상 해당 클라이언트를 부르면 안됨!
}

/**
 * @note 채널 운영자가 특정 참가인원을 강퇴하는 명령어
 * @note KICK <channel> <user> [<comment>]
*/
void Command::kick(Client *client)
{
/*
127.000.000.001.49876-127.000.000.001.06667: KICK #1 lower :

127.000.000.001.06667-127.000.000.001.49876: :upper!root@127.0.0.1 KICK #1 lower :

127.000.000.001.06667-127.000.000.001.49872: :upper!root@127.0.0.1 KICK #1 lower :
*/
	// KICK #1 get :out

	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "KICK"));
		return ;
	}

	std::string nickname = client->getNickname();
	std::string channel = cmd[1];
	std::string target = cmd[2];
	int	socket = client->getClientSocket();
	int target_socket = serv->getClientSocket(target);
	if (serv->HasDuplicateNickname(target) == false)
	{ // 유저 음슴
		//:irc.local 401 upper get :No such nick
		client->PushSendQueue(":irc.local 401 " + nickname + " " + target + " :No such nick" + rn);
		return;
	}
	else if (serv->HasChannel(channel) == false)
	{ // 채널이 음슴
		// :irc.local 442 upper #2 :You're not on that channel!
		client->PushSendQueue(":irc.local 442 " + nickname + " " + channel + " :You're not on that channel!" + rn);
		return;
	}
	else if (serv->HasClientInChannel(target_socket, channel) == false)
	{// 채널은 존재하는데 그곳에 해당 유저가 음슴
		// :irc.local 441 upper lower #1 :They are not on that channel
		client->PushSendQueue(":irc.local 441 " + nickname + " " + target + " " + channel + " :They are not on that channel" + rn);
		return;
	}
	else if (serv->IsChannelOwner(socket, channel) == false)
	{//권한이 없음
		// :irc.local 482 lower #1 :You must be a channel op or higher to kick a more privileged user.
		client->PushSendQueue(":irc.local 482 "+ nickname + " " + channel + " :You must be a channel op or higher to kick a more privileged user." + rn);
		return;
	}
	// 해당 채널에서 강퇴했다는 로그 발송
	serv->SendMessageToAllClientsInChannel(channel, irc_utils::getForm(client, private_msg));
	// 해당 채널에서 강퇴
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
	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "KICK"));
		return ;
	}
	std::string channel_name = cmd[2];
	std::string nick_name = client->getNickname();
	std::string invited = cmd[1];
	if (serv->HasChannel(cmd[2]) == false)
	{//해당 채널이 없음.
		//":irc.local 403 middle lower :No such channel"
		client->PushSendQueue(":irc.local 403 " + nick_name + " " + channel_name + " :No such channel" + rn);
		return;
	}
	else if (serv->HasDuplicateNickname(invited) == false)
	{//해당 닉네임 없음 = 해당 유저 없음
		//:irc.local 401 middle lowe :No such nick
		client->PushSendQueue(":irc.local 401 " + nick_name + " " + invited + " :No such nick" + rn);
		return;
	}
	else if (serv->HasClientInChannel(serv->getClientSocket(invited), channel_name))
	{//이미 있는 사람 초대했으면??
		//:irc.local 443 upper middle #hi :is already on channel
		client->PushSendQueue(":irc.local 443 " + nick_name + " " + invited + " " + channel_name + " :is already on channel" + rn);
		return;
	}
	else if (serv->IsChannelOwner(client->getClientSocket(), channel_name) == false)
	{//권한이 없음
		//:irc.local 482 middle #hi :You must be a channel op or higher to send an invite.
		client->PushSendQueue(":irc.local 482 " + invited + " " + channel_name + " :You must be a channel op or higher to send an invite." + rn);
		return;
	}
	//초대하는 코드
	/*
	127.000.000.001.52370-127.000.000.001.06667: INVITE lower #hi

	127.000.000.001.06667-127.000.000.001.52370: :irc.local 341 upper lower :#hi

	127.000.000.001.06667-127.000.000.001.52390: :upper!root@127.0.0.1 INVITE lower :#hi
	*/
	//초대 발송자에게 결과 날리기
	client->PushSendQueue(":irc.local 341 " + nick_name + " " + invited + " " + channel_name + rn);
	//초대 수신자에게 날리기
	serv->PushSendQueueClient(serv->getClientSocket(invited), irc_utils::getForm(client, private_msg));
	//해당 채널에 초대받은 사람 초대 리스트에 넣기
	serv->AddInviteClient(channel_name, invited);
}

/**
 * @note 운영자가 채널의 주제를 정하는 명령어
 * @note TOPIC [<topic>]
*/
void	Command::topic(Client *client)
{
	std::string nickname = client->getNickname();
	int socket = client->getClientSocket();
	if (cmd.size() == 1)
	{
		//:irc.local 461 upper TOPIC :Not enough parameters.
		client->PushSendQueue(":irc.local 461 " + nickname + " TOPIC :Not enough parameters." + rn);
		return;
	}
	std::string channel = cmd[1];
	if (serv->HasChannel(channel) == false)
	{//채널이 음슴
		//:irc.local 403 upper hi :No such channel
		client->PushSendQueue(":irc.local 403 " + nickname + " " + channel + " :No such channel" + rn);
	}
	else if (serv->HasModeInChannel('t', channel) == false || serv->IsChannelOwner(socket, channel))
	{//채널에 t 모드가 없거나 권한이 있으니 topic 설정 가능
		//127.000.000.001.52292-127.000.000.001.06667: TOPIC <channel> :<topic>
		serv->SetTopicInChannel(channel, cmd[2].substr(1),  client->getRealname() + "@" + client->getHostname());
		serv->SendMessageToAllClientsInChannel(channel, irc_utils::getForm(client, private_msg));
	}//잘되는 것으로 보인다. 야호!
	else
	{
		/*
		원본
		127.000.000.001.52292-127.000.000.001.06667: TOPIC <channel> :<topic>

		우리것
		127.000.000.001.06667-127.000.000.001.52372: :irc.local 482 upper #hi :You must be a channel op or higher to change the topic.

		192.168.065.002.08080-172.017.000.002.52164: :irc.local 482 upper #hi :You must be a channel op or higher to change the topic.
		*/
		client->PushSendQueue(get_reply_number(ERR_CHANOPRIVSNEEDED) + get_reply_str(ERR_CHANOPRIVSNEEDED, nickname, cmd[1]));
	}
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
 * @note 추후 전체적인 코드 리팩토링 필요..
*/
void Command::mode(Client *client)
{
	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "MODE"));
		return ;
	}

	std::string channel = cmd[1];
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	std::string hostname = client->getHostname();
	int socket = client->getClientSocket();
	std::string str = (":" + nickname + "!" + realname + "@" + hostname + " MODE " + channel + " ");
	std::vector<std::string> args;

	// 사용자가 처음 서버에 진입할때 사용자 모드를 +i로 바꿔줌
	if (cmd[1][0] != '#' && cmd[2] == "+i")
		client->PushSendQueue(":" + nickname + "!" + realname + "@" + hostname + " MODE " + nickname + " :+i" + rn);
	else if (!serv->HasChannel(channel))
		client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nickname, channel) + rn);
	else if (cmd.size() == 2)
	{
		std::time_t now = std::time(0);
		std::string timestamp = std::to_string(now);
		client->PushSendQueue(":irc.local 324 " + nickname + " " + channel + " :+nt" + rn);
		client->PushSendQueue(":irc.local 329 " + nickname + " " + channel + " :" + timestamp + rn); // 시간 스탬프 값 필요
		serv->SetModeToChannel('t', channel);
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
				else if (!serv->HasClientInChannel(cmd[idx], channel)) {
					client->PushSendQueue(":irc.local 401 " + nickname + " " + cmd[idx] + " :No such nick" + rn);
					return;
				}
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
					int limit = std::stoi(cmd[idx]);
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
			default: // 해당하는 모드가 없을 때 처리
				client->PushSendQueue(":irc.local 472 " + nickname + " " + mode + " :is not a recognised channel mode." + rn);
				break;
			}
		}
		// 클라이언트에 보낼 string 완성
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
			default: // 해당하는 모드가 없을 때 처리
				client->PushSendQueue(":irc.local 472 " + nickname + " " + mode + " :is not a recognised channel mode." + rn);
				break;
			}
		}
		// 클라이언트에 보낼 string 완성
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
	std::string channel = cmd[1];
	std::string nickname = client->getNickname();
	std::string realname = client->getRealname();
	std::string hostname = client->getHostname();
	std::vector<std::string> args = irc_utils::Split(cmd[2], ',');
	std::string clients_list =  serv->ClientsInChannelList(channel);
	std::vector<std::string> clients_vec = irc_utils::Split(clients_list, ' ');

	if (cmd.size() == 2 && cmd[1][0] == '#') {
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
	else if (args[1] == "743") {
		for (size_t i = 0; i < clients_vec.size(); i++) {
			if (clients_vec[i][0] == '@') {
				client->PushSendQueue(":irc.local 354 " + nickname + " 743 " + cmd[1] + " " + realname +\
									  " " + hostname + " " + clients_vec[i].substr(1) + " H@ 0 0 :" + realname + rn);
			}
			else
				client->PushSendQueue(":irc.local 354 " + nickname + " 743 " + cmd[1] + " " + realname + \
									  " " + hostname + " " + nickname + " H 0 0 :" + realname + rn);
		}
		client->PushSendQueue(":irc.local 315 " + nickname + " " + channel + " :End of /WHO list." + rn);
	}
	else if (args[1] == "745") {
		client->PushSendQueue(":irc.local 354 " + nickname + " 745 " + cmd[1] + " :0" + rn);
		client->PushSendQueue(":irc.local 315 " + nickname + " " + channel + " :End of /WHO list." + rn);
	}
}

/**
 * @note 클라이언트가 최초 접속시 날리는 명령어
*/
void Command::cap(Client *client)
{
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
