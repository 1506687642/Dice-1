/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123���
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <thread>
#include <chrono>
#include <mutex>

#include "APPINFO.h"
#include "RandomGenerator.h"
#include "RD.h"
#include "CQEVE_ALL.h"
#include "InitList.h"
#include "GlobalVar.h"
#include "NameStorage.h"
#include "GetRule.h"
#include "DiceMsgSend.h"
#include "CustomMsg.h"
#include "NameGenerator.h"
#include "MsgFormat.h"
#include "DiceNetwork.h"
#include "CardDeck.h"
#include "DiceConsole.h"
#include "EncodingConvert.h"
/*
TODO:
1. en�ɱ�ɳ��춨
2. st�����￨
3. st���￨��
4. st����չʾ��ȫ����չʾ�Լ�����
5. ����rules�������ݿ�
6. jrrp�Ż�
7. help�Ż�
8. ȫ���ǳ�
*/

using namespace std;
using namespace CQ;

unique_ptr<NameStorage> Name;

std::string strip(std::string origin)
{
	bool flag = true;
	while (flag)
	{
		flag = false;
		if (origin[0] == '!' || origin[0] == '.')
		{
			origin.erase(origin.begin());
			flag = true;
		}
		else if (origin.substr(0, 2) == "��" || origin.substr(0, 2) == "��")
		{
			origin.erase(origin.begin());
			origin.erase(origin.begin());
			flag = true;
		}
	}
	return origin;
}

std::string getName(long long QQ, long long GroupID = 0)
{
	//if (GroupID)
	{
		return strip(Name->get(GroupID, QQ).empty()
					? (Name->get(0, QQ).empty()
							? (getGroupMemberInfo(GroupID, QQ).GroupNick.empty()
									? getStrangerInfo(QQ).nick
									: getGroupMemberInfo(GroupID, QQ).GroupNick)
							: Name->get(0, QQ))
					: Name->get(GroupID, QQ));
	}
	/*˽��*/
	//return strip(getStrangerInfo(QQ).nick);
}


//Masterģʽ
bool boolMasterMode = false;
//����ģʽ
bool boolStandByMe = false;
//�����������ʺ�
long long IdentityQQ = 0;
long long StandQQ = 0;
map<long long, int> DefaultDice;
map<long long, string> WelcomeMsg;
map<long long, string> DefaultRule;
set<long long> DisabledJRRPGroup;
set<long long> DisabledJRRPDiscuss;
set<long long> DisabledMEGroup;
set<long long> DisabledMEDiscuss;
set<long long> DisabledHELPGroup;
set<long long> DisabledHELPDiscuss;
set<long long> DisabledOBGroup;
set<long long> DisabledOBDiscuss;
unique_ptr<Initlist> ilInitList;

struct SourceType
{
	SourceType(long long a, int b, long long c) : QQ(a), Type(b), GrouporDiscussID(c)
	{
	}
	long long QQ = 0;
	int Type = 0;
	long long GrouporDiscussID = 0;

	bool operator<(SourceType b) const
	{
		return this->QQ < b.QQ;
	}
};

using PropType = map<string, int>;
map<SourceType, PropType> CharacterProp;
multimap<long long, long long> ObserveGroup;
multimap<long long, long long> ObserveDiscuss;
string strFileLoc;

