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
	cmd_list.push_back("NOTICE");
	cmd_ft[14] = &Command::notice;
}

Command::~Command() {}

void	Command::clean_cmd()
{
	cmd.clear();
}

//return = true : user typed cmd
//return = false : user typed just chat str
//USER asdf
bool	Command::excute(Client *client, std::string str)
{
	//set default=============================
	std::string temp("");
	private_msg = str;
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
	// if (ret)
	// {
		//명령어라면 무엇을 더 하면 될까?
	// }
	cmd.clear();
	return ret;
}

void	Command::pass(Client *client)
{//PASS <password>
	// std::cout << client->getUsername();
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
		client->PushSendQueue(":irc.local 431 " + get_reply_str(ERR_NONICKNAMEGIVEN));
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
{//USER <username> * * :<realname>
// 연결이 시작될 때 사용자의 사용자명, 실명 지정에 사용
// 실명 매개변수는 공백 문자를 포함할 수 있도록 마지막 매개변수여야 하며, :을 붙여 인식하도록 함
// 중간의 인자 두개는 안쓴다는데 왜 있는거지?
	if (cmd.size() < 5)
		client->PushSendQueue(get_reply_number(ERR_NEEDMOREPARAMS) + client->getNickname() + " USER " + get_reply_str(ERR_NEEDMOREPARAMS));
	if (client->getRealname().size() != 0)
		client->PushSendQueue(":irc.local 462 " + client->getNickname() + " " + get_reply_str(ERR_ALREADYREGISTRED));
	client->setUsername(cmd[1]);
	cmd[4].erase(0); //":" 제거
	client->setRealname(cmd[4]);
	
	//시간표현하는부분 gpt코드 가져다 쓴거라 과제 규칙에 맞게 수정필요
	std::time_t now = std::time(nullptr);
	std::tm* localTime = std::localtime(&now);
	char buffer[80];
    std::strftime(buffer, 80, "%H:%M:%S %b %d %Y", localTime);

	//001~005 메세지 나오게 하기 -> 대강넣은거라 수정필요 tcpflow에서 뭔가 이상하게 나옴..
	client->PushSendQueue(":irc.local NOTICE " + client->getNickname() + ":*** Could not resolve your hostname: Request timed out; using your IP address (127.0.0.1) instead.\n" + \
		":irc.local 001 " + client->getNickname() + " :Welcome to the Localnet IRC Network " + client->getNickname() + "!" + client->getRealname() + "@127.0.0.1\n" + \
		":irc.local 002 " + client->getNickname() + " :Your host is irc.local, running version ft_irc\n" + \
		":irc.local 003 " + client->getNickname() + " :This server was created " + buffer + "\n" + \
		":irc.local 004 " + client->getNickname() + " irc.local OUR FT_IRC\n" + \
		":irc.local 005 " + client->getNickname() + " MAXTARGETS=20 MODES=20 NAMELEN=128 NETWORK=Localnet NICKLEN=30 PREFIX=(ov)@+ SAFELIST STATUSMSG=@+ TOPICLEN=307 USERLEN=10 USERMODES=,,s,iow WHOX :are supported by this server\r\n"); //서버에서 지원하는 기능
	// :irc.local NOTICE seonyoon :*** Could not resolve your hostname: Request timed out; using your IP address (127.0.0.1) instead.
	// :irc.local 001 seonyoon :Welcome to the Localnet IRC Network seonyoon!root@127.0.0.1
	// :irc.local 002 seonyoon :Your host is irc.local, running version InspIRCd-3
	// :irc.local 003 seonyoon :This server was created 09:25:18 May 09 2024
	// :irc.local 004 seonyoon irc.local InspIRCd-3 iosw biklmnopstv :bklov
	// :irc.local 005 seonyoon AWAYLEN=200 CASEMAPPING=rfc1459 CHANLIMIT=#:20 CHANMODES=b,k,l,imnpst CHANNELLEN=64 CHANTYPES=# ELIST=CMNTU HOSTLEN=64 KEYLEN=32 KICKLEN=255 LINELEN=512 MAXLIST=b:100 :are supported by this server
	// :irc.local 005 seonyoon MAXTARGETS=20 MODES=20 NAMELEN=128 NETWORK=Localnet NICKLEN=30 PREFIX=(ov)@+ SAFELIST STATUSMSG=@+ TOPICLEN=307 USERLEN=10 USERMODES=,,s,iow WHOX :are supported by this server
	// :irc.local 251 seonyoon :There are 0 users and 0 invisible on 1 servers
}

// - i: 초대 전용 채널로 설정 및 해제
// - t: 채널 관리자가 TOPIC 명령어 제한 설정 및 해제 -> TOPIC 명령어를 운영자만 사용할 수 있는지 여부
// - k: 채널 비밀번호 설정 및 해제
// - o: 채널 관리자 특권 부여 및 제거
// - l: 채널에서 유저 제한 설정 및 해제
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
			if (!serv->IsInvitedChannel(client->getClientSocket(), channel[i]))
				client->PushSendQueue(":irc.local 473 " + client->getNickname() + " " + channel[i] + \
									  " :Cannot join channel (invite only)\r\n");
			if (serv->HasChannelPassword(channel[i])) {
				if (i + 1 > pw.size() || pw[i].empty() || !serv->CheckChannelPassword(pw[i], channel[i]))
					client->PushSendQueue(":irc.local 475 " + client->getNickname() + " " + channel[i] + \
									  " :Cannot join channel (incorrect channel key)");
			}
		}
	}
}

