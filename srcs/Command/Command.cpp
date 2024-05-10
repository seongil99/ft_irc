#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

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
		else
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
			std::cout << "cmd" << std::endl;
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
	std::cout << client->getUsername();
	if (cmd.size() == 1)
	{//pass 하나만 치면?

	}
	else if (cmd.size() == 2)
	{

	}
	else
	{//무언가를 더 침

	}
}

void	Command::nick(Client *client)
{//NICK <nickname>
	std::cout << client->getUsername();
	if (cmd.size() == 1)
	{//NICK 명령어만 입력했을 경우

		return ;
	}
	else if (cmd.size() == 2)
	{
		//닉네임 중복 여부 판단
		if (serv->IsNicknameExists(cmd[1]))
		{//닉네임 중복이니 그 사람 한테만 아래를 던져주면 됨
			serv->PushSendQueueClient(client->getClientSocket(), get_reply_number(ERR_NICKNAMEINUSE) + get_reply_str(ERR_NICKNAMEINUSE, cmd[1]));
		}
		else
		{//닉네임 중복이 안되었으니 닉네임 변경

		}
	}
	else//NICK 명령어 인자를 여러개 침
	{//NICK (str1) (str2)... 이경우엔?

	}
}

void	Command::user(Client *client)
{//USER <username> * * :<realname>
// 연결이 시작될 때 사용자의 사용자명, 실명 지정에 사용
// 실명 매개변수는 공백 문자를 포함할 수 있도록 마지막 매개변수여야 하며, :을 붙여 인식하도록 함
// 중간의 인자 두개는 안쓴다는데 왜 있는거지?
	std::cout << client->getUsername();
}

void	Command::join(Client *client)
{//Join (ch1,ch2,...chn) (pw1,pw2,...,pwn)
	//물론 채널이 존재하는지, 비번이 있는지 맞는지도 확인
	std::cout << client->getUsername();
	std::vector<std::string>	channel, pw;
	std::string	temp("");
	switch (cmd.size())
	{
	case 1://JOIN만 입력하면?
		break;
	case 2://JOIN 채널만 입력
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
		//채널이 존재하는지 확인하는 작업이 필요함
		//해당 채널이 비번이 있었으면?
		break;
	case 3://잘 입력한 경우
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
		temp.clear();
		if (cmd[2].find(",") == std::string::npos)
			pw.push_back(cmd[2]);//하나만 입력함
		else//,가 있음 두개 이상 입력했을 가능성
		{
			for (int i = 0; cmd[2][i]; i++)
			{
				if (cmd[2][i] == ',')
				{
					if (temp.empty() == false)
						pw.push_back(temp);
					temp.clear();
				}
				else
					temp += cmd[2][i];
			}
			if (temp.empty() == false)
				pw.push_back(temp);
		}
		if (pw.size() != channel.size())
		{//채널 입력 개수랑 비번 입력 개수가 다르면?
		//비번 없는 채널과 있는 채널 스까서 입력했으면?

		}
		//channel안에 있는 채널이 존재하는지 확인해야함
		//그에 대비하는 비번도 맞는지 확인해야함.
		break;
	default:// 그 이외로 무언가를 더 치면?
		break;
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
			if (serv->IsChannelExists(*it) == false)
			{// 없는 채널 입력함
				//없는 채널 입력 했다는 메시지
				continue;
			}
			Channel	*joined_channel = serv->getChannel(*it);
			if (joined_channel->DidJoinClient(client->getClientSocket()) == false)
			{//채널은 있는데 참가안한 채널을 입력함
				//참가안한 채널이라는 메시지
			}
			else
			{//있는 채널에 참가해 있음
				joined_channel->RemoveClient(client->getClientSocket());
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

void	Command::ping(Client *client)
{//PING <server1> [<server2>]
	std::cout << client->getUsername();
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
	if (serv->IsChannelExists(cmd[1]) == false)
	{//채널 음슴
		return;
	}//유저가 존재하는가
	else if (serv->IsNicknameExists(cmd[2]) == false)
	{//유저 음슴
		return;
	}
	Channel	*channel = serv->getChannel(cmd[1]);
	//유저가 해당 채널에 존재 하는가?
	if (channel->DidJoinClient(cmd[2]) == false)
	{
		//채널에 해당 유저가 음슴
		return;
	}
	Client	*kicked_client = channel->getJoinedClient(cmd[2]);

	/*
	강퇴당했다는 메시지를 보내야함
	당연히 명령어 친 사람과 강퇴 당한 사람에게 보내야겠지? 그보다 해당 채널에 있는 모든 사람에게 보내면 더 좋지 않나?
	
	강퇴 메시지 가공
	*/
	channel->SendMessageToAllClients(msg);
	//해당 채널에서 강퇴
	channel->RemoveClient(kicked_client->getClientSocket());
}

void	Command::invite(Client *client)
{//INVITE <nickname> <channel>	
	std::cout << client->getUsername();
	if (cmd.size() != 3)
	{// 무언갈 더 쳤거나 덜 쳣음

	}
	//물론 채널, 권한, 대상 유저가 존재하는지 확인
	if (serv->IsNicknameExists(cmd[1]) == false)
	{//해당 닉네임 없음 = 해당 유저 없음
		
	}
	else if (serv->IsChannelExists(cmd[2]) == false)
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
	if (serv->IsChannelExists(cmd[1]) == false)
	{//채널 음슴
		return;
	}
	Channel *channel = serv->getChannel(cmd[2]);
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
