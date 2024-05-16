#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

Command::Command(Server *server) : serv(server)
{
	cmd_ft["PRIVMSG"] = &Command::privmsg;
	cmd_ft["PASS"] = &Command::pass;
	cmd_ft["NICK"] = &Command::nick;
	cmd_ft["USER"] = &Command::user;
	cmd_ft["JOIN"] = &Command::join;
	cmd_ft["LIST"] = &Command::list;
	cmd_ft["PING"] = &Command::ping;
	cmd_ft["PONG"] = &Command::pong;
	cmd_ft["QUIT"] = &Command::quit;
	cmd_ft["KICK"] = &Command::kick;
	cmd_ft["INVITE"] = &Command::invite;
	cmd_ft["TOPIC"] = &Command::topic;
	cmd_ft["MODE"] = &Command::mode;
	cmd_ft["WHO"] = &Command::who;
	cmd_ft["PART"] = &Command::part;
	cmd_ft["CAP"] = &Command::cap;
	cmd_ft["OPER"] = &Command::oper;
}

Command::~Command() {}

void Command::clean_cmd()
{
	cmd.clear();
}

/**
 * @param clinet 클라이언트 포인터
 * @param str 클라이언트가 입력한 글
 * @returns
 * return true : user typed cmd. //////
 * return false : user typed just chat str
 */
bool Command::excute(Client *client, std::string str)
{
	// set default=============================
	std::string temp("");
	private_msg = str; // 여기 뒤에 \r\n을 빼야 하나?
	bool ret = false;
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
		{
			if (str[i - 1] == '\r')
			{
				std::string temp = str;
				private_msg = temp.substr(0, i);
				it = cmd_ft.find(cmd[0]);
				if (it != cmd_ft.end())
					(this->*(it->second))(client);
				temp.clear();
				cmd.clear();
				private_msg = temp.substr(i);
			}
		}
		else if (str[i] != '\r' && str[i] != '\n')
			temp += str[i];
	}
	if (temp.empty() == false)
		cmd.push_back(temp);
	//==========================================================
	// step 2 : figure it out is this cmd or not
	it = cmd_ft.find(cmd[0]);
	if (it != cmd_ft.end())
		(this->*(it->second))(client);
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
	if (cmd.size() == 1)
	{ // NICK 명령어만 입력했을 경우
		client->PushSendQueue(get_reply_number(ERR_NONICKNAMEGIVEN) + get_reply_str(ERR_NONICKNAMEGIVEN));
		return;
	}
	else
	{
		// 닉네임 중복 여부 판단
		if (serv->HasDuplicateNickname(cmd[1]))
		{ // 닉네임 중복이니 그 사람 한테만 아래를 던져주면 됨
			serv->PushSendQueueClient(client->getClientSocket(), get_reply_number(ERR_NICKNAMEINUSE) + get_reply_str(ERR_NICKNAMEINUSE, cmd[1]));
		}
		else
		{ // 닉네임 중복이 안되었으니 닉네임 변경
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
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + client->getNickname() + " USER " + get_reply_str(ERR_NEEDMOREPARAMS));
		return;
	}
	if (client->getRealname().size() != 0) // 사용자가 USER 명령어를 내렸을 경우 또는 최초 호출인데 뭔가 잡것이 있는 경우
		client->PushSendQueue(get_reply_number(ERR_ALREADYREGISTRED) + client->getNickname() + " " + get_reply_str(ERR_ALREADYREGISTRED));
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

	client->PushSendQueue(":irc.local NOTICE " + client->getNickname() + " :*** Could not resolve your hostname: Request timed out; using your IP address (" + client->getHostname() + ") instead.\r\n");
	if ((serv->CheckPassword("") || client->getPassword()) == false)//서버가 비번이 설정 되어 있으며, 클라이언트가 비번 못 맞춘 경우
	{
		//ERROR :Closing link: (root@192.168.65.2) [Access denied by configuration]
		client->PushSendQueue("ERROR :Closing link: (" + client->getRealname() + "@" + client->getHostname() + ") [Access denied by configuration]\r\n");
		return ;
	}

	client->PushSendQueue(get_reply_number(RPL_WELCOME) + client->getNickname() + get_reply_str(RPL_WELCOME, client->getNickname(), client->getRealname(), client->getHostname()));
	client->PushSendQueue(get_reply_number(RPL_YOURHOST) + client->getNickname() + get_reply_str(RPL_YOURHOST, "irc.local", "ft_irc"));
	client->PushSendQueue(get_reply_number(RPL_CREATED) + client->getNickname() + get_reply_str(RPL_CREATED, serv->getStartedTime()));
	client->PushSendQueue(get_reply_number(RPL_MYINFO) + get_reply_str(RPL_MYINFO, client->getNickname(), "irc.local", "OUR", "FT_IRC"));
	// client->PushSendQueue(":irc.local 005 " + client->getNickname() + " AWAYLEN=200 CASEMAPPING=rfc1459 CHANLIMIT=#:20 CHANMODES=b,k,l,imnpst CHANNELLEN=64 CHANTYPES=# ELIST=CMNTU HOSTLEN=64 KEYLEN=32 KICKLEN=255 LINELEN=512 MAXLIST=b:100 :are supported by this server\r\n");
	// client->PushSendQueue(":irc.local 005 " + client->getNickname() + " MAXTARGETS=20 MODES=20 NAMELEN=128 NETWORK=Localnet NICKLEN=30 PREFIX=(ov)@+ SAFELIST STATUSMSG=@+ TOPICLEN=307 USERLEN=10 USERMODES=,,s,iow WHOX :are supported by this server\r\n");
	client->PushSendQueue(":irc.local 005 " + client->getNickname() + " Enjoy our ft_irc.\r\n");
	/*
	의문점
	1. 안에 들어갈 문장은 대충 복붙했는데 이걸 어찌혀야하나
	2. 사용자가 접속하고 나서 입력하면? -> 지금은 아무일도 안일어남.
	3. 지금 논리상 realname에 :을 입력을 안하고 다른 글자를 넣어도 무사 통과하는데? 괜찮을까?
	*/
}