//��������
void dataBackUp() {
	//����MasterQQ
	ofstream ofstreamMaster(strFileLoc + "Master.RDconf", ios::out | ios::trunc);
	ofstreamMaster << masterQQ << std::endl << boolMasterMode << std::endl << boolDisabledGlobal << std::endl << boolDisabledMeGlobal << std::endl << boolPreserve << std::endl << boolDisabledJrrpGlobal << std::endl << boolNoDiscuss << std::endl;
	ofstreamMaster.close();
	//���ݸ��Ի����
	ofstream ofstreamPersonalMsg(strFileLoc + "PersonalMsg.RDconf", ios::out | ios::trunc);
	for (auto it = PersonalMsg.begin(); it != PersonalMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamPersonalMsg << it->first << std::endl << it->second << std::endl;
	}
	//����CustomMsg
	ofstream ofstreamCustomMsg(strFileLoc + "CustomMsg.json", ios::out | ios::trunc);
	ofstreamCustomMsg << "{\n";
	for (auto it : EditedMsg)
	{
		while (it.second.find("\r\n") != string::npos)it.second.replace(it.second.find("\r\n"), 2, "\\n");
		while (it.second.find('\n') != string::npos)it.second.replace(it.second.find('\n'), 1, "\\n");
		while (it.second.find('\r') != string::npos)it.second.replace(it.second.find('\r'), 1, "\\r");
		while (it.second.find("\t") != string::npos)it.second.replace(it.second.find("\t"), 1, "\\t");
		string strMsg = GBKtoUTF8(it.second);
		ofstreamCustomMsg <<"\"" << it.first << "\":\"" << strMsg << "\",";
	}
	ofstreamCustomMsg << "\n\"Number\":\"" << EditedMsg.size() << "\"\n}" << endl;
	//����Ĭ�Ϲ���
	ofstream ofstreamDefaultRule(strFileLoc + "DefaultRule.RDconf", ios::out | ios::trunc);
	for (auto it = DefaultRule.begin(); it != DefaultRule.end(); ++it)
	{
		ofstreamDefaultRule << it->first << std::endl << it->second << std::endl;
	}
	//���ݰ�����Ⱥ
	ofstream ofstreamWhiteGroup(strFileLoc + "WhiteGroup.RDconf", ios::out | ios::trunc);
	for (auto it = WhiteGroup.begin(); it != WhiteGroup.end(); ++it)
	{
		ofstreamWhiteGroup << *it << std::endl;
	}
	ofstreamWhiteGroup.close();
	//���ݺ�����Ⱥ
	ofstream ofstreamBlackGroup(strFileLoc + "BlackGroup.RDconf", ios::out | ios::trunc);
	for (auto it : BlackGroup)
	{
		ofstreamBlackGroup << it << std::endl;
	}
	ofstreamBlackGroup.close();
	//���ݰ������û�
	ofstream ofstreamWhiteQQ(strFileLoc + "WhiteQQ.RDconf", ios::out | ios::trunc);
	for (auto it : WhiteQQ)
	{
		ofstreamWhiteQQ << it << std::endl;
	}
	ofstreamWhiteQQ.close();
	//���ݺ������û�
	ofstream ofstreamBlackQQ(strFileLoc + "BlackQQ.RDconf", ios::out | ios::trunc);
	for (auto it : BlackQQ)
	{
		ofstreamBlackQQ << it << std::endl;
	}
	ofstreamBlackQQ.close();
	//�����������б�
	ofstream ofstreamDiscussList(strFileLoc + "DiscussList.map", ios::out | ios::trunc);
	for (auto it : DiscussList)
	{
		ofstreamDiscussList << it.first << "\n" << it.second << std::endl;
	}
	ofstreamDiscussList.close();
}
EVE_Enable(eventEnable)
{
	//Wait until the thread terminates
	while (msgSendThreadRunning)
		Sleep(10);

	thread msgSendThread(SendMsg);
	msgSendThread.detach();
	strFileLoc = getAppDirectory();
	/*
	* ���ƴ洢-�������ȡ
	*/
	Name = make_unique<NameStorage>(strFileLoc + "Name.dicedb");
	//��ȡMasterMode
	ifstream ifstreamMaster(strFileLoc + "Master.RDconf");
	if (ifstreamMaster)
	{
		ifstreamMaster >> masterQQ >> boolMasterMode >> boolDisabledGlobal >> boolDisabledMeGlobal >> boolPreserve >> boolDisabledJrrpGlobal >> boolNoDiscuss;
	}
	ifstreamMaster.close();
	ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
	if (ifstreamCharacterProp)
	{
		long long QQ, GrouporDiscussID;
		int Type, Value;
		string SkillName;
		while (ifstreamCharacterProp >> QQ >> Type >> GrouporDiscussID >> SkillName >> Value)
		{
			CharacterProp[SourceType(QQ, Type, GrouporDiscussID)][SkillName] = Value;
		}
	}
	ifstreamCharacterProp.close();

	ifstream ifstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf");
	if (ifstreamDisabledGroup)
	{
		long long Group;
		while (ifstreamDisabledGroup >> Group)
		{
			DisabledGroup.insert(Group);
		}
	}
	ifstreamDisabledGroup.close();
	ifstream ifstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf");
	if (ifstreamDisabledDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledDiscuss >> Discuss)
		{
			DisabledDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledDiscuss.close();
	ifstream ifstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf");
	if (ifstreamDisabledJRRPGroup)
	{
		long long Group;
		while (ifstreamDisabledJRRPGroup >> Group)
		{
			DisabledJRRPGroup.insert(Group);
		}
	}
	ifstreamDisabledJRRPGroup.close();
	ifstream ifstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf");
	if (ifstreamDisabledJRRPDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledJRRPDiscuss >> Discuss)
		{
			DisabledJRRPDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledJRRPDiscuss.close();
	ifstream ifstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf");
	if (ifstreamDisabledMEGroup)
	{
		long long Group;
		while (ifstreamDisabledMEGroup >> Group)
		{
			DisabledMEGroup.insert(Group);
		}
	}
	ifstreamDisabledMEGroup.close();
	ifstream ifstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf");
	if (ifstreamDisabledMEDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledMEDiscuss >> Discuss)
		{
			DisabledMEDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledMEDiscuss.close();
	ifstream ifstreamDisabledHELPGroup(strFileLoc + "DisabledHELPGroup.RDconf");
	if (ifstreamDisabledHELPGroup)
	{
		long long Group;
		while (ifstreamDisabledHELPGroup >> Group)
		{
			DisabledHELPGroup.insert(Group);
		}
	}
	ifstreamDisabledHELPGroup.close();
	ifstream ifstreamDisabledHELPDiscuss(strFileLoc + "DisabledHELPDiscuss.RDconf");
	if (ifstreamDisabledHELPDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledHELPDiscuss >> Discuss)
		{
			DisabledHELPDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledHELPDiscuss.close();
	ifstream ifstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf");
	if (ifstreamDisabledOBGroup)
	{
		long long Group;
		while (ifstreamDisabledOBGroup >> Group)
		{
			DisabledOBGroup.insert(Group);
		}
	}
	ifstreamDisabledOBGroup.close();
	ifstream ifstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf");
	if (ifstreamDisabledOBDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledOBDiscuss >> Discuss)
		{
			DisabledOBDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledOBDiscuss.close();
	ifstream ifstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf");
	if (ifstreamObserveGroup)
	{
		long long Group, QQ;
		while (ifstreamObserveGroup >> Group >> QQ)
		{
			ObserveGroup.insert(make_pair(Group, QQ));
		}
	}
	ifstreamObserveGroup.close();

	ifstream ifstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf");
	if (ifstreamObserveDiscuss)
	{
		long long Discuss, QQ;
		while (ifstreamObserveDiscuss >> Discuss >> QQ)
		{
			ObserveDiscuss.insert(make_pair(Discuss, QQ));
		}
	}
	ifstreamObserveDiscuss.close();
	ifstream ifstreamDefault(strFileLoc + "Default.RDconf");
	if (ifstreamDefault)
	{
		long long QQ;
		int DefVal;
		while (ifstreamDefault >> QQ >> DefVal)
		{
			DefaultDice[QQ] = DefVal;
		}
	}
	ifstreamDefault.close();
	ifstream ifstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf");
	if (ifstreamWelcomeMsg)
	{
		long long GroupID;
		string Msg;
		while (ifstreamWelcomeMsg >> GroupID >> Msg)
		{
			while (Msg.find("{space}") != string::npos)Msg.replace(Msg.find("{space}"), 7, " ");
			while (Msg.find("{tab}") != string::npos)Msg.replace(Msg.find("{tab}"), 5, "\t");
			while (Msg.find("{endl}") != string::npos)Msg.replace(Msg.find("{endl}"), 6, "\n");
			while (Msg.find("{enter}") != string::npos)Msg.replace(Msg.find("{enter}"), 7, "\r");
			WelcomeMsg[GroupID] = Msg;
		}
	}
	ifstreamWelcomeMsg.close();
	ifstream ifstreamDefaultRule(strFileLoc + "DefaultRule.RDconf");
	if (ifstreamDefaultRule)
	{
		long long QQ;
		string strRule;
		while (ifstreamWelcomeMsg >> QQ >> strRule){
			DefaultRule[QQ] = strRule;
		}
	}
	ifstreamDefaultRule.close();
	ifstream ifstreamPersonalMsg(strFileLoc + "PersonalMsg.RDconf");
	if (ifstreamPersonalMsg)
	{
		string strType;
		string Msg;
		while (ifstreamPersonalMsg >> strType >> Msg)
		{
			while (Msg.find("{space}") != string::npos)Msg.replace(Msg.find("{space}"), 7, " ");
			while (Msg.find("{tab}") != string::npos)Msg.replace(Msg.find("{tab}"), 5, "\t");
			while (Msg.find("{endl}") != string::npos)Msg.replace(Msg.find("{endl}"), 6, "\n");
			while (Msg.find("{enter}") != string::npos)Msg.replace(Msg.find("{enter}"), 7, "\r");
			PersonalMsg[strType] = Msg;
		}
	}
	ifstreamPersonalMsg.close();
	ifstream ifstreamWhiteGroup(strFileLoc + "WhiteGroup.RDconf");
	if (ifstreamWhiteGroup)
	{
		long long Group;
		while (ifstreamWhiteGroup >> Group)
		{
			WhiteGroup.insert(Group);
		}
	}
	ifstreamWhiteGroup.close();
	ifstream ifstreamBlackGroup(strFileLoc + "BlackGroup.RDconf");
	if (ifstreamBlackGroup)
	{
		long long Group;
		while (ifstreamBlackGroup >> Group)
		{
			BlackGroup.insert(Group);
		}
	}
	ifstreamBlackGroup.close();
	ifstream ifstreamWhiteQQ(strFileLoc + "WhiteQQ.RDconf");
	if (ifstreamWhiteQQ)
	{
		long long Group;
		while (ifstreamWhiteQQ >> Group)
		{
			WhiteQQ.insert(Group);
		}
	}
	ifstreamWhiteQQ.close();
	ifstream ifstreamBlackQQ(strFileLoc + "BlackQQ.RDconf");
	if (ifstreamBlackQQ)
	{
		long long Group;
		while (ifstreamBlackQQ >> Group)
		{
			BlackQQ.insert(Group);
		}
	}
	ifstreamBlackQQ.close();
	//��ȡ�������б�
	ifstream ifstreamDiscussList(strFileLoc + "DiscussList.map");
	if (ifstreamDiscussList)
	{
		long long llDiscuss;
		time_t tNow;
		while (ifstreamDiscussList >> llDiscuss >> tNow)
		{
			DiscussList[llDiscuss]=tNow;
		}
	}
	ifstreamDiscussList.close();
	ilInitList = make_unique<Initlist>(strFileLoc + "INIT.DiceDB");
	ifstream ifstreamCustomMsg(strFileLoc + "CustomMsg.json");
	if (ifstreamCustomMsg)
	{
		ReadCustomMsg(ifstreamCustomMsg);
	}
	ifstreamCustomMsg.close();
	//��ȡ����ģʽ
	ifstream ifstreamStandByMe(strFileLoc + "StandByMe.RDconf");
	if (ifstreamStandByMe)
	{
		ifstreamStandByMe >> IdentityQQ >> StandQQ;
		if (getLoginQQ() == StandQQ) {
			boolStandByMe = true;
			string strName,strMsg;
			while (ifstreamStandByMe) {
				ifstreamStandByMe >> strName >> strMsg;
				while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
				while (strMsg.find("\\b") != string::npos)strMsg.replace(strMsg.find("\\b"), 2, " ");
				GlobalMsg[strName] = strMsg;
			}
		}
	}
	ifstreamStandByMe.close();
	return 0;
}

//��������ָ��
int DiceReply(Msg fromMsg) {
	if (fromMsg.strMsg[0] != '.')
		return 0;
	int intMsgCnt = 1;
	int intT= (fromMsg.fromType == Private) ? PrivateT : (fromMsg.fromType == Group ? GroupT : DiscussT);
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;
	const string strNickName = getName(fromMsg.fromQQ, fromMsg.fromGroup);
	string strLowerMessage = fromMsg.strMsg;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), [](unsigned char c) { return tolower(c); });
	if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		if (intT == PrivateT) {
			fromMsg.reply("����");
			return 1;
		}
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum ==
			to_string(getLoginQQ()))
		{
			if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
			{
				intT==GroupT?setGroupLeave(fromMsg.fromGroup):setDiscussLeave(fromMsg.fromGroup);
			}
			else
			{
				fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
			}
		}
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 6) == "master"&&boolMasterMode) {
		intMsgCnt += 6;
		if (masterQQ == 0) {
			masterQQ = fromMsg.fromQQ;
			fromMsg.reply("���ʣ������"+GlobalMsg["strSelfName"]+"��Master��");
		}
		else if (fromMsg.fromQQ == masterQQ) {
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			//����ѡ��
			Process(fromMsg.strMsg.substr(intMsgCnt));
		}
		return 1;
	}
	if (BlackQQ.count(fromMsg.fromQQ) || (intT == GroupT &&BlackGroup.count(fromMsg.fromGroup))) {
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(
			static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum = strLowerMessage.substr(intMsgCnt, fromMsg.strMsg.find(' ', intMsgCnt) - intMsgCnt);
		if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
			getLoginQQ() % 10000))) 
		{
			if (Command == "on")
			{
				if (fromMsg.fromType == Group) {
					if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
					{
						if (DisabledGroup.count(fromMsg.fromGroup))
						{
							DisabledGroup.erase(fromMsg.fromGroup);
							fromMsg.reply(GlobalMsg["strBotOn"]);
						}
						else
						{
							fromMsg.reply(GlobalMsg["strBotOnAlready"]);
						}
					}
					else
					{
						fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
					}
				}
				else if (fromMsg.fromType == Discuss) {
					if (DisabledDiscuss.count(fromMsg.fromGroup))
					{
						DisabledDiscuss.erase(fromMsg.fromGroup);
						fromMsg.reply(GlobalMsg["strBotOn"]);
					}
					else
					{
						fromMsg.reply(GlobalMsg["strBotOnAlready"]);
					}
				}
			}
			else if (Command == "off")
			{
				if (fromMsg.fromType == Group) {
					if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
					{
						if (!DisabledGroup.count(fromMsg.fromGroup))
						{
							DisabledGroup.insert(fromMsg.fromGroup);
							fromMsg.reply(GlobalMsg["strBotOff"]);
						}
						else
						{
							fromMsg.reply(GlobalMsg["strBotOffAlready"]);
						}
					}
					else
					{
						fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
					}
				}
				else if (fromMsg.fromType == Discuss) {
					if (!DisabledDiscuss.count(fromMsg.fromGroup))
					{
						DisabledDiscuss.insert(fromMsg.fromGroup);
						fromMsg.reply(GlobalMsg["strBotOff"]);
					}
					else
					{
						fromMsg.reply(GlobalMsg["strBotOffAlready"]);
					}
				}
			}
			else
			{
				fromMsg.reply(Dice_Full_Ver + GlobalMsg["strBotMsg"]);
			}
		}
		return 1;
	}
	if (boolDisabledGlobal) {
		if (intT == PrivateT)fromMsg.reply(GlobalMsg["strGlobalOff"]);
		return 1;
	}
	if (!fromMsg.isCalled && (intT == GroupT && DisabledGroup.count(fromMsg.fromGroup)))return 0;
	if (!fromMsg.isCalled && (intT == DiscussT && DisabledDiscuss.count(fromMsg.fromGroup)))return 0;
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		const string strAction = strLowerMessage.substr(intMsgCnt);
		if (strAction == "on")
		{
			if (fromMsg.fromType == Group) {
				if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
				{
					if (DisabledHELPGroup.count(fromMsg.fromGroup))
					{
						DisabledHELPGroup.erase(fromMsg.fromGroup);
						fromMsg.reply("�ɹ��ڱ�Ⱥ������.help����!");
					}
					else
					{
						fromMsg.reply("�ڱ�Ⱥ��.help����û�б�����!");
					}
				}
				else
				{
					fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
				}
			}
			else if (fromMsg.fromType == Discuss) {
				if (DisabledHELPDiscuss.count(fromMsg.fromGroup))
				{
					DisabledHELPDiscuss.erase(fromMsg.fromGroup);
					fromMsg.reply("�ɹ��ڱ�Ⱥ������.help����!");
				}
				else
				{
					fromMsg.reply("�ڱ�Ⱥ��.help����û�б�����!");
				}
			}
			return 1;
		}
		if (strAction == "off")
		{
			if (fromMsg.fromType == Group) {
				if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
				{
					if (!DisabledHELPGroup.count(fromMsg.fromGroup))
					{
						DisabledHELPGroup.insert(fromMsg.fromGroup);
						fromMsg.reply("�ɹ��ڱ�Ⱥ�н���.help����!");
					}
					else
					{
						fromMsg.reply("�ڱ�Ⱥ��.help����û�б�����!");
					}
				}
				else
				{
					fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
				}
			}
			else if (fromMsg.fromType == Discuss) {
				if (!DisabledHELPDiscuss.count(fromMsg.fromGroup))
				{
					DisabledHELPDiscuss.insert(fromMsg.fromGroup);
					fromMsg.reply("�ɹ��ڱ�Ⱥ�н���.help����!");
				}
				else
				{
					fromMsg.reply("�ڱ�Ⱥ��.help����û�б�����!");
				}
			}
			return 1;
		}
		if (DisabledHELPGroup.count(fromMsg.fromGroup))
		{
			fromMsg.reply(GlobalMsg["strHELPDisabledErr"]);
			return 1;
		}
		fromMsg.reply(GlobalMsg["strHlpMsg"]);
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome")
	{
		if (intT != GroupT) {
			fromMsg.reply("�����⻶ӭ˭�أ�");
			return 1;
		}
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
		{
			string strWelcomeMsg = fromMsg.strMsg.substr(intMsgCnt);
			if (strWelcomeMsg.empty())
			{
				if (WelcomeMsg.count(fromMsg.fromGroup))
				{
					WelcomeMsg.erase(fromMsg.fromGroup);
					fromMsg.reply(GlobalMsg["strWelcomeMsgClearNotice"]);
				}
				else
				{
					fromMsg.reply(GlobalMsg["strWelcomeMsgClearErr"]);
				}
			}
			else
			{
				WelcomeMsg[fromMsg.fromGroup] = strWelcomeMsg;
				fromMsg.reply(GlobalMsg["strWelcomeMsgUpdateNotice"]);
			}
		}
		else
		{
			fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
	string strReply = strNickName;
	COC7D(strReply);
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
	string strReply = strNickName;
	COC6D(strReply);
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "group")
	{
		if (intT != GroupT)return 1;
		if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions == 1) {
			fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
			return 1;
		}
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(
			static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strReply;
		if (Command == "state") {
			time_t tNow = time(NULL);
			const int intTMonth= 30 * 24 * 60 * 60;
			set<string> sInact;
			set<string> sBlackQQ; 
			for (auto each : getGroupMemberList(fromMsg.fromGroup)) {
				if (!each.LastMsgTime || tNow - each.LastMsgTime > intTMonth) {
					sInact.insert(each.GroupNick + "(" + to_string(each.QQID) + ")");
				}
				if (BlackQQ.count(each.QQID)) {
					sBlackQQ.insert(each.GroupNick + "(" + to_string(each.QQID) + ")");
				}
			}
			strReply += getGroupList()[fromMsg.fromGroup] + "������Ⱥ��״:\n"
				+ "Ⱥ��:" + to_string(fromMsg.fromGroup) + "\n"
				+ GlobalMsg["strSelfName"] + "�ڱ�Ⱥ״̬��" + (DisabledGroup.count(fromMsg.fromGroup) ? "����" : "����") + "\n"
				+ ".me��" + (DisabledMEGroup.count(fromMsg.fromGroup) ? "����" : "����") + "\n"
				+ ".jrrp��" + (DisabledJRRPGroup.count(fromMsg.fromGroup) ? "����" : "����")+ "\n"
				+ (DisabledOBGroup.count(fromMsg.fromGroup) ? "�ѽ����Թ�ģʽ\n" : "")
				+ "��Ⱥ��ӭ:" +(WelcomeMsg.count(fromMsg.fromGroup) ? "������" : "δ����")
				+ (sInact.size() ? "\n30�첻��ԾȺԱ����" + to_string(sInact.size()) : "");
			if (sBlackQQ.size()) {
				strReply += "\n"+ GlobalMsg["strSelfName"] +"�ĺ�������Ա:";
				for (auto each : sBlackQQ) {
					strReply += "\n" + each;
				}
			}
			
			fromMsg.reply(strReply);
			return 1;
		}
		if (getGroupMemberInfo(fromMsg.fromGroup, getLoginQQ()).permissions == 1) {
			fromMsg.reply(GlobalMsg["strSelfPermissionErr"]);
			return 1;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum;
		if (strLowerMessage.substr(intMsgCnt, 10) == "[cq:at,qq=") {
			intMsgCnt += 10;
			while (isdigit(strLowerMessage[intMsgCnt])) {
				QQNum += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intMsgCnt++;
		}
		else while (isdigit(strLowerMessage[intMsgCnt])) {
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (Command == "ban")
		{
			long long llMemberQQ = stoll(QQNum);
			GroupMemberInfo Member = getGroupMemberInfo(fromMsg.fromGroup, llMemberQQ);
			if (Member.QQID == llMemberQQ)
			{
				if (Member.permissions > 1) {
					fromMsg.reply(GlobalMsg["strSelfPermissionErr"]);
					return 1;
				}
				string strMainDice;
				while (isdigit(strLowerMessage[intMsgCnt]) || (strLowerMessage[intMsgCnt]) == 'd' || (strLowerMessage[intMsgCnt]) == '+' || (strLowerMessage[intMsgCnt]) == '-') {
					strMainDice += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				const int intDefaultDice = DefaultDice.count(fromMsg.fromQQ) ? DefaultDice[fromMsg.fromQQ] : 100;
				RD rdMainDice(strMainDice, intDefaultDice);
				rdMainDice.Roll();
				int intDuration = rdMainDice.intTotal;
				if (setGroupBan(fromMsg.fromGroup, llMemberQQ, intDuration * 60) == 0)
					if (intDuration <= 0)
						fromMsg.reply("�ö�" + getName(Member.QQID, fromMsg.fromGroup) + "������ԡ�");
					else fromMsg.reply("�ö�" + getName(Member.QQID, fromMsg.fromGroup) + "����ʱ��" + rdMainDice.FormCompleteString() + "���ӡ�");
				else fromMsg.reply("����ʧ�ܡ�");
			}
			else fromMsg.reply("���޴��ˡ�");
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
	intMsgCnt += 5;
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;
	if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])) || fromMsg.strMsg[intMsgCnt] == ':')
			intMsgCnt++;
		string strDefaultRule = fromMsg.strMsg.substr(intMsgCnt);
		if (strDefaultRule.empty()) {
			DefaultRule.erase(fromMsg.fromQQ);
			fromMsg.reply(GlobalMsg["strRuleReset"]);
		}
		else {
			for (auto& n : strDefaultRule)
				n = toupper(static_cast<unsigned char>(n));
			DefaultRule[fromMsg.fromQQ] = strDefaultRule;
			fromMsg.reply(GlobalMsg["strRuleSet"]);
		}
	}
	else {
		string strSearch = fromMsg.strMsg.substr(intMsgCnt);
		string strDefaultRule = DefaultRule[fromMsg.fromQQ];
		for (auto& n : strSearch)
			n = toupper(static_cast<unsigned char>(n));
		string strReturn;
		if (DefaultRule.count(fromMsg.fromQQ) && strSearch.find(':') == string::npos && GetRule::get(strDefaultRule, strSearch, strReturn)) {
			fromMsg.reply(strReturn);
		}
		else if (GetRule::analyze(strSearch, strReturn))
		{
			fromMsg.reply(strReturn);
		}
		else
		{
			fromMsg.reply(GlobalMsg["strRuleErr"] + strReturn);
		}
	}
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6")
	{
	intMsgCnt += 4;
	if (strLowerMessage[intMsgCnt] == 's')
		intMsgCnt++;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strNum;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strNum += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.length() > 2)
	{
		fromMsg.reply(GlobalMsg["strCharacterTooBig"]);
		return 1;
	}
	const int intNum = stoi(strNum.empty() ? "1" : strNum);
	if (intNum > 10)
	{
		fromMsg.reply(GlobalMsg["strCharacterTooBig"]);
		return 1;
	}
	if (intNum == 0)
	{
		fromMsg.reply(GlobalMsg["strCharacterCannotBeZero"]);
		return 1;
	}
	string strReply = strNickName;
	COC6(strReply, intNum);
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw")
	{
	intMsgCnt += 4;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;

	string strDeckName;
	while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strDeckName += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strCardNum;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strCardNum += fromMsg.strMsg[intMsgCnt];
		intMsgCnt++;
	}
	auto intCardNum = strCardNum.empty() ? 1 : stoi(strCardNum);
	if (intCardNum == 0)
	{
		fromMsg.reply(GlobalMsg["strNumCannotBeZero"]);
		return 1;
	}
	int intFoundRes = CardDeck::findDeck(strDeckName);
	string strReply;
	if (intFoundRes == 0) {
		strReply = "��˵" + strDeckName + "?" + GlobalMsg["strDeckNotFound"];
		fromMsg.reply(strReply);
		return 1;
	}
	vector<string> TempDeck(CardDeck::mPublicDeck[strDeckName]);
	strReply = "������" + strNickName + "�������ȡ���:\n" + CardDeck::drawCard(TempDeck);
	while (--intCardNum && TempDeck.size()) {
		strReply += "\n" + CardDeck::drawCard(TempDeck);
		if (strReply.length() > 1000) {
			fromMsg.reply(strReply);
			strReply.clear();
		}
	}
	fromMsg.reply(strReply);
	if (intCardNum) {
		fromMsg.reply(GlobalMsg["strDeckEmpty"]);
		return 1;
	}
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init"&&intT)
	{
	intMsgCnt += 4;
	string strReply;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
	if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
	{
		if (ilInitList->clear(fromMsg.fromGroup))
			strReply = "�ɹ�����ȹ���¼��";
		else
			strReply = "�б�Ϊ�գ�";
		fromMsg.reply(strReply);
		return 1;
	}
	ilInitList->show(fromMsg.fromGroup, strReply);
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
	if (boolDisabledJrrpGlobal) {
		fromMsg.reply(GlobalMsg["strDisabledJrrpGlobal"]);
		return 1;
	}
	intMsgCnt += 4;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	const string Command = strLowerMessage.substr(intMsgCnt, fromMsg.strMsg.find(' ', intMsgCnt) - intMsgCnt);
	if (intT == GroupT) {
		if (Command == "on")
		{
			if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
			{
				if (DisabledJRRPGroup.count(fromMsg.fromGroup))
				{
					DisabledJRRPGroup.erase(fromMsg.fromGroup);
					fromMsg.reply("�ɹ��ڱ�Ⱥ������JRRP!");
				}
				else
				{
					fromMsg.reply("�ڱ�Ⱥ��JRRPû�б�����!");
				}
			}
			else
			{
				fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
			}
			return 1;
		}
		if (Command == "off")
		{
			if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
			{
				if (!DisabledJRRPGroup.count(fromMsg.fromGroup))
				{
					DisabledJRRPGroup.insert(fromMsg.fromGroup);
					fromMsg.reply("�ɹ��ڱ�Ⱥ�н���JRRP!");
				}
				else
				{
					fromMsg.reply("�ڱ�Ⱥ��JRRPû�б�����!");
				}
			}
			else
			{
				fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
			}
			return 1;
		}
		if (DisabledJRRPGroup.count(fromMsg.fromGroup))
		{
			fromMsg.reply("�ڱ�Ⱥ��JRRP�����ѱ�����");
			return 1;
		}
	}
	else if (intT == DiscussT) {
		if (Command == "on")
		{
			if (DisabledJRRPDiscuss.count(fromMsg.fromGroup))
			{
				DisabledJRRPDiscuss.erase(fromMsg.fromGroup);
				fromMsg.reply("�ɹ��ڴ˶�������������JRRP!");
			}
			else
			{
				fromMsg.reply("�ڴ˶���������JRRPû�б�����!");
			}
			return 1;
		}
		if (Command == "off")
		{
			if (!DisabledJRRPDiscuss.count(fromMsg.fromGroup))
			{
				DisabledJRRPDiscuss.insert(fromMsg.fromGroup);
				fromMsg.reply("�ɹ��ڴ˶��������н���JRRP!");
			}
			else
			{
				fromMsg.reply("�ڴ˶���������JRRPû�б�����!");
			}
			return 1;
		}
		if (DisabledJRRPDiscuss.count(fromMsg.fromGroup))
		{
			fromMsg.reply("�ڴ˶���������JRRP�ѱ�����!");
			return 1;
		}
	}
	string des;
	string data = "QQ=" + to_string(CQ::getLoginQQ()) + "&v=20190114" + "&QueryQQ=" + to_string(fromMsg.fromQQ);
	char *frmdata = new char[data.length() + 1];
	strcpy_s(frmdata, data.length() + 1, data.c_str());
	bool res = Network::POST("api.kokona.tech", "/jrrp", 5555, frmdata, des);
	delete[] frmdata;
	if (res)
	{
		fromMsg.reply(format(GlobalMsg["strJrrp"], { strNickName, des }));
	}
	else
	{
		fromMsg.reply(format(GlobalMsg["strJrrpErr"], { des }));
	}
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
	{
	intMsgCnt += 4;
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;

	string type;
	while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		type += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}

	auto nameType = NameGenerator::Type::UNKNOWN;
	if (type == "cn")
		nameType = NameGenerator::Type::CN;
	else if (type == "en")
		nameType = NameGenerator::Type::EN;
	else if (type == "jp")
		nameType = NameGenerator::Type::JP;

	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;

	string strNum;
	while (isdigit(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
	{
		strNum += fromMsg.strMsg[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.size() > 2)
	{
		fromMsg.reply(GlobalMsg["strNameNumTooBig"]);
		return 1;
	}
	int intNum = stoi(strNum.empty() ? "1" : strNum);
	if (intNum > 10)
	{
		fromMsg.reply(GlobalMsg["strNameNumTooBig"]);
		return 1;
	}
	if (intNum == 0)
	{
		fromMsg.reply(GlobalMsg["strNameNumCannotBeZero"]);
		return 1;
	}
	vector<string> TempNameStorage;
	while (TempNameStorage.size() != intNum)
	{
		string name = NameGenerator::getRandomName(nameType);
		if (find(TempNameStorage.begin(), TempNameStorage.end(), name) == TempNameStorage.end())
		{
			TempNameStorage.push_back(name);
		}
	}
	string strReply = strNickName + "���������:\n";
	for (auto i = 0; i != TempNameStorage.size(); i++)
	{
		strReply.append(TempNameStorage[i]);
		if (i != TempNameStorage.size() - 1)strReply.append(", ");
	}
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
	{
	intMsgCnt += 3;
	if (strLowerMessage[intMsgCnt] == '7')
		intMsgCnt++;
	if (strLowerMessage[intMsgCnt] == 's')
		intMsgCnt++;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strNum;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strNum += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.length() > 2)
	{
		fromMsg.reply(GlobalMsg["strCharacterTooBig"]);
		return 1;
	}
	const int intNum = stoi(strNum.empty() ? "1" : strNum);
	if (intNum > 10)
	{
		fromMsg.reply(GlobalMsg["strCharacterTooBig"]);
		return 1;
	}
	if (intNum == 0)
	{
		fromMsg.reply(GlobalMsg["strCharacterCannotBeZero"]);
		return 1;
	}
	string strReply = strNickName;
	COC7(strReply, intNum);
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
	intMsgCnt += 3;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strNum;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strNum += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.length() > 2)
	{
		fromMsg.reply(GlobalMsg["strCharacterTooBig"]);
		return 1;
	}
	const int intNum = stoi(strNum.empty() ? "1" : strNum);
	if (intNum > 10)
	{
		fromMsg.reply(GlobalMsg["strCharacterTooBig"]);
		return 1;
	}
	if (intNum == 0)
	{
		fromMsg.reply(GlobalMsg["strCharacterCannotBeZero"]);
		return 1;
	}
	string strReply = strNickName;
	DND(strReply, intNum);
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn")
	{
	intMsgCnt += 3;
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;
	string type = strLowerMessage.substr(intMsgCnt, 2);
	string name;
	if (type == "cn")
		name = NameGenerator::getChineseName();
	else if (type == "en")
		name = NameGenerator::getEnglishName();
	else if (type == "jp")
		name = NameGenerator::getJapaneseName();
	else
		name = NameGenerator::getRandomName();
	Name->set(fromMsg.fromGroup, fromMsg.fromQQ, name);
	const string strReply = format(GlobalMsg["strNameSet"], { strNickName, strip(name) });
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
	intMsgCnt += 3;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
	while (strDefaultDice[0] == '0')
		strDefaultDice.erase(strDefaultDice.begin());
	if (strDefaultDice.empty())
		strDefaultDice = "100";
	for (auto charNumElement : strDefaultDice)
		if (!isdigit(static_cast<unsigned char>(charNumElement)))
		{
			fromMsg.reply(GlobalMsg["strSetInvalid"]);
			return 1;
		}
	if (strDefaultDice.length() > 3 && strDefaultDice != "1000")
	{
		fromMsg.reply(GlobalMsg["strSetTooBig"]);
		return 1;
	}
	const int intDefaultDice = stoi(strDefaultDice);
	if (intDefaultDice == 100)
		DefaultDice.erase(fromMsg.fromQQ);
	else
		DefaultDice[fromMsg.fromQQ] = intDefaultDice;
	const string strSetSuccessReply = "�ѽ�" + strNickName + "��Ĭ�������͸���ΪD" + strDefaultDice;
	fromMsg.reply(strSetSuccessReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "str"&&boolMasterMode&&fromMsg.fromQQ == masterQQ) {
	string strName;
	while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length())
	{
		strName += fromMsg.strMsg[intMsgCnt];
		intMsgCnt++;
	}
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;
	if (intMsgCnt == fromMsg.strMsg.length()) {
		EditedMsg.erase(strName);
		fromMsg.reply("�����" + strName + "���Զ��壬���ָ�Ĭ��������Ҫ����Ӧ�á�");
	}
	else if (GlobalMsg.count(strName)) {
		string strMsg = fromMsg.strMsg.substr(intMsgCnt);
		EditedMsg[strName] = strMsg;
		GlobalMsg[strName] = (strName == "strHlpMsg") ? Dice_Short_Ver + "\n" + strMsg : strMsg;
		fromMsg.reply("�Ѽ���" + strName + "���Զ���");
	}
	else {
		fromMsg.reply("��˵" + strName + "�����ƺ����ǻ��õ�������");
	}

	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strSkillName;
	while (intMsgCnt != fromMsg.strMsg.length() && !isdigit(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])) && !isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt]))
		)
	{
		strSkillName += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;
	string strCurrentValue;
	while (isdigit(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
	{
		strCurrentValue += fromMsg.strMsg[intMsgCnt];
		intMsgCnt++;
	}
	int intCurrentVal;
	if (strCurrentValue.empty())
	{
		if (CharacterProp.count(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)) && CharacterProp[SourceType(
			fromMsg.fromQQ, intT, fromMsg.fromGroup)].count(strSkillName))
		{
			intCurrentVal = CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)][strSkillName];
		}
		else if (SkillDefaultVal.count(strSkillName))
		{
			intCurrentVal = SkillDefaultVal[strSkillName];
		}
		else
		{
			fromMsg.reply(GlobalMsg["strEnValInvalid"]);
			return 1;
		}
	}
	else
	{
		if (strCurrentValue.length() > 3)
		{
			fromMsg.reply(GlobalMsg["strEnValInvalid"]);

			return 1;
		}
		intCurrentVal = stoi(strCurrentValue);
	}

	string strAns = strNickName + "��" + strSkillName + "��ǿ��ɳ��춨:\n1D100=";
	const int intTmpRollRes = RandomGenerator::Randint(1, 100);
	strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

	if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
	{
		strAns += " ʧ��!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "û�б仯!";
	}
	else
	{
		strAns += " �ɹ�!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "����1D10=";
		const int intTmpRollD10 = RandomGenerator::Randint(1, 10);
		strAns += to_string(intTmpRollD10) + "��,��ǰΪ" + to_string(intCurrentVal + intTmpRollD10) + "��";
		if (strCurrentValue.empty())
		{
			CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)][strSkillName] = intCurrentVal +
				intTmpRollD10;
		}
	}
	fromMsg.reply(strAns);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
	string strAns = strNickName + "�ķ����-�ܽ�֢״:\n";
	LongInsane(strAns);
	fromMsg.reply(strAns);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
	if (boolDisabledMeGlobal)
	{
		fromMsg.reply(GlobalMsg["strDisabledMeGlobal"]);
		return 1;
	}
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	if (intT == 0) {
		string strGroupID;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strGroupID += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strAction = strip(fromMsg.strMsg.substr(intMsgCnt));

		for (auto i : strGroupID)
		{
			if (!isdigit(static_cast<unsigned char>(i)))
			{
				fromMsg.reply(GlobalMsg["strGroupIDInvalid"]);
				return 1;
			}
		}
		if (strGroupID.empty())
		{
			fromMsg.reply("Ⱥ�Ų���Ϊ��!");
			return 1;
		}
		if (strAction.empty())
		{
			fromMsg.reply("��������Ϊ��!");
			return 1;
		}
		const long long llGroupID = stoll(strGroupID);
		if (DisabledGroup.count(llGroupID))
		{
			fromMsg.reply(GlobalMsg["strDisabledErr"]);
			return 1;
		}
		if (DisabledMEGroup.count(llGroupID))
		{
			fromMsg.reply(GlobalMsg["strMEDisabledErr"]);
			return 1;
		}
		string strReply = getName(fromMsg.fromQQ, llGroupID) + strAction;
		const int intSendRes = sendGroupMsg(llGroupID, strReply);
		if (intSendRes < 0)
		{
			fromMsg.reply(GlobalMsg["strSendErr"]);
		}
		else
		{
			fromMsg.reply("����ִ�гɹ�!");
		}
		return 1;
	}
	string strAction = strLowerMessage.substr(intMsgCnt);
	if (intT == GroupT) {
		if (strAction == "on")
		{
			if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
			{
				if (DisabledMEGroup.count(fromMsg.fromGroup))
				{
					DisabledMEGroup.erase(fromMsg.fromGroup);
					fromMsg.reply("�ɹ��ڱ�Ⱥ������.me����!");
				}
				else
				{
					fromMsg.reply("�ڱ�Ⱥ��.me����û�б�����!");
				}
			}
			else
			{
				fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
			}
			return 1;
		}
		if (strAction == "off")
		{
			if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
			{
				if (!DisabledMEGroup.count(fromMsg.fromGroup))
				{
					DisabledMEGroup.insert(fromMsg.fromGroup);
					fromMsg.reply("�ɹ��ڱ�Ⱥ�н���.me����!");
				}
				else
				{
					fromMsg.reply("�ڱ�Ⱥ��.me����û�б�����!");
				}
			}
			else
			{
				fromMsg.reply(GlobalMsg["strPermissionDeniedErr"]);
			}
			return 1;
		}
		if (DisabledMEGroup.count(fromMsg.fromGroup))
		{
			fromMsg.reply(GlobalMsg["strMEDisabledErr"]);
			return 1;
		}
	}
	else if (intT == DiscussT) {
		if (strAction == "on")
		{
			if (DisabledMEDiscuss.count(fromMsg.fromGroup))
			{
				DisabledMEDiscuss.erase(fromMsg.fromGroup);
				fromMsg.reply("�ɹ��ڱ���������������.me����!");
			}
			else
			{
				fromMsg.reply("�ڱ�����������.me����û�б�����!");
			}
			return 1;
		}
		if (strAction == "off")
		{
			if (!DisabledMEDiscuss.count(fromMsg.fromGroup))
			{
				DisabledMEDiscuss.insert(fromMsg.fromGroup);
				fromMsg.reply("�ɹ��ڱ����������н���.me����!");
			}
			else
			{
				fromMsg.reply("�ڱ�����������.me����û�б�����!");
			}
			return 1;
		}
		if (DisabledMEDiscuss.count(fromMsg.fromGroup))
		{
			fromMsg.reply(GlobalMsg["strMEDisabledErr"]);
			return 1;
		}
	}
	strAction = strip(fromMsg.strMsg.substr(intMsgCnt));
	if (strAction.empty())
	{
		fromMsg.reply("��������Ϊ��!");
		return 1;
	}
	const string strReply = strNickName + strAction;
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;
	string name = strip(fromMsg.strMsg.substr(intMsgCnt));
	if (name.length() > 50)
	{
		fromMsg.reply(GlobalMsg["strNameTooLongErr"]);
		return 1;
	}
	if (!name.empty())
	{
		Name->set(fromMsg.fromGroup, fromMsg.fromQQ, name);
		const string strReply = format(GlobalMsg["strNameSet"], { strNickName, strip(name) });
		fromMsg.reply(strReply);
	}
	else
	{
		if (Name->del(fromMsg.fromGroup, fromMsg.fromQQ))
		{
			const string strReply = "�ѽ�" + strNickName + "������ɾ��";
			fromMsg.reply(strReply);
		}
		else
		{
			const string strReply = strNickName + GlobalMsg["strNameDelErr"];
			fromMsg.reply(strReply);
		}
	}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob")
	{
	if (intT == PrivateT) {
		fromMsg.reply("���뿴ʲôѽ��");
		return 1;
	}
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	const string Command = strLowerMessage.substr(intMsgCnt, fromMsg.strMsg.find(' ', intMsgCnt) - intMsgCnt);
	if (Command == "on")
	{
		if (intT == GroupT) {
			if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
			{
				if (DisabledOBGroup.count(fromMsg.fromGroup))
				{
					DisabledOBGroup.erase(fromMsg.fromGroup);
					fromMsg.reply("�ɹ��ڱ�Ⱥ�������Թ�ģʽ!");
				}
				else
				{
					fromMsg.reply("��Ⱥ���Թ�ģʽû�б�����!");
				}
			}
			else
			{
				fromMsg.reply("��û��Ȩ��ִ�д�����!");
			}
			return 1;
		}
		else {
			if (DisabledOBDiscuss.count(fromMsg.fromGroup))
			{
				DisabledOBDiscuss.erase(fromMsg.fromGroup);
				fromMsg.reply("�ɹ��ڱ����������������Թ�ģʽ!");
			}
			else
			{
				fromMsg.reply("�ڱ������������Թ�ģʽû�б�����!");
			}
			return 1;
		}
	}
	if (Command == "off")
	{
		if (intT == Group) {
			if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
			{
				if (!DisabledOBGroup.count(fromMsg.fromGroup))
				{
					DisabledOBGroup.insert(fromMsg.fromGroup);
					ObserveGroup.clear();
					fromMsg.reply("�ɹ��ڱ�Ⱥ�н����Թ�ģʽ!");
				}
				else
				{
					fromMsg.reply("��Ⱥ���Թ�ģʽû�б�����!");
				}
			}
			else
			{
				fromMsg.reply("��û��Ȩ��ִ�д�����!");
			}
			return 1;
		}
		else {
			if (!DisabledOBDiscuss.count(fromMsg.fromGroup))
			{
				DisabledOBDiscuss.insert(fromMsg.fromGroup);
				ObserveDiscuss.clear();
				fromMsg.reply("�ɹ��ڱ����������н����Թ�ģʽ!");
			}
			else
			{
				fromMsg.reply("�ڱ������������Թ�ģʽû�б�����!");
			}
			return 1;
		}
	}
	if (intT == GroupT && DisabledOBGroup.count(fromMsg.fromGroup))
	{
		fromMsg.reply("�ڱ�Ⱥ���Թ�ģʽ�ѱ�����!");
		return 1;
	}
	if (intT == DiscussT && DisabledOBDiscuss.count(fromMsg.fromGroup))
	{
		fromMsg.reply("�ڱ������������Թ�ģʽ�ѱ�����!");
		return 1;
	}
	if (Command == "list")
	{
		string Msg = "��ǰ���Թ�����:";
		const auto Range = (intT == GroupT) ? ObserveGroup.equal_range(fromMsg.fromGroup) : ObserveDiscuss.equal_range(fromMsg.fromGroup);
		for (auto it = Range.first; it != Range.second; ++it)
		{
			Msg += "\n" + getName(it->second, fromMsg.fromGroup) + "(" + to_string(it->second) + ")";
		}
		const string strReply = Msg == "��ǰ���Թ�����:" ? "��ǰ�����Թ���" : Msg;
		fromMsg.reply(strReply);
	}
	else if (Command == "clr")
	{
		if (intT = DiscussT) {
			ObserveDiscuss.erase(fromMsg.fromGroup);
			fromMsg.reply("�ɹ�ɾ�������Թ���!");
		}
		else if (getGroupMemberInfo(fromMsg.fromGroup, fromMsg.fromQQ).permissions >= 2)
		{
			ObserveGroup.erase(fromMsg.fromGroup);
			fromMsg.reply("�ɹ�ɾ�������Թ���!");
		}
		else
		{
			fromMsg.reply("��û��Ȩ��ִ�д�����!");
		}
	}
	else if (Command == "exit")
	{
		const auto Range = (intT == GroupT) ? ObserveGroup.equal_range(fromMsg.fromGroup) : ObserveDiscuss.equal_range(fromMsg.fromGroup);
		for (auto it = Range.first; it != Range.second; ++it)
		{
			if (it->second == fromMsg.fromQQ)
			{
				(intT == GroupT) ? ObserveGroup.erase(it) : ObserveDiscuss.erase(it);
				const string strReply = strNickName + "�ɹ��˳��Թ�ģʽ!";
				fromMsg.reply(strReply);
				return 1;
			}
		}
		const string strReply = strNickName + "û�м����Թ�ģʽ!";
		fromMsg.reply(strReply);
	}
	else
	{
		const auto Range = (intT == GroupT) ? ObserveGroup.equal_range(fromMsg.fromGroup) : ObserveDiscuss.equal_range(fromMsg.fromGroup);
		for (auto it = Range.first; it != Range.second; ++it)
		{
			if (it->second == fromMsg.fromQQ)
			{
				const string strReply = strNickName + "�Ѿ������Թ�ģʽ!";
				fromMsg.reply(strReply);
				return 1;
			}
		}
		(intT == GroupT) ? ObserveGroup.insert(make_pair(fromMsg.fromGroup, fromMsg.fromQQ)) : ObserveDiscuss.insert(make_pair(fromMsg.fromGroup, fromMsg.fromQQ));
		const string strReply = strNickName + "�ɹ������Թ�ģʽ!";
		fromMsg.reply(strReply);
	}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
	{
	intMsgCnt += 2;
	string strSkillName;
	string strMainDice = "D100";
	string strSkillModify;
	int intSkillModify = 0;
	if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
		strMainDice = strLowerMessage[intMsgCnt];
		intMsgCnt++;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strMainDice += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
	while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
		isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
		':'&& strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')
	{
		strSkillName += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
	if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
		strSkillModify += strLowerMessage[intMsgCnt];
		intMsgCnt++;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		intSkillModify = stoi(strSkillModify);
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
		':')intMsgCnt++;
	string strSkillVal;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strSkillVal += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		intMsgCnt++;
	}
	string strReason = fromMsg.strMsg.substr(intMsgCnt);
	int intSkillVal;
	if (strSkillVal.empty())
	{
		if (CharacterProp.count(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)) && CharacterProp[SourceType(
			fromMsg.fromQQ, intT, fromMsg.fromGroup)].count(strSkillName))
		{
			intSkillVal = CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)][strSkillName];
		}
		else if (SkillDefaultVal.count(strSkillName))
		{
			intSkillVal = SkillDefaultVal[strSkillName];
		}
		else
		{
			fromMsg.reply(GlobalMsg["strUnknownPropErr"]);
			return 1;
		}
	}
	else if (strSkillVal.length() > 3)
	{
		fromMsg.reply(GlobalMsg["strPropErr"]);
		return 1;
	}
	else
	{
		intSkillVal = stoi(strSkillVal);
	}
	int intFianlSkillVal = intSkillVal + intSkillModify;
	if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
	{
		fromMsg.reply(GlobalMsg["strSuccessRateErr"]);
		return 1;
	}
	RD rdMainDice(strMainDice);
	const int intFirstTimeRes = rdMainDice.Roll();
	if (intFirstTimeRes == ZeroDice_Err)
	{
		fromMsg.reply(GlobalMsg["strZeroDiceErr"]);
		return 1;
	}
	if (intFirstTimeRes == DiceTooBig_Err)
	{
		fromMsg.reply(GlobalMsg["strDiceTooBigErr"]);
		return 1;
	}
	const int intD100Res = rdMainDice.intTotal;
	string strReply = strNickName + "����" + strSkillName + strSkillModify + "�춨: " + rdMainDice.FormCompleteString() + "/" +
		to_string(intFianlSkillVal) + " ";
	if (intD100Res <= 5 && intD100Res <= intSkillVal)strReply += GlobalMsg["strRollCriticalSuccess"];
	else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
	else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
	else if (intD100Res <= intFianlSkillVal / 2)strReply += GlobalMsg["strRollHardSuccess"];
	else if (intD100Res <= intFianlSkillVal)strReply += GlobalMsg["strRollRegularSuccess"];
	else if (intD100Res <= 95)strReply += GlobalMsg["strRollFailure"];
	else strReply += GlobalMsg["strRollFumble"];
	if (!strReason.empty())
	{
		strReply = "����" + strReason + " " + strReply;
	}
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
	intMsgCnt += 2;
	string strSkillName;
	string strMainDice = "D100";
	string strSkillModify;
	int intSkillModify = 0;
	if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
		strMainDice = strLowerMessage[intMsgCnt];
		intMsgCnt++;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strMainDice += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
	while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
		isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
		':' && strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')
	{
		strSkillName += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
	if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
		strSkillModify += strLowerMessage[intMsgCnt];
		intMsgCnt++;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		intSkillModify = stoi(strSkillModify);
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
		':')intMsgCnt++;
	string strSkillVal;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strSkillVal += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		intMsgCnt++;
	}
	string strReason = fromMsg.strMsg.substr(intMsgCnt);
	int intSkillVal;
	if (strSkillVal.empty())
	{
		if (CharacterProp.count(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)) && CharacterProp[SourceType(
			fromMsg.fromQQ, intT, fromMsg.fromGroup)].count(strSkillName))
		{
			intSkillVal = CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)][strSkillName];
		}
		else if (SkillDefaultVal.count(strSkillName))
		{
			intSkillVal = SkillDefaultVal[strSkillName];
		}
		else
		{
			fromMsg.reply(GlobalMsg["strUnknownPropErr"]);
			return 1;
		}
	}
	else if (strSkillVal.length() > 3)
	{
		fromMsg.reply(GlobalMsg["strPropErr"]);
		return 1;
	}
	else
	{
		intSkillVal = stoi(strSkillVal);
	}
	int intFianlSkillVal = intSkillVal + intSkillModify;
	if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
	{
		fromMsg.reply(GlobalMsg["strSuccessRateErr"]);
		return 1;
	}
	RD rdMainDice(strMainDice);
	const int intFirstTimeRes = rdMainDice.Roll();
	if (intFirstTimeRes == ZeroDice_Err)
	{
		fromMsg.reply(GlobalMsg["strZeroDiceErr"]);
		return 1;
	}
	if (intFirstTimeRes == DiceTooBig_Err)
	{
		fromMsg.reply(GlobalMsg["strDiceTooBigErr"]);
		return 1;
	}
	const int intD100Res = rdMainDice.intTotal;
	string strReply = strNickName + "����" + strSkillName + strSkillModify + "�춨: " + rdMainDice.FormCompleteString() + "/" +
		to_string(intFianlSkillVal) + " ";
	if (intD100Res == 1)strReply += GlobalMsg["strRollCriticalSuccess"];
	else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
	else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
	else if (intD100Res <= intFianlSkillVal / 2)strReply += GlobalMsg["strRollHardSuccess"];
	else if (intD100Res <= intFianlSkillVal)strReply += GlobalMsg["strRollRegularSuccess"];
	else if (intD100Res <= 95)strReply += GlobalMsg["strRollFailure"];
	else strReply += GlobalMsg["strRollFumble"];
	if (!strReason.empty())
	{
		strReply = "����" + strReason + " " + strReply;
	}
	fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri"&&intT)
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		}
		else if (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			strinit += '+';
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strname = fromMsg.strMsg.substr(intMsgCnt);
		if (strname.empty())
			strname = strNickName;
		else
			strname = strip(strname);
		RD initdice(strinit);
		const int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			fromMsg.reply(GlobalMsg["strValueErr"]);
			return 1;
		}
		if (intFirstTimeRes == Input_Err)
		{
			fromMsg.reply(GlobalMsg["strInputErr"]);
			return 1;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			fromMsg.reply(GlobalMsg["strZeroDiceErr"]);
			return 1;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			fromMsg.reply(GlobalMsg["strZeroTypeErr"]);
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strDiceTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strTypeTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			fromMsg.reply(GlobalMsg["strAddDiceValErr"]);
			return 1;
		}
		if (intFirstTimeRes != 0)
		{
			fromMsg.reply(GlobalMsg["strUnknownErr"]);
			return 1;
		}
		ilInitList->insert(fromMsg.fromGroup, initdice.intTotal, strname);
		const string strReply = strname + "���ȹ����㣺" + strinit + '=' + to_string(initdice.intTotal);
		fromMsg.reply(strReply);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string SanCost = strLowerMessage.substr(intMsgCnt, fromMsg.strMsg.find(' ', intMsgCnt) - intMsgCnt);
	intMsgCnt += SanCost.length();
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string San;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		San += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (SanCost.empty() || SanCost.find("/") == string::npos)
	{
		fromMsg.reply(GlobalMsg["strSCInvalid"]);
		return 1;
	}
	if (San.empty() && !(CharacterProp.count(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)) && CharacterProp[
		SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)].count("����")))
	{
		fromMsg.reply(GlobalMsg["strSanInvalid"]);
		return 1;
	}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				fromMsg.reply(GlobalMsg["strSCInvalid"]);
				return 1;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				fromMsg.reply(GlobalMsg["strSCInvalid"]);
				return 1;
			}
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{
			fromMsg.reply(GlobalMsg["strSCInvalid"]);
			return 1;
		}
		if (San.length() >= 3)
		{
			fromMsg.reply(GlobalMsg["strSanInvalid"]);
			return 1;
		}
		const int intSan = San.empty() ? CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)]["����"] : stoi(San);
		if (intSan == 0)
		{
			fromMsg.reply(GlobalMsg["strSanInvalid"]);
			return 1;
		}
		string strAns = strNickName + "��Sancheck:\n1D100=";
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " �ɹ�\n���Sanֵ����" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdSuc.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)]["����"] = max(0, intSan - rdSuc.intTotal);
			}
		}
		else if (intTmpRollRes == 100 || (intSan < 50 && intTmpRollRes > 95))
		{
			strAns += " ��ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdFail.Max();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "���ֵ=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)]["����"] = max(0, intSan - rdFail.intTotal);
			}
		}
		else
		{
			strAns += " ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)]["����"] = max(0, intSan - rdFail.intTotal);
			}
		}
		fromMsg.reply(strAns);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	if (intMsgCnt == strLowerMessage.length())
	{
		fromMsg.reply(GlobalMsg["strStErr"]);
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
	{
		if (CharacterProp.count(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)))
		{
			CharacterProp.erase(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup));
		}
		fromMsg.reply(GlobalMsg["strPropCleared"]);
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "del")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
			intMsgCnt] == '|'))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)) && CharacterProp[SourceType(
				fromMsg.fromQQ, intT, fromMsg.fromGroup)].count(strSkillName))
			{
				CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)].erase(strSkillName);
				fromMsg.reply(GlobalMsg["strPropDeleted"]);
			}
			else
			{
				fromMsg.reply(GlobalMsg["strPropNotFound"]);
			}
			return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 4) == "show")
	{
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
			intMsgCnt] == '|'))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
			SourceType sCharProp= SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup);
			if (strSkillName.empty()&& CharacterProp.count(sCharProp)) {
				string strReply = strNickName + "�������б�";
				for (auto each : CharacterProp[sCharProp]) {
					strReply += " " + each.first + ":" + to_string(each.second);
				}
				fromMsg.reply(strReply);
				return 1;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)) && CharacterProp[SourceType(
				fromMsg.fromQQ, intT, fromMsg.fromGroup)].count(strSkillName))
			{
				fromMsg.reply(format(GlobalMsg["strProp"], {
					strNickName, strSkillName,
					to_string(CharacterProp[SourceType(fromMsg.fromQQ, intT, fromMsg.fromGroup)][strSkillName])
					}));
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				fromMsg.reply(format(GlobalMsg["strProp"], { strNickName, strSkillName, to_string(SkillDefaultVal[strSkillName]) }));
			}
			else
			{
				fromMsg.reply(GlobalMsg["strPropNotFound"]);
			}
			return 1;
	}
	bool boolError = false;
	while (intMsgCnt != strLowerMessage.length())
	{
		string strSkillName;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
			isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
			!= ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
		] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 3)
		{
			boolError = true;
			break;
		}
		CharacterProp[SourceType(fromMsg.fromQQ, GroupT, fromMsg.fromGroup)][strSkillName] = stoi(strSkillVal);
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
	}
	if (boolError)
	{
		fromMsg.reply(GlobalMsg["strPropErr"]);
	}
	else
	{
		fromMsg.reply(GlobalMsg["strSetPropSuccess"]);
	}
	}	
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "�ķ����-��ʱ֢״:\n";
		TempInsane(strAns);
		fromMsg.reply(strAns);
	}
	else if (strLowerMessage[intMsgCnt] == 'w')
	{
	intMsgCnt++;
	bool boolDetail = false;
	if (strLowerMessage[intMsgCnt] == 'w')
	{
		intMsgCnt++;
		boolDetail = true;
	}
	bool isHidden = false;
	if (strLowerMessage[intMsgCnt] == 'h')
	{
		isHidden = true;
		intMsgCnt += 1;
	}
	if (intT == 0)isHidden = false;
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
		intMsgCnt++;

	int intTmpMsgCnt;
	for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != fromMsg.strMsg.length() && fromMsg.strMsg[intTmpMsgCnt] != ' ';
		intTmpMsgCnt++)
	{
		if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
			intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
				&&
				strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
					intTmpMsgCnt
				] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
		{
			break;
		}
	}
	string strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
	while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intTmpMsgCnt])))
		intTmpMsgCnt++;
	string strReason = fromMsg.strMsg.substr(intTmpMsgCnt);


	int intTurnCnt = 1;
	if (strMainDice.find("#") != string::npos)
	{
		string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
		if (strTurnCnt.empty())
			strTurnCnt = "1";
		strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
		const int intDefaultDice = DefaultDice.count(fromMsg.fromQQ) ? DefaultDice[fromMsg.fromQQ] : 100;
		RD rdTurnCnt(strMainDice, intDefaultDice);
		const int intRdTurnCntRes = rdTurnCnt.Roll();
		if (intRdTurnCntRes == Value_Err)
		{
			fromMsg.reply(GlobalMsg["strValueErr"]);
			return 1;
		}
		if (intRdTurnCntRes == Input_Err)
		{
			fromMsg.reply(GlobalMsg["strInputErr"]);
			return 1;
		}
		if (intRdTurnCntRes == ZeroDice_Err)
		{
			fromMsg.reply(GlobalMsg["strZeroDiceErr"]);
			return 1;
		}
		if (intRdTurnCntRes == ZeroType_Err)
		{
			fromMsg.reply(GlobalMsg["strZeroTypeErr"]);
			return 1;
		}
		if (intRdTurnCntRes == DiceTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strDiceTooBigErr"]);
			return 1;
		}
		if (intRdTurnCntRes == TypeTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strTypeTooBigErr"]);
			return 1;
		}
		if (intRdTurnCntRes == AddDiceVal_Err)
		{
			fromMsg.reply(GlobalMsg["strAddDiceValErr"]);
			return 1;
		}
		if (intRdTurnCntRes != 0)
		{
			fromMsg.reply(GlobalMsg["strUnknownErr"]);
			return 1;
		}
		if (rdTurnCnt.intTotal > 10)
		{
			fromMsg.reply(GlobalMsg["strRollTimeExceeded"]);
			return 1;
		}
		if (rdTurnCnt.intTotal <= 0)
		{
			fromMsg.reply(GlobalMsg["strRollTimeErr"]);
			return 1;
		}
		intTurnCnt = rdTurnCnt.intTotal;
		if (strTurnCnt.find("d") != string::npos)
		{
			string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
			if (!isHidden || intT == PrivateT)
			{
				fromMsg.reply(strTurnNotice);
			}
			else
			{
				strTurnNotice = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromMsg.fromGroup] : "��������(" + to_string(fromMsg.fromGroup)) + ")�� " + strTurnNotice;
				AddMsgToQueue(strTurnNotice, fromMsg.fromQQ, Private);
				const auto range = ObserveGroup.equal_range(fromMsg.fromGroup);
				for (auto it = range.first; it != range.second; ++it)
				{
					if (it->second != fromMsg.fromQQ)
					{
						AddMsgToQueue(strTurnNotice, it->second, Private);
					}
				}
			}
		}
	}
	if (strMainDice.empty())
	{
		fromMsg.reply(GlobalMsg["strEmptyWWDiceErr"]);
		return 1;
	}
	string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
		? strMainDice.find('+')
		: strMainDice.find('-'));
	bool boolAdda10 = true;
	for (auto i : strFirstDice)
	{
		if (!isdigit(static_cast<unsigned char>(i)))
		{
			boolAdda10 = false;
			break;
		}
	}
	if (boolAdda10)
		strMainDice.insert(strFirstDice.length(), "a10");
	const int intDefaultDice = DefaultDice.count(fromMsg.fromQQ) ? DefaultDice[fromMsg.fromQQ] : 100;
	RD rdMainDice(strMainDice, intDefaultDice);

	const int intFirstTimeRes = rdMainDice.Roll();
	if (intFirstTimeRes == Value_Err)
	{
		fromMsg.reply(GlobalMsg["strValueErr"]);
		return 1;
	}
	if (intFirstTimeRes == Input_Err)
	{
		fromMsg.reply(GlobalMsg["strInputErr"]);
		return 1;
	}
	if (intFirstTimeRes == ZeroDice_Err)
	{
		fromMsg.reply(GlobalMsg["strZeroDiceErr"]);
		return 1;
	}
	else
	{
		if (intFirstTimeRes == ZeroType_Err)
		{
			fromMsg.reply(GlobalMsg["strZeroTypeErr"]);
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strDiceTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strTypeTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			fromMsg.reply(GlobalMsg["strAddDiceValErr"]);
			return 1;
		}
		if (intFirstTimeRes != 0)
		{
			fromMsg.reply(GlobalMsg["strUnknownErr"]);
			return 1;
		}
	}
	if (!boolDetail && intTurnCnt != 1)
	{
		string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
		if (!strReason.empty())
			strAns.insert(0, "����" + strReason + " ");
		vector<int> vintExVal;
		while (intTurnCnt--)
		{
			// �˴�����ֵ����
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdMainDice.Roll();
			strAns += to_string(rdMainDice.intTotal);
			if (intTurnCnt != 0)
				strAns += ",";
			if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
				rdMainDice.intTotal >= 96))
				vintExVal.push_back(rdMainDice.intTotal);
		}
		strAns += " }";
		if (!vintExVal.empty())
		{
			strAns += ",��ֵ: ";
			for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
			{
				strAns += to_string(*it);
				if (it != vintExVal.cend() - 1)
					strAns += ",";
			}
		}
		if (!isHidden || intT == PrivateT)
		{
			fromMsg.reply(strAns);
		}
		else
		{
			strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromMsg.fromGroup] : "��������(" + to_string(fromMsg.fromGroup)) + ")�� " + strAns;
			AddMsgToQueue(strAns, fromMsg.fromQQ, Private);
			const auto range = ObserveGroup.equal_range(fromMsg.fromGroup);
			for (auto it = range.first; it != range.second; ++it)
			{
				if (it->second != fromMsg.fromQQ)
				{
					AddMsgToQueue(strAns, it->second, Private);
				}
			}
		}
	}
	else
	{
		while (intTurnCnt--)
		{
			// �˴�����ֵ����
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdMainDice.Roll();
			string strAns = strNickName + "������: " + (boolDetail
				? rdMainDice.FormCompleteString()
				: rdMainDice.FormShortString());
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			if (!isHidden || intT == PrivateT)
			{
				fromMsg.reply(strAns);
			}
			else
			{
				strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromMsg.fromGroup] : "��������(" + to_string(fromMsg.fromGroup)) + ")�� " + strAns;
				AddMsgToQueue(strAns, fromMsg.fromQQ, Private);
				const auto range = ObserveGroup.equal_range(fromMsg.fromGroup);
				for (auto it = range.first; it != range.second; ++it)
				{
					if (it->second != fromMsg.fromQQ)
					{
						AddMsgToQueue(strAns, it->second, Private);
					}
				}
			}
		}
	}
	if (isHidden)
	{
		const string strReply = strNickName + "������һ�ΰ���";
		fromMsg.reply(strReply);
	}
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'h'
		|| strLowerMessage[intMsgCnt] == 'd')
	{
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (fromMsg.strMsg[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		if(intT==0)isHidden = false;
		while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != fromMsg.strMsg.length() && fromMsg.strMsg[intTmpMsgCnt] != ' ';
			intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[
				intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f'
					||
					strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
				if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
					intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
						&&
						strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
							intTmpMsgCnt
						] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a'&& strLowerMessage[intTmpMsgCnt] != 'x'&& strLowerMessage[intTmpMsgCnt] != '*')
				{
					break;
				}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (isspace(static_cast<unsigned char>(fromMsg.strMsg[intTmpMsgCnt])))
				intTmpMsgCnt++;
			strReason = fromMsg.strMsg.substr(intTmpMsgCnt);
		}
		else
			strReason = fromMsg.strMsg.substr(intMsgCnt);

		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			const int intDefaultDice = DefaultDice.count(fromMsg.fromQQ) ? DefaultDice[fromMsg.fromQQ] : 100;
			RD rdTurnCnt(strMainDice, intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				fromMsg.reply(GlobalMsg["strValueErr"]);
				return 1;
			}
			if (intRdTurnCntRes == Input_Err)
			{
				fromMsg.reply(GlobalMsg["strInputErr"]);
				return 1;
			}
			if (intRdTurnCntRes == ZeroDice_Err)
			{
				fromMsg.reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			}
			if (intRdTurnCntRes == ZeroType_Err)
			{
				fromMsg.reply(GlobalMsg["strZeroTypeErr"]);
				return 1;
			}
			if (intRdTurnCntRes == DiceTooBig_Err)
			{
				fromMsg.reply(GlobalMsg["strDiceTooBigErr"]);
				return 1;
			}
			if (intRdTurnCntRes == TypeTooBig_Err)
			{
				fromMsg.reply(GlobalMsg["strTypeTooBigErr"]);
				return 1;
			}
			if (intRdTurnCntRes == AddDiceVal_Err)
			{
				fromMsg.reply(GlobalMsg["strAddDiceValErr"]);
				return 1;
			}
			if (intRdTurnCntRes != 0)
			{
				fromMsg.reply(GlobalMsg["strUnknownErr"]);
				return 1;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				fromMsg.reply(GlobalMsg["strRollTimeExceeded"]);
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				fromMsg.reply(GlobalMsg["strRollTimeErr"]);
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
				if (!isHidden)
				{
					fromMsg.reply(strTurnNotice);
				}
				else
				{
					strTurnNotice = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromMsg.fromGroup] : "��������(" + to_string(fromMsg.fromGroup)) + ")�� " + strTurnNotice;
					AddMsgToQueue(strTurnNotice, fromMsg.fromQQ, Private);
					const auto range = ObserveGroup.equal_range(fromMsg.fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != fromMsg.fromQQ)
						{
							AddMsgToQueue(strTurnNotice, it->second, Private);
						}
					}
				}
			}
		}
		const int intDefaultDice = DefaultDice.count(fromMsg.fromQQ) ? DefaultDice[fromMsg.fromQQ] : 100;
		RD rdMainDice(strMainDice, intDefaultDice);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			fromMsg.reply(GlobalMsg["strValueErr"]);
			return 1;
		}
		if (intFirstTimeRes == Input_Err)
		{
			fromMsg.reply(GlobalMsg["strInputErr"]);
			return 1;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			fromMsg.reply(GlobalMsg["strZeroDiceErr"]);
			return 1;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			fromMsg.reply(GlobalMsg["strZeroTypeErr"]);
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strDiceTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			fromMsg.reply(GlobalMsg["strTypeTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			fromMsg.reply(GlobalMsg["strAddDiceValErr"]);
			return 1;
		}
		if (intFirstTimeRes != 0)
		{
			fromMsg.reply(GlobalMsg["strUnknownErr"]);
			return 1;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",��ֵ: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				fromMsg.reply(strAns);
			}
			else
			{
				strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromMsg.fromGroup] : "��������(" + to_string(fromMsg.fromGroup)) + ")�� " + strAns;
				AddMsgToQueue(strAns, fromMsg.fromQQ, Private);
				const auto range = ObserveGroup.equal_range(fromMsg.fromGroup);
				for (auto it = range.first; it != range.second; ++it)
				{
					if (it->second != fromMsg.fromQQ)
					{
						AddMsgToQueue(strAns, it->second, Private);
					}
				}
			}
		}
		else
		{
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "������: " + (boolDetail
					? rdMainDice.FormCompleteString()
					: rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "����" + strReason + " ");
				if (!isHidden)
				{
					fromMsg.reply(strAns);
				}
				else
				{
					strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromMsg.fromGroup] : "��������(" + to_string(fromMsg.fromGroup)) + ")�� " + strAns;
					AddMsgToQueue(strAns, fromMsg.fromQQ, Private);
					const auto range = ObserveGroup.equal_range(fromMsg.fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != fromMsg.fromQQ)
						{
							AddMsgToQueue(strAns, it->second, Private);
						}
					}
				}
			}
		}
		if (isHidden)
		{
			const string strReply = strNickName + "������һ�ΰ���";
			fromMsg.reply(strReply);
		}
	}
	return 0;
}

