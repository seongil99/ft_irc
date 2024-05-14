#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

Command::Command(Server *server) : serv(server)
{
	cmd_list.push_back("PRIVMSG");
	cmd_ft[0] = &Command::privmsg;
	cmd_list.push_back("PASS");
	cmd_ft[1] = &Command::pass;
	cmd_list.push_back("NICK");
	cmd_ft[2] = &Command::nick;
	cmd_list.push_back("USER");
	cmd_ft[3] = &Command::user;
	cmd_list.push_back("JOIN");
	cmd_ft[4] = &Command::join;
	cmd_list.push_back("OPER");
	cmd_ft[5] = &Command::oper;
	cmd_list.push_back("LIST");
	cmd_ft[6] = &Command::list;
	cmd_list.push_back("PING");
	cmd_ft[7] = &Command::ping;
	cmd_list.push_back("PONG");
	cmd_ft[8] = &Command::pong;
	cmd_list.push_back("QUIT");
	cmd_ft[9] = &Command::quit;
	cmd_list.push_back("KICK");
	cmd_ft[10] = &Command::kick;
	cmd_list.push_back("INVITE");
	cmd_ft[11] = &Command::invite;
	cmd_list.push_back("TOPIC");
	cmd_ft[12] = &Command::topic;
	cmd_list.push_back("MODE");
	cmd_ft[13] = &Command::mode;
	cmd_list.push_back("WHO");
	cmd_ft[14] = &Command::who;
	cmd_list.push_back("PART");
	cmd_ft[15] = &Command::part;
}

Command::~Command() {}

void	Command::clean_cmd()
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
bool	Command::excute(Client *client, std::string str)
{
	//set default=============================
	std::string temp("");
	private_msg = str;//여기 뒤에 \r\n을 빼야 하나?
	bool	ret = false;
	//==========================================================
	//step 1 : split string by ' '
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] == ' ')
		{
			if (temp.empty() == false)
				cmd.push_back(temp);
			temp.clear();
		}
		else if (str[i] != '\r' && str[i] != '\n')
			temp += str[i];
	}
	if (temp.empty() == false)
		cmd.push_back(temp);
	//==========================================================
	//step 2 : figure it out is this cmd or not
	for (size_t i = 0; i < cmd_list.size(); i++)
	{
		if (cmd[0] == cmd_list[i])
		{//this is cmd. execute it.
			(this->*cmd_ft[i])(client);
			ret = true;
			break;
		}
	}
	if (ret)
	{
		std::cout << "for debug : " << cmd[0] << std::endl;
	}
	cmd.clear();
	return ret;
}

void	Command::pass(Client *client)
{//PASS <password>
	if (cmd.size() == 1)
		serv->PushSendQueueClient(client->getClientSocket(), ":irc.local 461 " + get_reply_str(ERR_NEEDMOREPARAMS, "PASS"));
	else {
		if (!serv->CheckPassword(cmd[1])) //패스워드 틀렸을때 어떻게 해야할지 모르겠음 수정필요
			serv->PushSendQueueClient(client->getClientSocket(), ":irc.local Incorrect PASSWORD");
	}
}

void	Command::nick(Client *client)
{//NICK <nickname>
	if (cmd.size() == 1)
	{//NICK 명령어만 입력했을 경우
		client->PushSendQueue(get_reply_number(ERR_NONICKNAMEGIVEN) + get_reply_str(ERR_NONICKNAMEGIVEN));
		return ;
	}
	else
	{
		//닉네임 중복 여부 판단
		if (serv->HasDuplicateNickname(cmd[1]))
		{//닉네임 중복이니 그 사람 한테만 아래를 던져주면 됨
			serv->PushSendQueueClient(client->getClientSocket(), get_reply_number(ERR_NICKNAMEINUSE) + get_reply_str(ERR_NICKNAMEINUSE, cmd[1]));
		}
		else
		{//닉네임 중복이 안되었으니 닉네임 변경
			client->setNickname(cmd[1]);
		}
	}
	// NICK aaa bbb ccc ddd 이런 식으로 여러개 쳤을때는 닉네임이 aaa로 바뀌고 다른 반응 없음
}

