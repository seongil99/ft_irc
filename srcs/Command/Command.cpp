#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <unistd.h>

Command::Command(Server *server) : serv(server)
{
	cmd_ft["PRIVMSG"] = &Command::privmsg;
	cmd_ft["PASS"] = &Command::pass;
	cmd_ft["NICK"] = &Command::nick;
	cmd_ft["USER"] = &Command::user;
	cmd_ft["JOIN"] = &Command::join;
	cmd_ft["LIST"] = &Command::list;
	cmd_ft["PING"] = &Command::ping;
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
 * return false : user typed just chat str
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
				// DebugFtForCmdParssing();
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
	// DebugFtForCmdParssing();
	// client 삭제를 대비해서 밑에 그 어느것도 있으면 안됨!!
	cmd.clear();
	return ret;
}

//이거 채팅방 비번이 아니라 서버 비번 관련 명령어인것 같은데?
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
				//이걸 해결하려면 명령어마다 스레드를 만들어서 다루게 하는 방법밖에 없음.
				client->PushSendQueue("ERROR :Closing link: (root@192.168.65.2) [Access denied by dup-Nickname issue]\nPlease access again with other nickname!\r\n");
				return;
			}
			client->PushSendQueue(get_reply_number(ERR_NICKNAMEINUSE) + nickname + " " + cmd[1] + " :Nickname is already in use.\r\n");
		}
		else// 닉네임 중복이 안되었으니 닉네임 변경
		{
			/*
			127.000.000.001.54498-127.000.000.001.06667: NICK test

			127.000.000.001.06667-127.000.000.001.54498: :upper!root@127.0.0.1 NICK :test
			*/
			if (!client->getRealname().empty())
				client->PushSendQueue(":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " NICK :" + cmd[1] + "\r\n");
			client->setNickname(cmd[1]);
		}
	}
	// NICK aaa bbb ccc ddd 이런 식으로 여러개 쳤을때는 닉네임이 aaa로 바뀌고 다른 반응 없음
}

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
		//이거 없다고 뭔가 전송해줘야 될 것 같은데 뭘 전송해줘야될지 모르겠음... 일단 리턴해서 끝는 걸로...
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

	client->PushSendQueue(":irc.local NOTICE " + nickname + " :*** Could not resolve your hostname: Request timed out; using your IP address (" + hostname + ") instead.\r\n");
	if ((serv->CheckPassword("") || client->getPassword()) == false)//서버가 비번이 설정 되어 있으며, 클라이언트가 비번 못 맞춘 경우
	{
		//ERROR :Closing link: (root@192.168.65.2) [Access denied by configuration]
		client->PushSendQueue("ERROR :Closing link: (" + realname + "@" + hostname + ") [Access denied by configuration]\r\n");
		return ;
	}

	client->PushSendQueue(get_reply_number(RPL_WELCOME) + nickname + get_reply_str(RPL_WELCOME, nickname, realname, hostname));
	client->PushSendQueue(get_reply_number(RPL_YOURHOST) + nickname + get_reply_str(RPL_YOURHOST, "irc.local", "ft_irc"));
	client->PushSendQueue(get_reply_number(RPL_CREATED) + nickname + get_reply_str(RPL_CREATED, serv->getStartedTime()));
	client->PushSendQueue(get_reply_number(RPL_MYINFO) + get_reply_str(RPL_MYINFO, nickname, "irc.local", "OUR", "FT_IRC"));
	// client->PushSendQueue(":irc.local 005 " + nickname + " AWAYLEN=200 CASEMAPPING=rfc1459 CHANLIMIT=#:20 CHANMODES=b,k,l,imnpst CHANNELLEN=64 CHANTYPES=# ELIST=CMNTU HOSTLEN=64 KEYLEN=32 KICKLEN=255 LINELEN=512 MAXLIST=b:100 :are supported by this server\r\n");
	// client->PushSendQueue(":irc.local 005 " + nickname + " MAXTARGETS=20 MODES=20 NAMELEN=128 NETWORK=Localnet NICKLEN=30 PREFIX=(ov)@+ SAFELIST STATUSMSG=@+ TOPICLEN=307 USERLEN=10 USERMODES=,,s,iow WHOX :are supported by this server\r\n");
	client->PushSendQueue(":irc.local 005 " + nickname + " Enjoy our ft_irc.\r\n");
	/*
	의문점
	1. 안에 들어갈 문장은 대충 복붙했는데 이걸 어찌혀야하나
	2. 사용자가 접속하고 나서 입력하면? -> 지금은 아무일도 안일어남.
	*/
}