// ch1 - pw1 짝이 맞아야만 들어갈 수 있음
// 채널 운영자가 여러명일 수 있다
// 비번이 있는 초대 전용 채널일 때 -> 초대를 받고 들어가면 비번 없어도 입장 가능
//						-> 초대를 못 받고 비번만 맞으면 473 error
void Command::join(Client *client)
{ // Join (ch1,ch2,...chn) (pw1,pw2,...,pwn)
	std::vector<std::string> channel, pw;

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
				return;
			}
			else if (serv->HasChannelPassword(channel[i]))
			{
				if (i + 1 > pw.size() || pw[i].empty() || !serv->CheckChannelPassword(pw[i], channel[i]))
				{
					client->PushSendQueue(":irc.local 475 " + client->getNickname() + " " + channel[i] +
										  " :Cannot join channel (incorrect channel key)\r\n");
					return;
				}
			}
			else if (serv->IsOverUsersLimitChannel(channel[i]))
			{
				client->PushSendQueue(":irc.local 471 " + client->getNickname() + " " + channel[i] +
									  " :Cannot join channel (channel is full)\r\n");
				return;
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
		}
	}
}

void Command::part(Client *client)
{// PART {#channel} {leave msg for last channel}
	/*
	/part good bye ->라고 입력하면
	:<nick>!<real>@127.0.0.1 PART #hi :good bye ->  이걸 클라이언트전부에게 뿌림

	log
	127.000.000.001.33312-127.000.000.001.06667: PART #hi :good bye // <= 이건 서버가 받은 메시지

	127.000.000.001.06667-127.000.000.001.33312: :test2!root@127.0.0.1 PART #hi :good bye //<= 이건 같은 방에 있던 클라이언트

	127.000.000.001.06667-127.000.000.001.33310: :test2!root@127.0.0.1 PART #hi :good bye //<= 이건 part친 본인

	현재 결과
	172.017.000.002.58140-192.168.065.002.08080: PART #hi :good bye

	192.168.065.002.08080-172.017.000.002.58140: :klha!root@127.0.0.1 PART #hi :good bye
	===================================
	근데 클라이언트가 안나가지네? 다른 문제가 있나? 여기서 뭘 더 하란 말이지?
	로그는 문제 없으나 다른 문제가 있을 가능성도 있음
	*/
	// std::cout << "you typed \"" << private_msg.size() << "\"."<< std::endl;
	std::string channel = cmd[1];
	std::string nick_name = client->getNickname();
	std::string temp;
	int	socket = client->getClientSocket();
	if (cmd.size() == 2)
		temp = irc_utils::getForm(client, cmd[0] + " :" + channel + "\r\n");
	else
		temp = irc_utils::getForm(client, private_msg);
	// private_msg의 마지막에 \r\n이 있어서 따로 추가 안해도 됨
	if (serv->HasChannel(channel) == false)
	{//그런 채널 없는데? -> 그럼 현재 채널 나가
	/* 현재 1이란 채널에서 /pass get out 사용
	-> 그럼 클라이언트가 /pass #1 :get out으로 던짐 이건 여기에 안들어와짐
	여기에 들어오는 건 status에서 똑같은 명령어를 쓴다면 이렇게 됨
	127.000.000.001.49828-127.000.000.001.06667: PART get :out
	127.000.000.001.06667-127.000.000.001.49828: :irc.local 403 upper get :No such channel
	*/
		client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, nick_name, channel));
		return;
	}
	else if (serv->HasClientInChannel(nick_name, channel) == false)
	{//채널 있기는 한데 넌 안들어가 있는데? -> 오류 메시지만 보내고 끝
	/*
	127.000.000.001.49828-127.000.000.001.06667: PART #4 :hwy?
	127.000.000.001.06667-127.000.000.001.49828: :irc.local 442 upper #4 :You're not on that channel
	*/
		client->PushSendQueue(get_reply_number(ERR_NOTONCHANNEL) + get_reply_str(ERR_NOTONCHANNEL, nick_name, channel));
		return;
	}
	/*
	나간 사람이 마직막 운영자면 -> 해당 채널에 운영자가 없는 채널이 되어 버림....
	나간 사람이 마지막 사람이라면 채널 자체를 없애버려야 한다.
	*/
	serv->SendMessageToAllClientsInChannel(channel, temp);
	serv->RemoveClientFromChannel(socket, channel);
}