void	Command::user(Client *client)
{//USER <username> <hostname> <servername> :<realname>
// 연결이 시작될 때 사용자의 사용자명, 실명 지정에 사용
// 실명 매개변수는 공백 문자를 포함할 수 있도록 마지막 매개변수여야 하며, :을 붙여 인식하도록 함
	if (cmd.size() < 5)
	{//인자를 적게 넣었을 경우
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + client->getNickname() + " USER " + get_reply_str(ERR_NEEDMOREPARAMS));
		return;
	}
	if (client->getRealname().size() != 0)//사용자가 USER 명령어를 내렸을 경우 또는 최초 호출인데 뭔가 잡것이 있는 경우
		client->PushSendQueue(get_reply_number(ERR_ALREADYREGISTRED) + client->getNickname() + " " + get_reply_str(ERR_ALREADYREGISTRED));
	//username 만들기
	std::string	temp = cmd[1];
	if (temp[temp.size() - 1] == '\n')
		temp = temp.substr(0, temp.size());
	client->setUsername(temp);
	temp.clear();
	//realname 만들기
	for (size_t i = 4; i < cmd.size(); i++)
	{
		temp += cmd[i];
		if (i != cmd.size() - 1)
			temp += " ";
	}
	client->setRealname(temp.substr(1, temp.size() - 1));

	client->PushSendQueue(":irc.local NOTICE " + client->getNickname() + " :*** Could not resolve your hostname: Request timed out; using your IP address (127.0.0.1) instead.\r\n");
	client->PushSendQueue(get_reply_number(RPL_WELCOME) + client->getNickname() + get_reply_str(RPL_WELCOME, client->getNickname(), client->getRealname(), "127.0.0.1"));
	client->PushSendQueue(get_reply_number(RPL_YOURHOST) + client->getNickname() + get_reply_str(RPL_YOURHOST, "irc.local", "ft_irc"));
	client->PushSendQueue(get_reply_number(RPL_CREATED) + client->getNickname() + get_reply_str(RPL_CREATED, serv->getStartedTime()));
	client->PushSendQueue(get_reply_number(RPL_MYINFO) + get_reply_str(RPL_MYINFO, client->getNickname(), "irc.local", "OUR", "FT_IRC"));
	client->PushSendQueue(":irc.local 005 " + client->getNickname() + " AWAYLEN=200 CASEMAPPING=rfc1459 CHANLIMIT=#:20 CHANMODES=b,k,l,imnpst CHANNELLEN=64 CHANTYPES=# ELIST=CMNTU HOSTLEN=64 KEYLEN=32 KICKLEN=255 LINELEN=512 MAXLIST=b:100 :are supported by this server\r\n");
	client->PushSendQueue(":irc.local 005 " + client->getNickname() + " MAXTARGETS=20 MODES=20 NAMELEN=128 NETWORK=Localnet NICKLEN=30 PREFIX=(ov)@+ SAFELIST STATUSMSG=@+ TOPICLEN=307 USERLEN=10 USERMODES=,,s,iow WHOX :are supported by this server\r\n");
	/*
	의문점
	1. 안에 들어갈 문장은 대충 복붙했는데 이걸 어찌혀야하나
	2. 사용자가 접속하고 나서 입력하면? -> 지금은 아무일도 안일어남.
	3. 지금 논리상 realname에 :을 입력을 안하고 다른 글자를 넣어도 무사 통과하는데? 괜찮을까?
	*/
}