// Join (ch1,ch2,...chn) (pw1,pw2,...,pwn)
void Command::join(Client *client)
{
	std::vector<std::string> channel, pw;

	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "JOIN"));
		return ;
	}

	if (cmd.size() == 2 && cmd[1] == ":")
	{
		client->PushSendQueue(":irc.local 451 * JOIN :You have not registered.\r\n");
		return;
	}

	channel = irc_utils::Split(cmd[1], ',');
	pw = irc_utils::Split(cmd[2], ',');

	for (size_t i = 0; i < channel.size(); i++)
	{
		// 채널이 없으면 채널을 만든다
		if (!serv->HasChannel(channel[i]))
			serv->CreateChannel(channel[i]);
		else
		{ // 채널이 있는 상태 -> 권한, 비밀번호 확인
			if (!serv->IsInvitedChannel(client->getClientSocket(), channel[i]))
			{
				client->PushSendQueue(":irc.local 473 " + client->getNickname() + " " + channel[i] + " :Cannot join channel (invite only)\r\n");
				continue;
			}
			else if (serv->HasChannelPassword(channel[i]))
			{
				if (i + 1 > pw.size() || pw[i].empty() || !serv->CheckChannelPassword(pw[i], channel[i]))
				{
					client->PushSendQueue(":irc.local 475 " + client->getNickname() + " " + channel[i] +
										  " :Cannot join channel (incorrect channel key)\r\n");
					continue;
				}
			}
			else if (serv->IsOverUsersLimitChannel(channel[i]))
			{
				client->PushSendQueue(":irc.local 471 " + client->getNickname() + " " + channel[i] +
									  " :Cannot join channel (channel is full)\r\n");
				continue;
			}
		}
		if (serv->HowManyChannelsJoined(client->getClientSocket()) >= 10) // 채널 10개 이상 속했을 때
			client->PushSendQueue("irc.local 405 " + client->getNickname() + " " + get_reply_str(ERR_TOOMANYCHANNELS, channel[i]) + "\r\n");
		else
		{
			serv->AddClientToChannel(*client, channel[i]);
			client->PushSendQueue(":" + client->getNickname() + "!" + client->getRealname() + "@" + \
								  client->getHostname() + " JOIN :" + channel[i] + "\r\n");
			if (serv->HasTopicInChannel(channel[i]))
			{
				client->PushSendQueue(":irc.local 332 " + client->getNickname() + " " + channel[i] + " :" + serv->GetTopicInChannel(channel[i]) + "\r\n");
				client->PushSendQueue(":irc.local 333 " + client->getNickname() + " " + channel[i] + " " +
									  serv->WhoDidTopicInChannel(channel[i]) + " :" + serv->WhatTimeChannelMade(channel[i]) + "\r\n");
			}
			client->PushSendQueue(":irc.local 353 " + client->getNickname() + " = " +
								  channel[i] + " :" + serv->ClientsInChannelList(channel[i]) + "\r\n");
			client->PushSendQueue(":irc.local 366 " + client->getNickname() + " " + channel[i] + " :End of /NAMES list.\r\n");
			serv->SendMessageToOthersInChannel(client->getClientSocket(), channel[i], ":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " JOIN :" + channel[i] + "\r\n");
			serv->RemoveInviteClient(channel[i], client->getNickname());
		}
	}
}