EVE_PrivateMsg_EX(eventPrivateMsg)
{
	if (eve.isSystem())return;
	if (BlackQQ.count(eve.fromQQ)) {
		eve.message_block();
		return;
	}
	init(eve.message);
	init2(eve.message);
	Msg fromMsg(eve.message, eve.fromQQ);
	if (DiceReply(fromMsg))eve.message_block();
	return;
}

EVE_GroupMsg_EX(eventGroupMsg)
{
	if (eve.isAnonymous())return;
	if (eve.isSystem()) {
		if (eve.message.find("������Ա����") != string::npos&&eve.message.find(to_string(getLoginQQ())) != string::npos) {
			long long fromQQ;
			int intAuthCnt = 0;
			for (auto member : getGroupMemberList(eve.fromGroup)) {
				if (member.permissions == 3) {
					//��Ӧ���ľ�����Ⱥ����������
					fromQQ = member.QQID;
				}
				else if (member.permissions == 2) {
					//��¼����Ա����
					intAuthCnt++;
				}
			}
			if (masterQQ&&boolMasterMode) {
				string strMsg = "��Ⱥ\"" + getGroupList()[eve.fromGroup] + "\"(" + to_string(eve.fromGroup) + ")��," + eve.message + "\nȺ��" + getStrangerInfo(fromQQ).nick + "(" + to_string(fromQQ) + "),���й���Ա" + to_string(intAuthCnt) + "��";
				AddMsgToQueue(strMsg, masterQQ);
				BlackGroup.insert(eve.fromGroup);
				if (WhiteGroup.count(eve.fromGroup))WhiteGroup.erase(eve.fromGroup);
				//setGroupLeave(eve.fromGroup);
			}
			string strInfo = "{\"LoginQQ\":\"" + to_string(getLoginQQ()) + "\",\"fromGroup\":" + to_string(eve.fromGroup) + "\",\"Type\":\"banned\",\"fromQQ\":\"" + to_string(fromQQ) + "\"";
		}
		else return;
	}
	init(eve.message);
	while (isspace(static_cast<unsigned char>(eve.message[0])))
		eve.message.erase(eve.message.begin());
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	bool boolNamed=false;
	if (eve.message.substr(0, 6) == "[CQ:at")
	{
		if (eve.message.substr(0, strAt.length()) == strAt)
		{
			eve.message = eve.message.substr(strAt.length());
			boolNamed = true;
		}
		else
		{
			return;
		}
	}
	init2(eve.message);
	Msg fromMsg(eve.message, eve.fromGroup, Group, eve.fromQQ);
	fromMsg.isCalled = boolNamed;
	if(DiceReply(fromMsg))eve.message_block();
	return;
}