//ch1 - pw1 짝이 맞아야만 들어갈 수 있음
//채널 운영자가 여러명일 수 있다
//비번이 있는 초대 전용 채널일 때 -> 초대를 받고 들어가면 비번 없어도 입장 가능
//						-> 초대를 못 받고 비번만 맞으면 473 error
void	Command::join(Client *client)
{//Join (ch1,ch2,...chn) (pw1,pw2,...,pwn)
	std::vector<std::string>	channel, pw;

	if (cmd.size() == 2 && cmd[1] == ":")
	{
		client->PushSendQueue(":irc.local 451 * JOIN :You have not registered.\r\n");
		return ;
	}

	channel = irc_utils::Split(cmd[1], ',');
	pw = irc_utils::Split(cmd[2], ',');

	for (size_t i = 0; i < channel.size(); i++) {
		//채널이 없으면 채널을 만든다
		if (!serv->HasChannel(channel[i]))
			serv->CreateChannel(channel[i]);
		else { //채널이 있는 상태 -> 권한, 비밀번호 확인
			if (!serv->IsInvitedChannel(client->getClientSocket(), channel[i])) {
				client->PushSendQueue(":irc.local 473 " + client->getNickname() + " " + channel[i] + " :Cannot join channel (invite only)\r\n");
				return ;
			}
			else if (serv->HasChannelPassword(channel[i])) {
				if (i + 1 > pw.size() || pw[i].empty() || !serv->CheckChannelPassword(pw[i], channel[i])) {
					client->PushSendQueue(":irc.local 475 " + client->getNickname() + " " + channel[i] + \
									  	  " :Cannot join channel (incorrect channel key)\r\n");
					return ;
				}
			}
			else if (serv->IsOverUsersLimitChannel(channel[i])) {
					client->PushSendQueue(":irc.local 471 " + client->getNickname() + " " + channel[i] + \
										  " :Cannot join channel (+l)\r\n");
					return ;
			}
		}
		if (serv->HowManyChannelsJoined(client->getClientSocket()) >= 10) //채널 10개 이상 속했을 때
			client->PushSendQueue("irc.local 405 " + client->getNickname() + " " + get_reply_str(ERR_TOOMANYCHANNELS, channel[i]) + "\r\n");
		else {
			serv->AddClientToChannel(*client, channel[i]);
			client->PushSendQueue(client->getNickname() + "!" + client->getRealname() + \
								"@127.0.0.1 JOIN : " + channel[i] + "\r\n");
			client->PushSendQueue(":irc.local 353 " + client->getNickname() + " = " + \
								channel[i] + " :" + serv->ClientsInChannelList(channel[i]) + "\r\n");
			client->PushSendQueue(":irc.local 366 " + client->getNickname() + " " + channel[i] + " :End of /NAMES list.\r\n");
		}
	}
}

void	Command::part(Client *client)// 이게 아니던...데?
{//PART <channel>{,<channel>}
	/*
	/part hi ->라고 입력하면
	:<nick>!<real>@127.0.0.1 PART #hi :hi ->  이걸 클라이언트전부에게 뿌림
	*/
	std::string temp(":");
	std::cout << "you typed " << private_msg << std::endl;
	temp += client->getNickname() + "!" + client->getRealname() + "@127.0.0.1 " + private_msg;
	std::string channel = client->getLastJoinedChannelName();
	serv->SendMessageToAllClientsInChannel(channel, temp);
	serv->RemoveClientFromChannel(client->getClientSocket(), channel);
}

void	Command::privmsg(Client *client)
{//PRIVMSG (user1,user2,...,usern) <text to be sent>
	std::vector<std::string>	target;//귓말 받을 사람 모음
	if (cmd.size() == 1)
	{
		//PRIVMSG만 입력하면?
		std::cout << client->getUsername();
	}
	else if (cmd.size() == 2)
	{
		//무언갈 덜 입력함
	}
	else
	{
		if (cmd[1].find(",") == std::string::npos)
			target.push_back(cmd[1]);//한명만 입력함
		else//,가 있음 두명 이상 입력했을 가능성
		{
			std::string	temp("");
			for (int i = 0; cmd[1][i]; i++)
			{
				if (cmd[1][i] == ',')
				{
					if (temp.empty() == false)
						target.push_back(temp);
					temp.clear();
				}
				else
					temp += cmd[1][i];
			}
			if (temp.empty() == false)
				target.push_back(temp);
		}
		size_t msg_start = cmd[0].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		msg_start += cmd[1].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		std::string msg = private_msg.substr(msg_start);//끝에 \r\n가 남았는데 괜찮을까?
		//보낼 메시지 완성 -> msg
		//target안에 있는 대상이 존재하는지 확인해야함
		if (target.size() == 1)
		{
			std::cout << "target is 1. that is " << target[0] << std::endl;
			if (target[0][0] == '#' && serv->HasChannel(target[0].substr(1)))//받는게 사람이 아니라 채널...흠
				serv->SendMessageToAllClientsInChannel(target[0].substr(1), msg.substr(1, msg.size()));
			// else if ()
		}
	}
}