void	Command::part(Client *client)
{//PART <channel>{,<channel>}
	std::cout << client->getUsername();
	std::vector<std::string>	channel;
	std::string	temp("");
	if (cmd.size() == 1)
	{//PART 하나만 입력함

	}
	else if (cmd.size() == 2)
	{
		if (cmd[1].find(",") == std::string::npos)
			channel.push_back(cmd[1]);//하나만 입력함
		else//,가 있음 두개 이상 입력했을 가능성
		{
			for (int i = 0; cmd[1][i]; i++)
			{
				if (cmd[1][i] == ',')
				{
					if (temp.empty() == false)
						channel.push_back(temp);
				temp.clear();
				}
				else
					temp += cmd[1][i];
			}
			if (temp.empty() == false)
				channel.push_back(temp);
		}
		//물론 채널이 존재하는지, 들어가 있었는지 확인
		//근데 하나는 그렇다 치더라도 2개 이상 입력했으면 어떻게 처리하지?
		for (std::vector<std::string>::iterator it = channel.begin(); it != channel.end(); it++)
		{
			if (serv->HasChannel(*it) == false)
			{// 없는 채널 입력함
				//없는 채널 입력 했다는 메시지
				continue;
			}
			// Channel	*joined_channel = serv->getChannel(*it);
			if (serv->HasClientInChannel(client->getClientSocket(), (*it)) == false)
			{//채널은 있는데 참가안한 채널을 입력함
				//참가안한 채널이라는 메시지
			}
			else
			{//있는 채널에 참가해 있음
				serv->RemoveClientFromChannel(client->getClientSocket(), (*it));
				//해당 채널에서 나갔다는 메시지
			}
		}
	}
	else// 무언갈 더 입력함
	{

	}
}

void	Command::privmsg(Client *client)
{//PRIVMSG (user1,user2,...,usern) <text to be sent>
	std::cout << client->getUsername();
	std::vector<std::string>	target;//귓말 받을 사람 모음
	if (cmd.size() == 1)
	{
		//PRIVMSG만 입력하면?
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
		//target안에 있는 대상이 존재하는지 확인해야함
		size_t msg_start = cmd[0].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		msg_start += cmd[1].size();
		while (private_msg[msg_start] == ' ')
			msg_start++;
		std::string msg = private_msg.substr(msg_start);
		//보낼 메시지 완성 -> msg
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
	// std::cout << client->getUsername();
	std::vector<std::string>	target;
	if (cmd.size() == 1)
	{// 하나만 입력하면 사용 가능한 모든 채널 열람
		std::string ret = serv->getAllChannelName();
		if (ret.size() == 0)//채널이 없음
			ret = "There is no channel";// 채널이 없다는 메시지
		serv->PushSendQueueClient(client->getClientSocket(), ret);
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
	std::cout << client->getUsername();
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
	std::cout << client->getUsername();
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
	std::cout << client->getUsername();
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
	std::cout << client->getUsername();
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

void	Command::mode(Client *client)
{//MODE <channel> {[+|-]|i|t|k|o|l} [<limit>] [<user>] [<ban mask>]
	std::cout << client->getUsername();
	//물론 채널, 권한이 존재하는지 확인
	if (cmd[1] == "i")
	{

	}
	else if (cmd[1] == "t")
	{

	}
	else if (cmd[1] == "k")
	{

	}
	else if (cmd[1] == "o")
	{

	}
	else if (cmd[1] == "l")
	{

	}
	else//플래그를 무조건 넣어야하나?
	{//그 이외의 플래그?

	}
}

void	Command::notice(Client *client)
{//이게 뭐하는 명령어인지 몰?루
	std::cout << client->getUsername();
}

void	Command::pong(Client *client)
{//PONG <server1> [<server2>]
	std::cout << client->getUsername();
}