// PRIVMSG (user1,user2,...,usern) <text to be sent>
void Command::privmsg(Client *client)
{
	std::vector<std::string> target; // 귓말 받을 사람 모음
	target = irc_utils::Split(cmd[1], ',');
	size_t msg_start = cmd[0].size();
	while (private_msg[msg_start] == ' ')
		msg_start++;
	msg_start += cmd[1].size();
	while (private_msg[msg_start] == ' ')
		msg_start++;
	std::string msg = private_msg.substr(msg_start + 1);
	msg.erase(msg.size() - 2);
	// 보낼 메시지 완성 -> msg
	// target안에 있는 대상이 존재하는지 확인해야함
	// :a!root@127.0.0.1 PRIVMSG #ch1 :hello~
	// :a!root@127.0.0.1 PRIVMSG #ch2 :hello~
	for (size_t i = 0; i < target.size(); i++) {
		if (target[i][0] == '#') {
			if (serv->HasChannel(target[i]) && serv->HasClientInChannel(client->getClientSocket(), target[i]))
				// serv->SendMessageToOthersInChannel(client->getClientSocket(), target[i], irc_utils::getForm(client, private_msg));
				serv->SendMessageToOthersInChannel(client->getClientSocket(), target[i], \
					":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " PRIVMSG " + target[i] + " :" + msg + "\r\n");
			else if (serv->HasChannel(target[i]))
				client->PushSendQueue(":irc.local 404 " + client->getNickname() + " " + get_reply_str(ERR_CANNOTSENDTOCHAN, target[i]));
			else
				client->PushSendQueue(get_reply_number(ERR_NOSUCHCHANNEL) + get_reply_str(ERR_NOSUCHCHANNEL, client->getNickname(), target[i]));
		}
		else {
			if (!serv->HasDuplicateNickname(target[i]))
				client->PushSendQueue(":irc.local 401 " + client->getNickname() + " " +  get_reply_str(ERR_NOSUCHNICK, target[i]));
			else {
				serv->SendMessageToOtherClient(client->getClientSocket(), target[i], \
				":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " PRIVMSG " + target[i] + " :" + msg + "\r\n");
			}
		}
	}
}


void Command::oper(Client *client)
{ // OPER <user> <password>
	std::cout << client->getUsername();
	if (cmd.size() < 3)
	{ // 뭔갈 덜 입력함
	}
	else
	{
		// 발송한 클라이언트가 해당 채널에서 권한이 있는가?
		// 그럼 이 클라이언트가 어느 채널에서 메시지를 보냈는지 어떻게 알지?
		// 해당 유저가 존재하는가?
		// 비번은 뭘 기준으로 해야되나?
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
	// 두번째랑 세번째 인자는 왜 있는지 몰?루
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
	// quit 명령어를 내렸다는 것을 참가했던 모든 채널의 클라이언트에게 본인제외하고 다 보내야함.
	serv->SendMessageToAllJoinedChannel(client->getClientSocket(), quit_msg);
	// 아래는 할 것 다하고 호출!
	serv->RemoveClientFromServer(client->getClientSocket());
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
	std::string nickname = client->getNickname();
	std::string channel = cmd[1];
	std::string target = cmd[2];
	int	socket = client->getClientSocket();
	int target_socket = serv->getClientSocket(target);
	if (serv->HasDuplicateNickname(target) == false)
	{ // 유저 음슴
		//:irc.local 401 upper get :No such nick
		serv->PushSendQueueClient(socket, ":irc.local 401 " + nickname + " " + target + " :No such nick\r\n");
		return;
	}
	else if (serv->HasChannel(channel) == false)
	{ // 채널이 음슴
		// :irc.local 442 upper #2 :You're not on that channel!
		serv->PushSendQueueClient(socket, ":irc.local 442 " + nickname + " " + channel + " :You're not on that channel!\r\n");
		return;
	}
	else if (serv->HasClientInChannel(target_socket, channel) == false)
	{// 채널은 존재하는데 그곳에 해당 유저가 음슴
		// :irc.local 441 upper lower #1 :They are not on that channel
		serv->PushSendQueueClient(socket, ":irc.local 441 " + nickname + " " + target + " " + channel + " :They are not on that channel\r\n");
		return;
	}
	else if (serv->IsChannelOwner(socket, channel) == false)
	{//권한이 없음
		// :irc.local 482 lower #1 :You must be a channel op or higher to kick a more privileged user.
		serv->PushSendQueueClient(socket, ":irc.local 482 "+ nickname + " " + channel + " :You must be a channel op or higher to kick a more privileged user.\r\n");
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
	std::string channel_name = cmd[2];
	std::string nick_name = client->getNickname();
	std::string invited = cmd[1];
	if (serv->HasChannel(cmd[2]) == false)
	{//해당 채널이 없음.
		//":irc.local 403 middle lower :No such channel"
		serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 403 " + nick_name + " " + channel_name + " :No such channel\r\n");
		return;
	}
	else if (serv->HasDuplicateNickname(invited) == false)
	{//해당 닉네임 없음 = 해당 유저 없음
		//:irc.local 401 middle lowe :No such nick
		serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 401 " + nick_name + " " + invited + " :No such nick\r\n");
		return;
	}
	else if (serv->HasClientInChannel(serv->getClientSocket(invited), channel_name))
	{//이미 있는 사람 초대했으면??
		//:irc.local 443 upper middle #hi :is already on channel
		serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 443 " + nick_name + " " + invited + " " + channel_name + " :is already on channel\r\n");
		return;
	}
	else if (serv->IsChannelOwner(client->getClientSocket(), channel_name) == false)
	{//권한이 없음
		//:irc.local 482 middle #hi :You must be a channel op or higher to send an invite.
		serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 482 " + invited + " " + channel_name + " :You must be a channel op or higher to send an invite.\r\n");
		return;
	}
	//초대하는 코드
	/*
	127.000.000.001.52370-127.000.000.001.06667: INVITE lower #hi

	127.000.000.001.06667-127.000.000.001.52370: :irc.local 341 upper lower :#hi

	127.000.000.001.06667-127.000.000.001.52390: :upper!root@127.0.0.1 INVITE lower :#hi
	*/
		//명령어 발송자에게 날리기
		serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 341 " + nick_name + " " + invited + " :" + channel_name + "\r\n");
		//초대 수신자에게 날리기
		serv->PushSendQueueClient(serv->getClientSocket(invited), irc_utils::getForm(client, private_msg));
		//해당 채널에 초대받은 사람 초대 리스트에 넣기
		serv->AddInviteClient(channel_name, invited);
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
		serv->PushSendQueueClient(socket, ":irc.local 461 " + nickname + " TOPIC :Not enough parameters.\r\n");
		return;
	}
	std::string channel = cmd[1];
	if (serv->HasChannel(channel) == false)
	{//채널이 음슴
		//:irc.local 403 upper hi :No such channel
		serv->PushSendQueueClient(socket, ":irc.local 403 " + nickname + " " + channel + " :No such channel\r\n");
	}
	else if (serv->HasModeInChannel('t', channel) == false || serv->IsChannelOwner(socket, channel))
	{//채널에 t 모드가 없거나 권한이 있으니 topic 설정 가능
		serv->SetTopicInChannel(channel, private_msg.substr(8 + channel.size(), private_msg.size() - 2),  client->getRealname() + "@" + client->getHostname());
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
		serv->PushSendQueueClient(client->getClientSocket(), get_reply_number(ERR_CHANOPRIVSNEEDED) + get_reply_str(ERR_CHANOPRIVSNEEDED, client->getNickname(), cmd[1]));
		// serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 482 " + client->getNickname() + " " + cmd[1] + " :You must be a channel op or higher to change the topic.\r\n");
		/*
		문제점
		이러고 다른 채팅방(채팅방이름앞에 공백하나 추가한것)으로 튕겨나간다 왜?
		*/
	}
}

// 없는 옵션 빼려고 할때는 서버에서 반응 x
//  - i: 초대 전용 채널로 설정 및 해제
//  - t: 채널 관리자가 TOPIC 명령어 제한 설정 및 해제 -> TOPIC 명령어를 운영자만 사용할 수 있는지 여부
//  - k: 채널 비밀번호 설정 및 해제
//  - o: 채널 관리자 특권 부여 및 제거
//  - l: 채널에서 유저 제한 설정 및 해제
// MODE <channel> {[+|-]|i|t|k|o|l} [<limit>] [<user>] [<ban mask>]
// 추후 전체적인 코드 리팩토링 필요..
void Command::mode(Client *client)
{
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
				else {
					serv->SetModeToChannel('i', channel);
					options += "i";
				}
				break;
			case 't':
				if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode t (topiclock).\r\n");
				else {
					serv->SetModeToChannel('t', channel);
					options += "t";
				}
				break;
			case 'k':
				if (cmd.size() < idx + 1)
					client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " k * :You must specify a parameter for the key mode. Syntax: <key>.\r\n");
				else if (!serv->IsChannelOwner(client->getClientSocket(), channel))
					client->PushSendQueue(":irc.local 482 " + client->getNickname() + " " + channel + " :You must be a channel op or higher to unset channel mode k (key).\r\n");
				else
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
					serv->SetModeToChannel('o', channel);
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
				else
				{
					serv->SetModeToChannel('l', channel);
					int limit = std::stoi(cmd[idx]);
					serv->SetUsersLimitInChannel(static_cast<size_t>(limit), channel);
					options += "l";
					args.push_back(cmd[idx]);
					idx++;
				}
				break;
			default: // 해당하는 모드가 없을 때 처리
				client->PushSendQueue(":irc.local 472 " + client->getNickname() + " " + mode + " :is not a recognised channel mode.\r\n");
				break;
			}
		}
		// 클라이언트에 보낼 string 완성
		if (idx == 3 || !args.size())
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
				else if (serv->HasModeInChannel('o', channel))
				{
					serv->RemoveModeFromChannel('o', channel);
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
					idx++;
				}
				break;
			default: // 해당하는 모드가 없을 때 처리
				client->PushSendQueue(":irc.local 472 " + client->getNickname() + " " + mode + " :is not a recognised channel mode.\r\n");
				break;
			}
		}
		// 클라이언트에 보낼 string 완성
		if (idx == 3 || !args.size())
			str += ":";
		if (options.length() > 0) 
			str += "-";
		str += options + " ";
		for (size_t i = 3; i < idx; i++)
		{
			if (idx == i + 1 && args.size())
				str += ":";
			if (args.size()) {
				str += (args.front() + " ");
				args.erase(args.begin());
			}
		}
		serv->SendMessageToAllClientsInChannel(channel, str + "\r\n");
	}
}
// :irc.local 354 test 743 #aaa root 127.0.0.1 test H@ 0 0 :root
// :irc.local 315 test #aaa :End of /WHO list.
// 	354, 315만 보내게 해놔서 수정 필요
// 최초 생성때랑 기존에 있는 채널에 들어갈때 각각 보내는 메시지가 다름
void Command::who(Client *client)
{
	std::string channel = cmd[1];
	std::vector<std::string> args = irc_utils::Split(cmd[2], ',');
	std::string clients_list =  serv->ClientsInChannelList(channel);
	std::vector<std::string> clients_vec = irc_utils::Split(clients_list, ' ');

	if (args[1] == "743") {
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

	/*
	WHO #ch %tcuhnfdar,743

	:irc.local 354 c 743 #ch root 127.0.0.1 a H@ 0 0 :root
	:irc.local 354 c 743 #ch root 127.0.0.1 b H 0 0 :root
	:irc.local 354 c 743 #ch root 127.0.0.1 c H 0 0 :root
	:irc.local 315 c #ch :End of /WHO list.
	*/
}

void Command::pong(Client *client)
{ // PONG <server1> [<server2>]
	client->PushSendQueue(":irc.local PONG irc.local :irc.local\r\n");
}
void Command::cap(Client *client)
{
	client->PushSendQueue(":irc.local Connecting...\r\n");
}