void	Command::oper(Client *client)
{//OPER <user> <password>
	std::cout << client->getUsername();
	if (cmd.size() < 3)
	{//뭔갈 덜 입력함

	}
	else
	{
		//발송한 클라이언트가 해당 채널에서 권한이 있는가?
		//그럼 이 클라이언트가 어느 채널에서 메시지를 보냈는지 어떻게 알지?
		//해당 유저가 존재하는가?
		//비번은 뭘 기준으로 해야되나?
	}
}

void	Command::list(Client *client)
{//LIST [<channel>{,<channel>} [<server>]]
	std::vector<std::string>	target;
	if (cmd.size() == 1)
	{// 하나만 입력하면 사용 가능한 모든 채널 열람
		std::string ret = serv->getAllChannelName();
		if (ret.size() == 0)//채널이 없음
			ret = "There is no channel";// 채널이 없다는 메시지
		serv->PushSendQueueClient(client->getClientSocket(), ret + "\r\n");
	}
	else
	{
		if (cmd[1].find(",") == std::string::npos)
			target.push_back(cmd[1]);//하나만 입력함
		else//,가 있음 두개 이상 입력했을 가능성
		{
			std::string	temp("");
			for (int i = 0; cmd[1][i]; i++)
			{
				if (cmd[1][i] == ',')
				{
					if (temp.empty() == false)
						target.push_back(temp);
					temp.clear();
				}
				else
					temp += cmd[1][i];
			}
			if (temp.empty() == false)
				target.push_back(temp);
		}
	}
// 두번째랑 세번째 인자는 왜 있는지 몰?루
}

void	Command::ping(Client *client) //ping을 받은 상황
{//PING <server1> [<server2>]
	client->PushSendQueue(":irc.local PONG irc.local :irc.local\r\n");
//클라이어트가 서버로 PING 메시지를 보내면, 서버는 PONG 메시지로 응답해 연결이 활성 상태임을 알려줌
}

void	Command::quit(Client *client)
{//QUIT [<Quit message>]
	//근데 이거 유저가 치면 본인이 나간다는 말 아닌가?
	//나갈때는 모두 다 보내면 되지 않나
	if (cmd.size() == 1)
	{//QUIT만 침
		//해당 유저가 나간다는 메시지

		//아래는 할 것 다하고 호출!
		serv->RemoveClientFromServer(client->getClientSocket());
	}
	else
	{//내보낼때 메시지도 같이 침
		std::string msg = private_msg.substr(5);
		if (msg[0] == ' ')
		{
			int space = 0;
			while (msg[space] == ' ')
				space++;
			msg = msg.substr(space);
		}
		//보낼 메시지 완성 -> msg
	}
}

void	Command::kick(Client *client)
{//KICK <channel> <user> [<comment>]
	//물론 채널, 해당 유저의 권한, 대상 유저가 존재하는지 확인
	//권한이 있는가?
	std::string msg(""/*강퇴 기본 메시지*/);
	if (cmd.size() < 3)
	{//채널 or 유저를 안침 물론 둘다 안쳤을수도

	}
	else if (cmd.size() > 3)
	{//마지막에 강퇴 메시지를 넣었음
		size_t msg_start = cmd[0].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		msg_start += cmd[1].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		msg = private_msg.substr(msg_start);
		//보낼 메시지 완성 -> msg
	}
	//채널이 존재 하는가
	if (serv->HasChannel(cmd[1]) == false)
	{//채널 음슴
		return;
	}//유저가 존재하는가
	else if (serv->HasDuplicateNickname(cmd[2]) == false)
	{//유저 음슴
		return;
	}
	// Channel	*channel = serv->getChannel(cmd[1]);
	//유저가 해당 채널에 존재 하는가?
	if (serv->HasClientInChannel(client->getClientSocket(), cmd[1]) == false)
	{
		//채널에 해당 유저가 음슴
		return;
	}
	Client	*kicked_client = client;
	(void)kicked_client;

	/*
	강퇴당했다는 메시지를 보내야함
	당연히 명령어 친 사람과 강퇴 당한 사람에게 보내야겠지? 그보다 해당 채널에 있는 모든 사람에게 보내면 더 좋지 않나?

	강퇴 메시지 가공
	*/
	// channel->SendMessageToAllClients(msg);
	serv->SendMessageToAllClientsInChannel(cmd[1], msg);
	//해당 채널에서 강퇴
	// channel->RemoveClient(kicked_client->getClientSocket());
	serv->RemoveClientFromChannel(client->getClientSocket(), cmd[1]);
}