// PART {#channel} {leave msg for last channel}
//*****채널에서 마지막 한 명이 나갈때 채널 삭제 필요
void Command::part(Client *client)
{// PART {#channel} {leave msg for last channel}
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
	std::string temp;
	int	socket = client->getClientSocket();
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
			if (cmd.size() == 2)
				temp = irc_utils::getForm(client, cmd[0] + " :" + *it + "\r\n");
			else
				temp = irc_utils::getForm(client, cmd[0] + " " + *it + " " + cmd[2] + "\r\n");
			serv->SendMessageToAllClientsInChannel(*it, temp);
			serv->RemoveClientFromChannel(socket, *it);
		}
	}
}

// PRIVMSG (user1,user2,...,usern) <text to be sent>
void Command::privmsg(Client *client)
{
	std::vector<std::string> target; // 귓말 받을 사람 모음

	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "PRIVMSG"));
		return ;
	}

	target = irc_utils::Split(cmd[1], ',');
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
			if (serv->HasChannel(target[i]) && serv->HasClientInChannel(client->getClientSocket(), target[i]))
				serv->SendMessageToOthersInChannel(client->getClientSocket(), target[i], \
					":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " PRIVMSG " + target[i] + " :" + msg + "\r\n");
			else if (serv->HasChannel(target[i]))
				client->PushSendQueue(":irc.local 404 " + client->getNickname() + " " + get_reply_str(ERR_CANNOTSENDTOCHAN, target[i]));
			else
				client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, client->getNickname(), target[i]));
		}
		else {
			if (!serv->HasDuplicateNickname(target[i]))//:irc.local 401 middle 2 :No such nick
				client->PushSendQueue(":irc.local 401 " + client->getNickname() + " " + target[i] + " :No such nick\r\n");
			else {
				serv->SendMessageToOtherClient(client->getClientSocket(), target[i], \
				":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " PRIVMSG " + target[i] + " :" + msg + "\r\n");
			}
		}
	}
}

void Command::list(Client *client)
{ // LIST [<channel>{,<channel>} [<server>]]
	std::string nickname = client->getNickname();
	std::vector<std::string> target;
	client->PushSendQueue(":irc.local 321 " + nickname + " Channel :Users Name\r\n");
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
	// 세번째 인자는 왜 있는지 몰?루
	client->PushSendQueue(":irc.local 323 " + nickname + " :End of channel list.\r\n");
}

void Command::ping(Client *client) // ping을 받은 상황
{								   // PING <server1> [<server2>]
	client->PushSendQueue(":irc.local PONG irc.local :irc.local\r\n");
	// 클라이어트가 서버로 PING 메시지를 보내면, 서버는 PONG 메시지로 응답해 연결이 활성 상태임을 알려줌
}