EVE_DiscussMsg_EX(eventDiscussMsg)
{
	time_t tNow = time(NULL);
	if (boolNoDiscuss) {
		AddMsgToQueue(GlobalMsg["strNoDiscuss"], eve.fromDiscuss, Discuss);
		Sleep(1000);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	if (eve.isSystem()) {
		if (boolMasterMode&&eve.message.find("�Ƴ�") != string::npos&&eve.message.find("��") != string::npos|| eve.message.find("��") != string::npos) {
			if (masterQQ) {
				AddMsgToQueue("��������"+to_string(eve.fromDiscuss)+"�У�"+eve.message, masterQQ);
			}
		}
		return;
	}
	DiscussList[eve.fromDiscuss] = tNow;
	init(eve.message);
	bool boolNamed = false;
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	if (eve.message.substr(0, 6) == "[CQ:at")
	{
		if (eve.message.substr(0, strAt.length()) == strAt)
		{
			eve.message = eve.message.substr(strAt.length() + 1);
			boolNamed = true;
		}
		else
		{
			return;
		}
	}
	init2(eve.message);

	Msg fromMsg(eve.message, eve.fromDiscuss, Discuss, eve.fromQQ);
	fromMsg.isCalled = boolNamed;
	if (DiceReply(fromMsg))eve.message_block();
	return;
}

EVE_System_GroupMemberIncrease(eventGroupMemberIncrease)
{
	if (beingOperateQQ != getLoginQQ() && WelcomeMsg.count(fromGroup))
	{
		string strReply = WelcomeMsg[fromGroup];
		while (strReply.find("{@}") != string::npos)
		{
			strReply.replace(strReply.find("{@}"), 3, "[CQ:at,qq=" + to_string(beingOperateQQ) + "]");
		}
		while (strReply.find("{nick}") != string::npos)
		{
			strReply.replace(strReply.find("{nick}"), 6, getStrangerInfo(beingOperateQQ).nick);
		}
		while (strReply.find("{age}") != string::npos)
		{
			strReply.replace(strReply.find("{age}"), 5, to_string(getStrangerInfo(beingOperateQQ).age));
		}
		while (strReply.find("{sex}") != string::npos)
		{
			strReply.replace(strReply.find("{sex}"), 5,
			                 getStrangerInfo(beingOperateQQ).sex == 0
				                 ? "��"
				                 : getStrangerInfo(beingOperateQQ).sex == 1
				                 ? "Ů"
				                 : "δ֪");
		}
		while (strReply.find("{qq}") != string::npos)
		{
			strReply.replace(strReply.find("{qq}"), 4, to_string(beingOperateQQ));
		}
		AddMsgToQueue(strReply, fromGroup, Group);
	}
	else if(beingOperateQQ == getLoginQQ()){
		if (BlackGroup.count(fromGroup)) {
			AddMsgToQueue(GlobalMsg["strBlackGroup"], fromGroup, Group);
		}
		else if (boolPreserve&&WhiteGroup.count(fromGroup)==0) 
		{	//����СȺ�ƹ�����û���ϰ�����
			if (fromQQ==masterQQ||WhiteQQ.count(fromQQ)) {
				WhiteGroup.insert(fromGroup);
				return 0;
			}
			AddMsgToQueue(GlobalMsg["strPreserve"], fromGroup, Group);
			setGroupLeave(fromGroup);
			return 0;
		}
		//else if(boolStandByMe&&getGroupMemberInfo(fromGroup, IdentityQQ).QQID != IdentityQQ) {
		//	AddMsgToQueue("�벻Ҫ������������Ⱥ��", fromGroup, Group);
		//	setGroupLeave(fromGroup);
		//	return 0;
		//}
		else if(!GlobalMsg["strAddGroup"].empty()) {
			AddMsgToQueue(GlobalMsg["strAddGroup"], fromGroup, Group);
		}
	}
	return 0;
}

EVE_System_GroupMemberDecrease(eventGroupMemberDecrease) {
	if (beingOperateQQ == getLoginQQ()) {
		if (masterQQ&&boolMasterMode) {
			string strMsg = to_string(fromQQ)+"��"+GlobalMsg["strSelfName"]+"�Ƴ���Ⱥ" + to_string(fromGroup) + "��";
			AddMsgToQueue(strMsg, masterQQ,Private);
			if (WhiteQQ.count(fromQQ)) {
				WhiteQQ.erase(fromQQ);
			}
			BlackQQ.insert(fromQQ);
			if (WhiteGroup.count(fromGroup)) {
				WhiteGroup.erase(fromGroup);
			}
			BlackGroup.insert(fromGroup);
		}
		string strInfo = "{\"LoginQQ\":\"" + to_string(getLoginQQ()) + "\",\"fromGroup\":" + to_string(fromGroup) + "\",\"Type\":\"kicked\",\"fromQQ\":\"" + to_string(fromQQ) + "\"";
	}
	return 0;
}

EVE_Request_AddGroup(eventGroupInvited) {
	if (subType == 2) {
		if (masterQQ&&boolMasterMode) {
			string strMsg = "Ⱥ����������ԣ�" + getStrangerInfo(fromQQ).nick +"("+ to_string(fromQQ) + "),Ⱥ��(" + to_string(fromGroup)+")��";
			if (BlackGroup.count(fromGroup)) {
				strMsg += "\n�Ѿܾ���Ⱥ�ں������У�";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else if (BlackQQ.count(fromQQ)) {
				strMsg += "\n�Ѿܾ����û��ں������У�";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else if (WhiteGroup.count(fromQQ)) {
				strMsg += "\n��ͬ�⣨Ⱥ�ڰ������У�";
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			else if (WhiteQQ.count(fromQQ)) {
				strMsg += "\n��ͬ�⣨�û��ڰ������У�";
				WhiteGroup.insert(fromQQ);
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			else if (boolPreserve) {
				strMsg += "\n�Ѿܾ�����ǰ��˽��ģʽ��";
				WhiteGroup.insert(fromQQ);
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else{
				strMsg += "��ͬ��";
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			AddMsgToQueue(strMsg, masterQQ, Private);
		}
	}
	return 1;
}

EVE_Menu(eventMasterMode) {
	if (boolMasterMode) {
		boolMasterMode = false;
		MessageBoxA(nullptr, "Masterģʽ�ѹرա�", "Masterģʽ�л�",MB_OK | MB_ICONINFORMATION);
	}else {
		boolMasterMode = true;
		MessageBoxA(nullptr, "Masterģʽ�ѿ�����", "Masterģʽ�л�", MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

EVE_Disable(eventDisable)
{
	Enabled = false;
	ilInitList.reset();
	Name.reset();
	dataBackUp();
	ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); ++it)
	{
		ofstreamDisabledGroup << *it << std::endl;
	}
	ofstreamDisabledGroup.close();

	ofstream ofstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledDiscuss.begin(); it != DisabledDiscuss.end(); ++it)
	{
		ofstreamDisabledDiscuss << *it << std::endl;
	}
	ofstreamDisabledDiscuss.close();
	ofstream ofstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPGroup.begin(); it != DisabledJRRPGroup.end(); ++it)
	{
		ofstreamDisabledJRRPGroup << *it << std::endl;
	}
	ofstreamDisabledJRRPGroup.close();

	ofstream ofstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPDiscuss.begin(); it != DisabledJRRPDiscuss.end(); ++it)
	{
		ofstreamDisabledJRRPDiscuss << *it << std::endl;
	}
	ofstreamDisabledJRRPDiscuss.close();

	ofstream ofstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEGroup.begin(); it != DisabledMEGroup.end(); ++it)
	{
		ofstreamDisabledMEGroup << *it << std::endl;
	}
	ofstreamDisabledMEGroup.close();

	ofstream ofstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEDiscuss.begin(); it != DisabledMEDiscuss.end(); ++it)
	{
		ofstreamDisabledMEDiscuss << *it << std::endl;
	}
	ofstreamDisabledMEDiscuss.close();

	ofstream ofstreamDisabledHELPGroup(strFileLoc + "DisabledHELPGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPGroup.begin(); it != DisabledHELPGroup.end(); ++it)
	{
		ofstreamDisabledHELPGroup << *it << std::endl;
	}
	ofstreamDisabledHELPGroup.close();

	ofstream ofstreamDisabledHELPDiscuss(strFileLoc + "DisabledHELPDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPDiscuss.begin(); it != DisabledHELPDiscuss.end(); ++it)
	{
		ofstreamDisabledHELPDiscuss << *it << std::endl;
	}
	ofstreamDisabledHELPDiscuss.close();

	ofstream ofstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBGroup.begin(); it != DisabledOBGroup.end(); ++it)
	{
		ofstreamDisabledOBGroup << *it << std::endl;
	}
	ofstreamDisabledOBGroup.close();

	ofstream ofstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBDiscuss.begin(); it != DisabledOBDiscuss.end(); ++it)
	{
		ofstreamDisabledOBDiscuss << *it << std::endl;
	}
	ofstreamDisabledOBDiscuss.close();

	ofstream ofstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveGroup.begin(); it != ObserveGroup.end(); ++it)
	{
		ofstreamObserveGroup << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveGroup.close();

	ofstream ofstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveDiscuss.begin(); it != ObserveDiscuss.end(); ++it)
	{
		ofstreamObserveDiscuss << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveDiscuss.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it)
	{
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
		{
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " "
				<< it1->first << " " << it1->second << std::endl;
		}
	}
	ofstreamCharacterProp.close();
	ofstream ofstreamDefault(strFileLoc + "Default.RDconf", ios::out | ios::trunc);
	for (auto it = DefaultDice.begin(); it != DefaultDice.end(); ++it)
	{
		ofstreamDefault << it->first << " " << it->second << std::endl;
	}
	ofstreamDefault.close();

	ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::out | ios::trunc);
	for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
	}
	ofstreamWelcomeMsg.close();
	DefaultDice.clear();
	DisabledGroup.clear();
	DisabledDiscuss.clear();
	DisabledJRRPGroup.clear();
	DisabledJRRPDiscuss.clear();
	DisabledMEGroup.clear();
	DisabledMEDiscuss.clear();
	DisabledOBGroup.clear();
	DisabledOBDiscuss.clear();
	ObserveGroup.clear();
	ObserveDiscuss.clear();
	strFileLoc.clear();
	return 0;
}