void	Command::invite(Client *client)
{//INVITE <nickname> <channel>
	std::cout << client->getUsername();//컴파일 에러 방지용. 나중에 꼭 지울것!!
	if (cmd.size() != 3)
	{// 무언갈 더 쳤거나 덜 쳣음

	}
	//물론 채널, 권한, 대상 유저가 존재하는지 확인
	if (serv->HasDuplicateNickname(cmd[1]) == false)
	{//해당 닉네임 없음 = 해당 유저 없음

	}
	else if (serv->HasChannel(cmd[2]) == false)
	{//해당 채널이 없음.

	}
	//이미 있는 사람 초대했으면??
	//초대하는 코드
}

void	Command::topic(Client *client)
{//TOPIC <channel> [<topic>]
	std::cout << client->getUsername();//컴파일 에러 방지용. 나중에 꼭 지울것!!
	if (cmd.size() == 1)
	{//TOPIC만 입력하면?
		return;
	}
	// 채널 존재하는지 확인
	if (serv->HasChannel(cmd[1]) == false)
	{//채널 음슴
		return;
	}
	Channel *channel = serv->getChannel(cmd[2]);
	(void)channel;
	if (cmd.size() == 2)//해당 채널의 토픽을 확인하는 명령어
	{

	}
	else if (cmd.size() == 3)//해당 채널의 토픽을 설정하는 명령어
	{

	}
	else
	{// 그 이외로 무언가를 더 치면?
		return;
	}
}