void Command::quit(Client *client)
{ // QUIT [<Quit message>]
	// 나갈때는 모두 다 보내면 되지 않나
	std::string quit_msg;
	if (cmd.size() == 1)
	{ // QUIT만 침
		// 원본:lower!root@127.0.0.1 QUIT :Quit: leaving

		quit_msg = irc_utils::getForm(client, "QUIT :Quit: leaving\r\n");
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

void Command::kick(Client *client)
{ // KICK <channel> <user> [<comment>]
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
		client->PushSendQueue(":irc.local 401 " + nickname + " " + target + " :No such nick\r\n");
		return;
	}
	else if (serv->HasChannel(channel) == false)
	{ // 채널이 음슴
		// :irc.local 442 upper #2 :You're not on that channel!
		client->PushSendQueue(":irc.local 442 " + nickname + " " + channel + " :You're not on that channel!\r\n");
		return;
	}
	else if (serv->HasClientInChannel(target_socket, channel) == false)
	{// 채널은 존재하는데 그곳에 해당 유저가 음슴
		// :irc.local 441 upper lower #1 :They are not on that channel
		client->PushSendQueue(":irc.local 441 " + nickname + " " + target + " " + channel + " :They are not on that channel\r\n");
		return;
	}
	else if (serv->IsChannelOwner(socket, channel) == false)
	{//권한이 없음
		// :irc.local 482 lower #1 :You must be a channel op or higher to kick a more privileged user.
		client->PushSendQueue(":irc.local 482 "+ nickname + " " + channel + " :You must be a channel op or higher to kick a more privileged user.\r\n");
		return;
	}
	// 해당 채널에서 강퇴했다는 로그 발송
	serv->SendMessageToAllClientsInChannel(channel, irc_utils::getForm(client, private_msg));
	// 해당 채널에서 강퇴
	serv->RemoveClientFromChannel(target_socket, channel);
}

void	Command::invite(Client *client)
{//INVITE <nickname> <channel>
	/*특이사항
		하나만 입력하면 채널을 알아서 넣음. 그때는 클라이언트 닉네임을 넣어야함.
		두개를 입력하면 그대로 들어감. 입력할때 채널은 #을 붙여서 입력해야 제대로 인식함
		세개 이상 입력하면 알아서 인자 두개만 넣어서 보내고 나머지는 버림 => 즉 무조건 인자가 3개로 들어오는게 보장됨
	*/

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
		client->PushSendQueue(":irc.local 403 " + nick_name + " " + channel_name + " :No such channel\r\n");
		return;
	}
	else if (serv->HasDuplicateNickname(invited) == false)
	{//해당 닉네임 없음 = 해당 유저 없음
		//:irc.local 401 middle lowe :No such nick
		client->PushSendQueue(":irc.local 401 " + nick_name + " " + invited + " :No such nick\r\n");
		return;
	}
	else if (serv->HasClientInChannel(serv->getClientSocket(invited), channel_name))
	{//이미 있는 사람 초대했으면??
		//:irc.local 443 upper middle #hi :is already on channel
		client->PushSendQueue(":irc.local 443 " + nick_name + " " + invited + " " + channel_name + " :is already on channel\r\n");
		return;
	}
	else if (serv->IsChannelOwner(client->getClientSocket(), channel_name) == false)
	{//권한이 없음
		//:irc.local 482 middle #hi :You must be a channel op or higher to send an invite.
		client->PushSendQueue(":irc.local 482 " + invited + " " + channel_name + " :You must be a channel op or higher to send an invite.\r\n");
		return;
	}
	//초대하는 코드
	/*
	127.000.000.001.52370-127.000.000.001.06667: INVITE lower #hi

	127.000.000.001.06667-127.000.000.001.52370: :irc.local 341 upper lower :#hi

	127.000.000.001.06667-127.000.000.001.52390: :upper!root@127.0.0.1 INVITE lower :#hi
	*/
		//명령어 발송자에게 날리기
		client->PushSendQueue(":irc.local 341 " + nick_name + " " + invited + " " + channel_name + "\r\n");
		//초대 수신자에게 날리기
		serv->PushSendQueueClient(serv->getClientSocket(invited), irc_utils::getForm(client, private_msg));
		//해당 채널에 초대받은 사람 초대 리스트에 넣기
		serv->AddInviteClient(channel_name, invited);
		// std:: cout << "" << serv->getInvitedClientOfChannel(channel_name) << std::endl;
	/*
	의문점
	초대 성공하면, 받는 사람은 어떻게 초대 받음?
	*/
}

void	Command::topic(Client *client)
{//TOPIC [<topic>]
	std::string nickname = client->getNickname();
	int socket = client->getClientSocket();
	if (cmd.size() == 1)
	{
		//:irc.local 461 upper TOPIC :Not enough parameters.
		client->PushSendQueue(":irc.local 461 " + nickname + " TOPIC :Not enough parameters.\r\n");
		return;
	}
	std::string channel = cmd[1];
	if (serv->HasChannel(channel) == false)
	{//채널이 음슴
		//:irc.local 403 upper hi :No such channel
		client->PushSendQueue(":irc.local 403 " + nickname + " " + channel + " :No such channel\r\n");
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
		client->PushSendQueue(get_reply_number(ERR_CHANOPRIVSNEEDED) + get_reply_str(ERR_CHANOPRIVSNEEDED, client->getNickname(), cmd[1]));
		// serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 482 " + client->getNickname() + " " + cmd[1] + " :You must be a channel op or higher to change the topic.\r\n");
		/*
		문제점
		이러고 다른 채팅방(채팅방이름앞에 공백하나 추가한것)으로 튕겨나간다 왜?
		*/
	}
}