EVE_Exit(eventExit)
{
	if (!Enabled)
		return 0;
	ilInitList.reset();
	Name.reset();
	dataBackUp();
	ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); ++it)
	{
		ofstreamDisabledGroup << *it << std::endl;
	}
	ofstreamDisabledGroup.close();

	ofstream ofstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledDiscuss.begin(); it != DisabledDiscuss.end(); ++it)
	{
		ofstreamDisabledDiscuss << *it << std::endl;
	}
	ofstreamDisabledDiscuss.close();
	ofstream ofstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPGroup.begin(); it != DisabledJRRPGroup.end(); ++it)
	{
		ofstreamDisabledJRRPGroup << *it << std::endl;
	}
	ofstreamDisabledJRRPGroup.close();

	ofstream ofstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPDiscuss.begin(); it != DisabledJRRPDiscuss.end(); ++it)
	{
		ofstreamDisabledJRRPDiscuss << *it << std::endl;
	}
	ofstreamDisabledJRRPDiscuss.close();

	ofstream ofstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEGroup.begin(); it != DisabledMEGroup.end(); ++it)
	{
		ofstreamDisabledMEGroup << *it << std::endl;
	}
	ofstreamDisabledMEGroup.close();

	ofstream ofstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEDiscuss.begin(); it != DisabledMEDiscuss.end(); ++it)
	{
		ofstreamDisabledMEDiscuss << *it << std::endl;
	}
	ofstreamDisabledMEDiscuss.close();

	ofstream ofstreamDisabledHELPGroup(strFileLoc + "DisabledHELPGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPGroup.begin(); it != DisabledHELPGroup.end(); ++it)
	{
		ofstreamDisabledHELPGroup << *it << std::endl;
	}
	ofstreamDisabledHELPGroup.close();

	ofstream ofstreamDisabledHELPDiscuss(strFileLoc + "DisabledHELPDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPDiscuss.begin(); it != DisabledHELPDiscuss.end(); ++it)
	{
		ofstreamDisabledHELPDiscuss << *it << std::endl;
	}
	ofstreamDisabledHELPDiscuss.close();

	ofstream ofstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBGroup.begin(); it != DisabledOBGroup.end(); ++it)
	{
		ofstreamDisabledOBGroup << *it << std::endl;
	}
	ofstreamDisabledOBGroup.close();

	ofstream ofstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBDiscuss.begin(); it != DisabledOBDiscuss.end(); ++it)
	{
		ofstreamDisabledOBDiscuss << *it << std::endl;
	}
	ofstreamDisabledOBDiscuss.close();

	ofstream ofstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveGroup.begin(); it != ObserveGroup.end(); ++it)
	{
		ofstreamObserveGroup << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveGroup.close();

	ofstream ofstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveDiscuss.begin(); it != ObserveDiscuss.end(); ++it)
	{
		ofstreamObserveDiscuss << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveDiscuss.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it)
	{
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
		{
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " "
				<< it1->first << " " << it1->second << std::endl;
		}
	}
	ofstreamCharacterProp.close();
	ofstream ofstreamDefault(strFileLoc + "Default.RDconf", ios::out | ios::trunc);
	for (auto it = DefaultDice.begin(); it != DefaultDice.end(); ++it)
	{
		ofstreamDefault << it->first << " " << it->second << std::endl;
	}
	ofstreamDefault.close();

	ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::out | ios::trunc);
	for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
	}
	ofstreamWelcomeMsg.close();
	return 0;
}

MUST_AppInfo_RETURN(CQAPPID);