//없는 옵션 빼려고 할때는 서버에서 반응 x
// - i: 초대 전용 채널로 설정 및 해제
// - t: 채널 관리자가 TOPIC 명령어 제한 설정 및 해제 -> TOPIC 명령어를 운영자만 사용할 수 있는지 여부
// - k: 채널 비밀번호 설정 및 해제
// - o: 채널 관리자 특권 부여 및 제거
// - l: 채널에서 유저 제한 설정 및 해제
//MODE <channel> {[+|-]|i|t|k|o|l} [<limit>] [<user>] [<ban mask>]
//코드가 너무 더럽다..
void	Command::mode(Client *client)
{
	std::string channel = cmd[1];
	std::string info = " ";
	
	//사용자가 처음 서버에 진입할때 사용자 모드를 +i로 바꿔줌
	if (cmd[1][0] != '#' && cmd[2] == "+i") 
		client->PushSendQueue(":" + client->getNickname() + "!" + client->getRealname() + "@127.0.0.1 MODE " + client->getNickname() + " :+i\r\n");
	else if (cmd.size() == 2) {
		client->PushSendQueue(":irc.local 324 " + client->getNickname() + " " + channel + " :+nt\r\n");
		client->PushSendQueue(":irc.local 329 " + client->getNickname() + " " + channel + "\r\n"); //시간 스탬프 값 필요
	}
	else if (cmd[2][0] == 'b')
		client->PushSendQueue(":irc.local 368 " + client->getNickname() + " " + channel + " :End of channel ban list\r\n");
	else if (cmd[2][0] == '+') {
		std::string str = (":" + client->getNickname() + "!" + client->getRealname() + "127.0.0.1 MODE " + channel + " ");
		size_t idx = 3;
		for (size_t i = 1; i < cmd[2].size(); i++) {
			char mode = cmd[2][i];
			switch (mode) {
				case 'i':
					serv->SetModeToChannel('i', channel);
					break;
				case 't':
					serv->SetModeToChannel('t', channel);
					break;
				case 'k':
					if (cmd.size() < idx + 1) {
						client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " k * :You must specify a parameter for the key mode. Syntax: <key>.\r\n");
					} else {
						serv->SetModeToChannel('k', channel);
						serv->SetPasswordInChannel(cmd[idx], channel);
						idx++;
					}
					break;
				case 'o':
					if (cmd.size() < idx + 1) {
						client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " o * :You must specify a parameter for the key mode. Syntax: <nick>.\r\n");
					} else {
						serv->SetModeToChannel('o', channel);
						serv->AddChannelOwner(cmd[idx], channel);
						idx++;
					}
					break;
				case 'l':
					if (cmd.size() < idx + 1) {
						client->PushSendQueue(":irc.local 696 " + client->getNickname() + " " + channel + " l * :You must specify a parameter for the key mode. Syntax: <limit>.\r\n");
					} else {
						serv->SetModeToChannel('l', channel);
						int limit = std::stoi(cmd[idx]);
						serv->SetUsersLimitInChannel(static_cast<size_t>(limit), channel);
						idx++;
					}
					break;
				default: // 해당하는 모드가 없을 때 처리
					break;
			}
		}
		//클라이언트에 보낼 string 완성
		if (idx == 3)
			str += ":";
		str += cmd[2];
		for (size_t i = 3; i < idx; i++) {
			if (idx == i + 1)
				info += ":";
			info += (cmd[i] + " ");
		}
		serv->SendMessageToAllClientsInChannel(channel, str + info + "\r\n");
	}
	else if (cmd[2][0] == '-') {
		std::string str = (":" + client->getNickname() + "!" + client->getRealname() + "127.0.0.1 MODE " + channel + " ");
		int idx = 3;
		for (size_t i = 1; i < cmd[2].size(); i++) {
			char mode = cmd[2][i];
			switch (mode) {
				case 'i':
					if (serv->HasModeInChannel('i', channel))
						serv->RemoveModeFromChannel('i', channel);
					break;
				case 't':
					if (serv->HasModeInChannel('t', channel))
						serv->RemoveModeFromChannel('t', channel);
					break;
				case 'k':
					if (serv->HasModeInChannel('k', channel)) {
						serv->RemoveModeFromChannel('k', channel);
						serv->SetPasswordInChannel("", channel);
						idx++;
					}
					break;
				case 'o':
					if (serv->HasModeInChannel('o', channel)) {
						serv->RemoveModeFromChannel('o', channel);
						serv->RemoveChannelOwner(cmd[idx], channel);
						idx++;
					}
					break;
				case 'l':
					if (serv->HasModeInChannel('l', channel)) {
						serv->RemoveModeFromChannel('l', channel);
						serv->SetUsersLimitInChannel(0, channel);
						idx++;
					}
					break;
				default: // 해당하는 모드가 없을 때 처리
					break;
			}
		}
		//클라이언트에 보낼 string 완성
		if (idx == 3)
			str += ":";
		str += cmd[2];
		for (size_t i = 3; i < idx; i++) {
			if (idx == i + 1)
				info += ":";
			info += (cmd[i] + " ");
		}
		serv->SendMessageToAllClientsInChannel(channel, str + info + "\r\n");
	}
}
// :irc.local 354 test 743 #aaa root 127.0.0.1 test H@ 0 0 :root
// :irc.local 315 test #aaa :End of /WHO list.
// 	354, 315만 보내게 해놔서 수정 필요
void	Command::who(Client *client)
{
	std::string channel = cmd[1];
	client->PushSendQueue(":irc.local 354 " + client->getNickname() + "\r\n");
	client->PushSendQueue(":irc.local 315 " + client->getNickname() + " " + channel + " :End of /WHO list.\r\n");
}

void	Command::pong(Client *client)
{//PONG <server1> [<server2>]
	client->PushSendQueue(" :irc.local PONG irc.local :irc.local\r\n");
}