// 없는 옵션 빼려고 할 때, 있는 옵션 등록하려 할 때는 서버에서 반응 x
//  - i: 초대 전용 채널로 설정 및 해제
//  - t: 채널 관리자가 TOPIC 명령어 제한 설정 및 해제 -> TOPIC 명령어를 운영자만 사용할 수 있는지 여부
//  - k: 채널 비밀번호 설정 및 해제
//  - o: 채널 관리자 특권 부여 및 제거
//  - l: 채널에서 유저 제한 설정 및 해제
// MODE <channel> {[+|-]|i|t|k|o|l} [<limit>] [<user>] [<ban mask>]
// 추후 전체적인 코드 리팩토링 필요..
void Command::mode(Client *client)
{
	if (cmd.size() < 2) {
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + get_reply_str(ERR_NEEDMOREPARAMS, "MODE"));
		return ;
	}

	std::string channel = cmd[1];
	std::string str = (":" + client->getNickname() + "!" + client->getRealname() + "127.0.0.1 MODE " + channel + " ");
	std::vector<std::string> args;

	// 사용자가 처음 서버에 진입할때 사용자 모드를 +i로 바꿔줌
	if (cmd[1][0] != '#' && cmd[2] == "+i")
		client->PushSendQueue(":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " MODE " + client->getNickname() + " :+i\r\n");
	else if (!serv->HasChannel(channel))
		client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, client->getNickname(), channel) + "\r\n");
	else if (cmd.size() == 2)
	{
		std::time_t now = std::time(0);
		std::string timestamp = std::to_string(now);
		client->PushSendQueue(":irc.local 324 " + client->getNickname() + " " + channel + " :+nt\r\n");
		client->PushSendQueue(":irc.local 329 " + client->getNickname() + " " + channel + " :" + timestamp + "\r\n"); // 시간 스탬프 값 필요
		serv->SetModeToChannel('t', channel);
	}
	else if (cmd[2][0] == 'b')
		client->PushSendQueue(":irc.local 368 " + client->getNickname() + " " + channel + " :End of channel ban list\r\n");
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
				if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to set channel mode i (inviteonly).\r\n");
				else if (!serv->HasModeInChannel('i', channel)) {
					serv->SetModeToChannel('i', channel);
					options += "i";
				}
				break;
			case 't':
				if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode t (topiclock).\r\n");
				else if (!serv->HasModeInChannel('t', channel)) {
					serv->SetModeToChannel('t', channel);
					options += "t";
				}
				break;
			case 'k':
				if (cmd.size() < idx + 1)
					client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " k * :You must specify a parameter for the key mode. Syntax: <key>.\r\n");
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode k (key).\r\n");
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
					client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " o * :You must specify a parameter for the key mode. Syntax: <nick>.\r\n");
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode o (op).\r\n");
				else if (!serv->HasClientInChannel(cmd[idx], channel)) {
					client->PushSendQueue(":irc.local 401 " + client->getNickname() + " " + cmd[idx] + " :No such nick\r\n");
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
					client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " l * :You must specify a parameter for the key mode. Syntax: <limit>.\r\n");
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to set channel mode l (limit).\r\n");
				else if (!serv->HasModeInChannel('l', channel))
				{
					if (cmd[idx].size() > 9) {
						client->PushSendQueue(":irc.local 696 "+ client->getNickname() + " " + channel + " l " + cmd[idx] + " :Invalid limit mode parameter. Syntax: <limit>.\r\n"); 
						break;
					}
					int limit = std::stoi(cmd[idx]);
					if (limit < 0 || limit > INT_MAX)
						client->PushSendQueue(":irc.local 696 "+ client->getNickname() + " " + channel + " l " + cmd[idx] + " :Invalid limit mode parameter. Syntax: <limit>.\r\n"); 
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
				client->PushSendQueue(":irc.local 472 " + client->getNickname() + " " + mode + " :is not a recognised channel mode.\r\n");
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
			serv->SendMessageToAllClientsInChannel(channel, str + "\r\n");
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
				if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to set channel mode i (inviteonly).\r\n");
				else if (serv->HasModeInChannel('i', channel)) {
					serv->RemoveModeFromChannel('i', channel);
					options += "i";
				}
				break;
			case 't':
				if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode t (topiclock).\r\n");
				else if (serv->HasModeInChannel('t', channel)) {
					serv->RemoveModeFromChannel('t', channel);
					options += "t";
				}
				break;
			case 'k':
				if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode k (key).\r\n");
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
					client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " o * :You must specify a parameter for the key mode. Syntax: <nick>.\r\n");
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode o (op).\r\n");
				else
				{
					serv->RemoveChannelOwner(cmd[idx], channel);
					options += "o";
					args.push_back(cmd[idx]);
					idx++;
				}
				break;
			case 'l':
				if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to set channel mode l (limit).\r\n");
				else if (serv->HasModeInChannel('l', channel))
				{
					serv->RemoveModeFromChannel('l', channel);
					serv->SetUsersLimitInChannel(0, channel);
					options += "l";
				}
				break;
			default: // 해당하는 모드가 없을 때 처리
				client->PushSendQueue(":irc.local 472 " + client->getNickname() + " " + mode + " :is not a recognised channel mode.\r\n");
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
			serv->SendMessageToAllClientsInChannel(channel, str + "\r\n");
	}
}

// 채널 입장 시 join -> mode -> who 순으로 동작
void Command::who(Client *client)
{
	std::string channel = cmd[1];
	std::vector<std::string> args = irc_utils::Split(cmd[2], ',');
	std::string clients_list =  serv->ClientsInChannelList(channel);
	std::vector<std::string> clients_vec = irc_utils::Split(clients_list, ' ');

	if (cmd.size() == 2 && cmd[1][0] == '#') {
		for (size_t i = 0; i < clients_vec.size(); i++) {
			if (clients_vec[i][0] == '@') {
				client->PushSendQueue(":irc.local 354 " + client->getNickname() + " " + cmd[1] + " " + client->getRealname() +\
									  " " + client->getHostname() + " " + clients_vec[i].substr(1) + " H@ 0 0 :" + client->getRealname() + "\r\n");
			}
			else
				client->PushSendQueue(":irc.local 354 " + client->getNickname() + " " + cmd[1] + " " + client->getRealname() + \
									  " " + client->getHostname() + " " + clients_vec[i] + " H 0 0 :" + client->getRealname() + "\r\n");
		}
		client->PushSendQueue(":irc.local 315 " + client->getNickname() + " " + channel + " :End of /WHO list.\r\n");
	}
	else if (args[1] == "743") {
		for (size_t i = 0; i < clients_vec.size(); i++) {
			if (clients_vec[i][0] == '@') {
				client->PushSendQueue(":irc.local 354 " + client->getNickname() + " 743 " + cmd[1] + " " + client->getRealname() +\
									  " " + client->getHostname() + " " + clients_vec[i].substr(1) + " H@ 0 0 :" + client->getRealname() + "\r\n");
			}
			else
				client->PushSendQueue(":irc.local 354 " + client->getNickname() + " 743 " + cmd[1] + " " + client->getRealname() + \
									  " " + client->getHostname() + " " + client->getNickname() + " H 0 0 :" + client->getRealname() + "\r\n");
		}
		client->PushSendQueue(":irc.local 315 " + client->getNickname() + " " + channel + " :End of /WHO list.\r\n");
	}
	else if (args[1] == "745") {
		client->PushSendQueue(":irc.local 354 " + client->getNickname() + " 745 " + cmd[1] + " :0" + "\r\n");
		client->PushSendQueue(":irc.local 315 " + client->getNickname() + " " + channel + " :End of /WHO list.\r\n");
	}
}

void Command::cap(Client *client)
{
	client->PushSendQueue(":irc.local Connecting...\r\n");
}

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
