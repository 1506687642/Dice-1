#pragma once
#ifndef DICE_EVENT
#define DICE_EVENT
#include <map>
#include <set>
#include "CQAPI_EX.h"
#include "DiceMsgSend.h"
#include "GlobalVar.h"
#include "MsgFormat.h"
#include "initList.h"
//�����������Ϣ
using namespace std;
using namespace CQ;

extern unique_ptr<NameStorage> Name;
string strip(std::string origin)
{
	while (true)
	{
		if (origin[0] == '!' || origin[0] == '.')
		{
			origin.erase(origin.begin());
		}
		else if (origin.substr(0, 2) == "��" || origin.substr(0, 2) == "��")
		{
			origin.erase(origin.begin());
			origin.erase(origin.begin());
		}
		else return origin;
	}
}

extern map<long long, int> DefaultDice;
extern map<long long, string> WelcomeMsg;
extern map<long long, string> DefaultRule;
extern set<long long> DisabledJRRPGroup;
extern set<long long> DisabledJRRPDiscuss;
extern set<long long> DisabledMEGroup;
extern set<long long> DisabledMEDiscuss;
extern set<long long> DisabledHELPGroup;
extern set<long long> DisabledHELPDiscuss;
extern set<long long> DisabledOBGroup;
extern set<long long> DisabledOBDiscuss;
extern unique_ptr<Initlist> ilInitList;
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
extern map<SourceType, PropType> CharacterProp;
extern multimap<long long, long long> ObserveGroup;
extern multimap<long long, long long> ObserveDiscuss;
class FromMsg {
public:
	std::string strMsg;
	long long fromID = 0;
	CQ::msgtype fromType = CQ::Private;
	long long fromQQ = 0;
	long long fromGroup = 0;
	FromMsg(std::string message, long long fromNum) :strMsg(message), fromQQ(fromNum), fromID(fromNum) {}

	FromMsg(std::string message, long long fromGroup, CQ::msgtype msgType, long long fromNum) :strMsg(message), fromQQ(fromNum), fromType(msgType), fromID(fromGroup), fromGroup(fromGroup) {}

	string getName(long long QQ , long long GroupID)
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
	void reply(std::string strReply) {
		AddMsgToQueue(strReply, fromID, fromType);
	}

	bool DiceReply() {
		if (strMsg[0] != '.')
			return false;
		intMsgCnt++ ;
		int intT = (fromType == Private) ? PrivateT : (fromType == Group ? GroupT : DiscussT);
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		const string strNickName = getName(fromQQ, fromGroup);
		string strLowerMessage = strMsg;
		std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), [](unsigned char c) { return tolower(c); });
		if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
		{
			if (intT == PrivateT) {
				reply("����");
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
				if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
				{
					intT == GroupT ? setGroupLeave(fromGroup) : setDiscussLeave(fromGroup);
				}
				else
				{
					reply(GlobalMsg["strPermissionDeniedErr"]);
				}
			}
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 6) == "master"&&boolMasterMode) {
			intMsgCnt += 6;
			if (masterQQ == 0) {
				masterQQ = fromQQ;
				reply("���ʣ������" + GlobalMsg["strSelfName"] + "��Master��");
			}
			else if (fromQQ == masterQQ) {
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				//����ѡ��
				ConsoleHandler(strMsg.substr(intMsgCnt));
			}
			return 1;
		}
		if (BlackQQ.count(fromQQ) || (intT == GroupT && BlackGroup.count(fromGroup))) {
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
			string QQNum = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				if (Command == "on")
				{
					if (fromType == Group) {
						if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
						{
							if (DisabledGroup.count(fromGroup))
							{
								DisabledGroup.erase(fromGroup);
								reply(GlobalMsg["strBotOn"]);
							}
							else
							{
								reply(GlobalMsg["strBotOnAlready"]);
							}
						}
						else
						{
							reply(GlobalMsg["strPermissionDeniedErr"]);
						}
					}
					else if (fromType == Discuss) {
						if (DisabledDiscuss.count(fromGroup))
						{
							DisabledDiscuss.erase(fromGroup);
							reply(GlobalMsg["strBotOn"]);
						}
						else
						{
							reply(GlobalMsg["strBotOnAlready"]);
						}
					}
				}
				else if (Command == "off")
				{
					if (fromType == Group) {
						if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
						{
							if (!DisabledGroup.count(fromGroup))
							{
								DisabledGroup.insert(fromGroup);
								reply(GlobalMsg["strBotOff"]);
							}
							else
							{
								reply(GlobalMsg["strBotOffAlready"]);
							}
						}
						else
						{
							reply(GlobalMsg["strPermissionDeniedErr"]);
						}
					}
					else if (fromType == Discuss) {
						if (!DisabledDiscuss.count(fromGroup))
						{
							DisabledDiscuss.insert(fromGroup);
							reply(GlobalMsg["strBotOff"]);
						}
						else
						{
							reply(GlobalMsg["strBotOffAlready"]);
						}
					}
				}
				else
				{
					reply(Dice_Full_Ver + GlobalMsg["strBotMsg"]);
				}
			}
			return 1;
		}
		if (boolDisabledGlobal) {
			if (intT == PrivateT)reply(GlobalMsg["strGlobalOff"]);
			return 1;
		}
		if (!isCalled && (intT == GroupT && DisabledGroup.count(fromGroup)))return 0;
		if (!isCalled && (intT == DiscussT && DisabledDiscuss.count(fromGroup)))return 0;
		if (strLowerMessage.substr(intMsgCnt, 4) == "help")
		{
			intMsgCnt += 4;
			while (strLowerMessage[intMsgCnt] == ' ')
				intMsgCnt++;
			const string strOption = strLowerMessage.substr(intMsgCnt);
			if (strOption == "on")
			{
				if (fromType == Group) {
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (DisabledHELPGroup.count(fromGroup))
						{
							DisabledHELPGroup.erase(fromGroup);
							reply("�ɹ��ڱ�Ⱥ������.help����!");
						}
						else
						{
							reply("�ڱ�Ⱥ��.help����û�б�����!");
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
				}
				else if (fromType == Discuss) {
					if (DisabledHELPDiscuss.count(fromGroup))
					{
						DisabledHELPDiscuss.erase(fromGroup);
						reply("�ɹ��ڱ�Ⱥ������.help����!");
					}
					else
					{
						reply("�ڱ�Ⱥ��.help����û�б�����!");
					}
				}
				return 1;
			}
			else if (strOption == "off")
			{
				if (fromType == Group) {
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (!DisabledHELPGroup.count(fromGroup))
						{
							DisabledHELPGroup.insert(fromGroup);
							reply("�ɹ��ڱ�Ⱥ�н���.help����!");
						}
						else
						{
							reply("�ڱ�Ⱥ��.help����û�б�����!");
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
				}
				else if (fromType == Discuss) {
					if (!DisabledHELPDiscuss.count(fromGroup))
					{
						DisabledHELPDiscuss.insert(fromGroup);
						reply("�ɹ��ڱ�Ⱥ�н���.help����!");
					}
					else
					{
						reply("�ڱ�Ⱥ��.help����û�б�����!");
					}
				}
				return 1;
			}
			if (DisabledHELPGroup.count(fromGroup))
			{
				reply(GlobalMsg["strHELPDisabledErr"]);
				return 1;
			}
			if (strOption.empty()) {
				reply(GlobalMsg["strHlpMsg"]);
			}
			else if (HelpDoc.count(strOption)) {
				string strReply = HelpDoc[strOption];
				while (strReply[0] == '&') {
					strReply = HelpDoc[strReply.substr(1)];
				}
				reply(strReply);
				return 1;
			}
			else {
				reply(GlobalMsg["strHlpNotFound"]);
			}

		}
		else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome")
		{
			if (intT != GroupT) {
				reply(GlobalMsg["strWelcomePrivate"]);
				return 1;
			}
			intMsgCnt += 7;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
			{
				string strWelcomeMsg = strMsg.substr(intMsgCnt);
				if (strWelcomeMsg.empty())
				{
					if (WelcomeMsg.count(fromGroup))
					{
						WelcomeMsg.erase(fromGroup);
						reply(GlobalMsg["strWelcomeMsgClearNotice"]);
					}
					else
					{
						reply(GlobalMsg["strWelcomeMsgClearErr"]);
					}
				}
				else
				{
					WelcomeMsg[fromGroup] = strWelcomeMsg;
					reply(GlobalMsg["strWelcomeMsgUpdateNotice"]);
				}
			}
			else
			{
				reply(GlobalMsg["strPermissionDeniedErr"]);
			}
		}
		else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
		{
			string strReply = strNickName;
			COC7D(strReply);
			reply(strReply);
		}
		else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
		{
			string strReply = strNickName;
			COC6D(strReply);
			reply(strReply);
		}
		else if (strLowerMessage.substr(intMsgCnt, 5) == "group")
		{
			if (intT != GroupT)return 1;
			if (getGroupMemberInfo(fromGroup, fromQQ).permissions == 1) {
				reply(GlobalMsg["strPermissionDeniedErr"]);
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
				const int intTMonth = 30 * 24 * 60 * 60;
				set<string> sInact;
				set<string> sBlackQQ;
				for (auto each : getGroupMemberList(fromGroup)) {
					if (!each.LastMsgTime || tNow - each.LastMsgTime > intTMonth) {
						sInact.insert(each.GroupNick + "(" + to_string(each.QQID) + ")");
					}
					if (BlackQQ.count(each.QQID)) {
						sBlackQQ.insert(each.GroupNick + "(" + to_string(each.QQID) + ")");
					}
				}
				strReply += getGroupList()[fromGroup] + "������Ⱥ��״:\n"
					+ "Ⱥ��:" + to_string(fromGroup) + "\n"
					+ GlobalMsg["strSelfName"] + "�ڱ�Ⱥ״̬��" + (DisabledGroup.count(fromGroup) ? "����" : "����") + "\n"
					+ ".me��" + (DisabledMEGroup.count(fromGroup) ? "����" : "����") + "\n"
					+ ".jrrp��" + (DisabledJRRPGroup.count(fromGroup) ? "����" : "����") + "\n"
					+ (DisabledOBGroup.count(fromGroup) ? "�ѽ����Թ�ģʽ\n" : "")
					+ "��Ⱥ��ӭ:" + (WelcomeMsg.count(fromGroup) ? "������" : "δ����")
					+ (sInact.size() ? "\n30�첻��ԾȺԱ����" + to_string(sInact.size()) : "");
				if (sBlackQQ.size()) {
					strReply += "\n" + GlobalMsg["strSelfName"] + "�ĺ�������Ա:";
					for (auto each : sBlackQQ) {
						strReply += "\n" + each;
					}
				}

				reply(strReply);
				return 1;
			}
			if (getGroupMemberInfo(fromGroup, getLoginQQ()).permissions == 1) {
				reply(GlobalMsg["strSelfPermissionErr"]);
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
				GroupMemberInfo Member = getGroupMemberInfo(fromGroup, llMemberQQ);
				if (Member.QQID == llMemberQQ)
				{
					if (Member.permissions > 1) {
						reply(GlobalMsg["strSelfPermissionErr"]);
						return 1;
					}
					string strMainDice;
					while (isdigit(strLowerMessage[intMsgCnt]) || (strLowerMessage[intMsgCnt]) == 'd' || (strLowerMessage[intMsgCnt]) == '+' || (strLowerMessage[intMsgCnt]) == '-') {
						strMainDice += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					const int intDefaultDice = DefaultDice.count(fromQQ) ? DefaultDice[fromQQ] : 100;
					RD rdMainDice(strMainDice, intDefaultDice);
					rdMainDice.Roll();
					int intDuration = rdMainDice.intTotal;
					if (setGroupBan(fromGroup, llMemberQQ, intDuration * 60) == 0)
						if (intDuration <= 0)
							reply("�ö�" + getName(Member.QQID, fromGroup) + "������ԡ�");
						else reply("�ö�" + getName(Member.QQID, fromGroup) + "����ʱ��" + rdMainDice.FormCompleteString() + "���ӡ�");
					else reply("����ʧ�ܡ�");
				}
				else reply("���޴��ˡ�");
			}
			return 1;
		}
		else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
		{
			intMsgCnt += 5;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
				intMsgCnt++;
			if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
				intMsgCnt += 3;
				while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == ':')
					intMsgCnt++;
				string strDefaultRule = strMsg.substr(intMsgCnt);
				if (strDefaultRule.empty()) {
					DefaultRule.erase(fromQQ);
					reply(GlobalMsg["strRuleReset"]);
				}
				else {
					for (auto& n : strDefaultRule)
						n = toupper(static_cast<unsigned char>(n));
					DefaultRule[fromQQ] = strDefaultRule;
					reply(GlobalMsg["strRuleSet"]);
				}
			}
			else {
				string strSearch = strMsg.substr(intMsgCnt);
				string strDefaultRule = DefaultRule[fromQQ];
				for (auto& n : strSearch)
					n = toupper(static_cast<unsigned char>(n));
				string strReturn;
				if (DefaultRule.count(fromQQ) && strSearch.find(':') == string::npos && GetRule::get(strDefaultRule, strSearch, strReturn)) {
					reply(strReturn);
				}
				else if (GetRule::analyze(strSearch, strReturn))
				{
					reply(strReturn);
				}
				else
				{
					reply(GlobalMsg["strRuleErr"] + strReturn);
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
				reply(GlobalMsg["strCharacterTooBig"]);
				return 1;
			}
			const int intNum = stoi(strNum.empty() ? "1" : strNum);
			if (intNum > 10)
			{
				reply(GlobalMsg["strCharacterTooBig"]);
				return 1;
			}
			if (intNum == 0)
			{
				reply(GlobalMsg["strCharacterCannotBeZero"]);
				return 1;
			}
			string strReply = strNickName;
			COC6(strReply, intNum);
			reply(strReply);
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
				strCardNum += strMsg[intMsgCnt];
				intMsgCnt++;
			}
			auto intCardNum = strCardNum.empty() ? 1 : stoi(strCardNum);
			if (intCardNum == 0)
			{
				reply(GlobalMsg["strNumCannotBeZero"]);
				return 1;
			}
			int intFoundRes = CardDeck::findDeck(strDeckName);
			string strReply;
			if (intFoundRes == 0) {
				strReply = "��˵" + strDeckName + "?" + GlobalMsg["strDeckNotFound"];
				reply(strReply);
				return 1;
			}
			vector<string> TempDeck(CardDeck::mPublicDeck[strDeckName]);
			strReply = format(GlobalMsg["strDrawCard"], { strNickName , CardDeck::drawCard(TempDeck) });
			while (--intCardNum && TempDeck.size()) {
				strReply += "\n" + CardDeck::drawCard(TempDeck);
				if (strReply.length() > 1000) {
					reply(strReply);
					strReply.clear();
				}
			}
			reply(strReply);
			if (intCardNum) {
				reply(GlobalMsg["strDeckEmpty"]);
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
				if (ilInitList->clear(fromGroup))
					strReply = "�ɹ�����ȹ���¼��";
				else
					strReply = "�б�Ϊ�գ�";
				reply(strReply);
				return 1;
			}
			ilInitList->show(fromGroup, strReply);
			reply(strReply);
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
		{
			if (boolDisabledJrrpGlobal) {
				reply(GlobalMsg["strDisabledJrrpGlobal"]);
				return 1;
			}
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			const string Command = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
			if (intT == GroupT) {
				if (Command == "on")
				{
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (DisabledJRRPGroup.count(fromGroup))
						{
							DisabledJRRPGroup.erase(fromGroup);
							reply("�ɹ��ڱ�Ⱥ������JRRP!");
						}
						else
						{
							reply("�ڱ�Ⱥ��JRRPû�б�����!");
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
					return 1;
				}
				if (Command == "off")
				{
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (!DisabledJRRPGroup.count(fromGroup))
						{
							DisabledJRRPGroup.insert(fromGroup);
							reply("�ɹ��ڱ�Ⱥ�н���JRRP!");
						}
						else
						{
							reply("�ڱ�Ⱥ��JRRPû�б�����!");
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
					return 1;
				}
				if (DisabledJRRPGroup.count(fromGroup))
				{
					reply("�ڱ�Ⱥ��JRRP�����ѱ�����");
					return 1;
				}
			}
			else if (intT == DiscussT) {
				if (Command == "on")
				{
					if (DisabledJRRPDiscuss.count(fromGroup))
					{
						DisabledJRRPDiscuss.erase(fromGroup);
						reply("�ɹ��ڴ˶�������������JRRP!");
					}
					else
					{
						reply("�ڴ˶���������JRRPû�б�����!");
					}
					return 1;
				}
				if (Command == "off")
				{
					if (!DisabledJRRPDiscuss.count(fromGroup))
					{
						DisabledJRRPDiscuss.insert(fromGroup);
						reply("�ɹ��ڴ˶��������н���JRRP!");
					}
					else
					{
						reply("�ڴ˶���������JRRPû�б�����!");
					}
					return 1;
				}
				if (DisabledJRRPDiscuss.count(fromGroup))
				{
					reply("�ڴ˶���������JRRP�ѱ�����!");
					return 1;
				}
			}
			string des;
			string data = "QQ=" + to_string(CQ::getLoginQQ()) + "&v=20190114" + "&QueryQQ=" + to_string(fromQQ);
			char *frmdata = new char[data.length() + 1];
			strcpy_s(frmdata, data.length() + 1, data.c_str());
			bool res = Network::POST("api.kokona.tech", "/jrrp", 5555, frmdata, des);
			delete[] frmdata;
			if (res)
			{
				reply(format(GlobalMsg["strJrrp"], { strNickName, des }));
			}
			else
			{
				reply(format(GlobalMsg["strJrrpErr"], { des }));
			}
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
		{
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
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

			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
				intMsgCnt++;

			string strNum;
			while (isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
			{
				strNum += strMsg[intMsgCnt];
				intMsgCnt++;
			}
			if (strNum.size() > 2)
			{
				reply(GlobalMsg["strNameNumTooBig"]);
				return 1;
			}
			int intNum = stoi(strNum.empty() ? "1" : strNum);
			if (intNum > 10)
			{
				reply(GlobalMsg["strNameNumTooBig"]);
				return 1;
			}
			if (intNum == 0)
			{
				reply(GlobalMsg["strNameNumCannotBeZero"]);
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
			string strReply = format(GlobalMsg["strNameGenerator"], { strNickName }) + "\n";
			for (auto i = 0; i != TempNameStorage.size(); i++)
			{
				strReply.append(TempNameStorage[i]);
				if (i != TempNameStorage.size() - 1)strReply.append(", ");
			}
			reply(strReply);
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
				reply(GlobalMsg["strCharacterTooBig"]);
				return 1;
			}
			const int intNum = stoi(strNum.empty() ? "1" : strNum);
			if (intNum > 10)
			{
				reply(GlobalMsg["strCharacterTooBig"]);
				return 1;
			}
			if (intNum == 0)
			{
				reply(GlobalMsg["strCharacterCannotBeZero"]);
				return 1;
			}
			string strReply = strNickName;
			COC7(strReply, intNum);
			reply(strReply);
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
				reply(GlobalMsg["strCharacterTooBig"]);
				return 1;
			}
			const int intNum = stoi(strNum.empty() ? "1" : strNum);
			if (intNum > 10)
			{
				reply(GlobalMsg["strCharacterTooBig"]);
				return 1;
			}
			if (intNum == 0)
			{
				reply(GlobalMsg["strCharacterCannotBeZero"]);
				return 1;
			}
			string strReply = strNickName;
			DND(strReply, intNum);
			reply(strReply);
		}
		else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn")
		{
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
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
			Name->set(fromGroup, fromQQ, name);
			const string strReply = format(GlobalMsg["strNameSet"], { strNickName, strip(name) });
			reply(strReply);
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
					reply(GlobalMsg["strSetInvalid"]);
					return 1;
				}
			if (strDefaultDice.length() > 3 && strDefaultDice != "1000")
			{
				reply(GlobalMsg["strSetTooBig"]);
				return 1;
			}
			const int intDefaultDice = stoi(strDefaultDice);
			if (intDefaultDice == 100)
				DefaultDice.erase(fromQQ);
			else
				DefaultDice[fromQQ] = intDefaultDice;
			const string strSetSuccessReply = "�ѽ�" + strNickName + "��Ĭ�������͸���ΪD" + strDefaultDice;
			reply(strSetSuccessReply);
		}
		else if (strLowerMessage.substr(intMsgCnt, 3) == "str"&&boolMasterMode&&fromQQ == masterQQ) {
			string strName;
			while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length())
			{
				strName += strMsg[intMsgCnt];
				intMsgCnt++;
			}
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
				intMsgCnt++;
			if (intMsgCnt == strMsg.length()) {
				EditedMsg.erase(strName);
				reply("�����" + strName + "���Զ��壬���ָ�Ĭ��������Ҫ����Ӧ�á�");
			}
			else if (GlobalMsg.count(strName)) {
				string strMessage = strMsg.substr(intMsgCnt);
				EditedMsg[strName] = strMessage;
				GlobalMsg[strName] = (strName == "strHlpMsg") ? Dice_Short_Ver + "\n" + strMsg : strMessage;
				reply("�Ѽ���" + strName + "���Զ���");
			}
			else {
				reply("��˵" + strName + "�����ƺ����ǻ��õ�������");
			}

		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
		{
			intMsgCnt += 2;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strMsg.length() && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])) && !isspace(static_cast<unsigned char>(strMsg[intMsgCnt]))
				)
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
				intMsgCnt++;
			string strCurrentValue;
			while (isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
			{
				strCurrentValue += strMsg[intMsgCnt];
				intMsgCnt++;
			}
			int intCurrentVal;
			if (strCurrentValue.empty())
			{
				if (CharacterProp.count(SourceType(fromQQ, intT, fromGroup)) && CharacterProp[SourceType(
					fromQQ, intT, fromGroup)].count(strSkillName))
				{
					intCurrentVal = CharacterProp[SourceType(fromQQ, intT, fromGroup)][strSkillName];
				}
				else if (SkillDefaultVal.count(strSkillName))
				{
					intCurrentVal = SkillDefaultVal[strSkillName];
				}
				else
				{
					reply(GlobalMsg["strEnValInvalid"]);
					return 1;
				}
			}
			else
			{
				if (strCurrentValue.length() > 3)
				{
					reply(GlobalMsg["strEnValInvalid"]);

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
					CharacterProp[SourceType(fromQQ, intT, fromGroup)][strSkillName] = intCurrentVal +
						intTmpRollD10;
				}
			}
			reply(strAns);
		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
		{
			string strAns = strNickName + "�ķ����-�ܽ�֢״:\n";
			LongInsane(strAns);
			reply(strAns);
		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
		{
			if (boolDisabledMeGlobal)
			{
				reply(GlobalMsg["strDisabledMeGlobal"]);
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
				string strAction = strip(strMsg.substr(intMsgCnt));

				for (auto i : strGroupID)
				{
					if (!isdigit(static_cast<unsigned char>(i)))
					{
						reply(GlobalMsg["strGroupIDInvalid"]);
						return 1;
					}
				}
				if (strGroupID.empty())
				{
					reply(GlobalMsg["strGroupIDEmpty"]);
					return 1;
				}
				if (strAction.empty())
				{
					reply(GlobalMsg["strActionEmpty"]);
					return 1;
				}
				const long long llGroupID = stoll(strGroupID);
				if (DisabledGroup.count(llGroupID))
				{
					reply(GlobalMsg["strDisabledErr"]);
					return 1;
				}
				if (DisabledMEGroup.count(llGroupID))
				{
					reply(GlobalMsg["strMEDisabledErr"]);
					return 1;
				}
				string strReply = getName(fromQQ, llGroupID) + strAction;
				const int intSendRes = sendGroupMsg(llGroupID, strReply);
				if (intSendRes < 0)
				{
					reply(GlobalMsg["strSendErr"]);
				}
				else
				{
					reply(GlobalMsg["strSendSuccess"]);
				}
				return 1;
			}
			string strAction = strLowerMessage.substr(intMsgCnt);
			if (intT == GroupT) {
				if (strAction == "on")
				{
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (DisabledMEGroup.count(fromGroup))
						{
							DisabledMEGroup.erase(fromGroup);
							reply(GlobalMsg["strMeOn"]);
						}
						else
						{
							reply(GlobalMsg["strMeOnAlready"]);
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
					return 1;
				}
				if (strAction == "off")
				{
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (!DisabledMEGroup.count(fromGroup))
						{
							DisabledMEGroup.insert(fromGroup);
							reply(GlobalMsg["strMeOff"]);
						}
						else
						{
							reply(GlobalMsg["strMeOffAlready"]);
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
					return 1;
				}
				if (DisabledMEGroup.count(fromGroup))
				{
					reply(GlobalMsg["strMEDisabledErr"]);
					return 1;
				}
			}
			else if (intT == DiscussT) {
				if (strAction == "on")
				{
					if (DisabledMEDiscuss.count(fromGroup))
					{
						DisabledMEDiscuss.erase(fromGroup);
						reply(GlobalMsg["strMeOn"]);
					}
					else
					{
						reply(GlobalMsg["strMeOnAlready"]);
					}
					return 1;
				}
				if (strAction == "off")
				{
					if (!DisabledMEDiscuss.count(fromGroup))
					{
						DisabledMEDiscuss.insert(fromGroup);
						reply(GlobalMsg["strMeOff"]);
					}
					else
					{
						reply(GlobalMsg["strMeOffAlready"]);
					}
					return 1;
				}
				if (DisabledMEDiscuss.count(fromGroup))
				{
					reply(GlobalMsg["strMEDisabledErr"]);
					return 1;
				}
			}
			strAction = strip(strMsg.substr(intMsgCnt));
			if (strAction.empty())
			{
				reply(GlobalMsg["strActionEmpty"]);
				return 1;
			}
			const string strReply = strNickName + strAction;
			reply(strReply);
		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
		{
			intMsgCnt += 2;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
				intMsgCnt++;
			string name = strip(strMsg.substr(intMsgCnt));
			if (name.length() > 50)
			{
				reply(GlobalMsg["strNameTooLongErr"]);
				return 1;
			}
			if (!name.empty())
			{
				Name->set(fromGroup, fromQQ, name);
				const string strReply = format(GlobalMsg["strNameSet"], { strNickName, strip(name) });
				reply(strReply);
			}
			else
			{
				if (Name->del(fromGroup, fromQQ))
				{
					const string strReply = format(GlobalMsg["strNameClr"], { strNickName });
					reply(strReply);
				}
				else
				{
					const string strReply = strNickName + GlobalMsg["strNameDelErr"];
					reply(strReply);
				}
			}
		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "ob")
		{
			if (intT == PrivateT) {
				reply(GlobalMsg["strObPrivate"]);
				return 1;
			}
			intMsgCnt += 2;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			const string Command = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
			if (Command == "on")
			{
				if (intT == GroupT) {
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (DisabledOBGroup.count(fromGroup))
						{
							DisabledOBGroup.erase(fromGroup);
							reply(GlobalMsg["strObOn"]);
						}
						else
						{
							reply(GlobalMsg["strObOnAlready"]);
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
					return 1;
				}
				else {
					if (DisabledOBDiscuss.count(fromGroup))
					{
						DisabledOBDiscuss.erase(fromGroup);
						reply(GlobalMsg["strObOn"]);
					}
					else
					{
						reply(GlobalMsg["strObOnAlready"]);
					}
					return 1;
				}
			}
			if (Command == "off")
			{
				if (intT == Group) {
					if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
					{
						if (!DisabledOBGroup.count(fromGroup))
						{
							DisabledOBGroup.insert(fromGroup);
							ObserveGroup.clear();
							reply(GlobalMsg["strObOff"]);
						}
						else
						{
							reply(GlobalMsg["strObOffAlready"]);
						}
					}
					else
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
					}
					return 1;
				}
				else {
					if (!DisabledOBDiscuss.count(fromGroup))
					{
						DisabledOBDiscuss.insert(fromGroup);
						ObserveDiscuss.clear();
						reply(GlobalMsg["strObOff"]);
					}
					else
					{
						reply(GlobalMsg["strObOffAlready"]);
					}
					return 1;
				}
			}
			if (intT == GroupT && DisabledOBGroup.count(fromGroup))
			{
				reply(GlobalMsg["strObOffAlready"]);
				return 1;
			}
			if (intT == DiscussT && DisabledOBDiscuss.count(fromGroup))
			{
				reply(GlobalMsg["strObOffAlready"]);
				return 1;
			}
			if (Command == "list")
			{
				string Msg = GlobalMsg["strObList"];
				const auto Range = (intT == GroupT) ? ObserveGroup.equal_range(fromGroup) : ObserveDiscuss.equal_range(fromGroup);
				for (auto it = Range.first; it != Range.second; ++it)
				{
					Msg += "\n" + getName(it->second, fromGroup) + "(" + to_string(it->second) + ")";
				}
				const string strReply = Msg == GlobalMsg["strObList"] ? GlobalMsg["strObListEmpty"] : Msg;
				reply(strReply);
			}
			else if (Command == "clr")
			{
				if (intT = DiscussT) {
					ObserveDiscuss.erase(fromGroup);
					reply(GlobalMsg["strObListClr"]);
				}
				else if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
				{
					ObserveGroup.erase(fromGroup);
					reply(GlobalMsg["strObListClr"]);
				}
				else
				{
					reply(GlobalMsg["strPermissionDeniedErr"]);
				}
			}
			else if (Command == "exit")
			{
				const auto Range = (intT == GroupT) ? ObserveGroup.equal_range(fromGroup) : ObserveDiscuss.equal_range(fromGroup);
				for (auto it = Range.first; it != Range.second; ++it)
				{
					if (it->second == fromQQ)
					{
						(intT == GroupT) ? ObserveGroup.erase(it) : ObserveDiscuss.erase(it);
						const string strReply = strNickName + GlobalMsg["strObExit"];
						reply(strReply);
						return 1;
					}
				}
				const string strReply = strNickName + GlobalMsg["strObExitAlready"];
				reply(strReply);
			}
			else
			{
				const auto Range = (intT == GroupT) ? ObserveGroup.equal_range(fromGroup) : ObserveDiscuss.equal_range(fromGroup);
				for (auto it = Range.first; it != Range.second; ++it)
				{
					if (it->second == fromQQ)
					{
						const string strReply = strNickName + GlobalMsg["strObEnterAlready"];
						reply(strReply);
						return 1;
					}
				}
				(intT == GroupT) ? ObserveGroup.insert(make_pair(fromGroup, fromQQ)) : ObserveDiscuss.insert(make_pair(fromGroup, fromQQ));
				const string strReply = strNickName + GlobalMsg["strObEnter"];
				reply(strReply);
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
			string strReason = strMsg.substr(intMsgCnt);
			int intSkillVal;
			if (strSkillVal.empty())
			{
				if (CharacterProp.count(SourceType(fromQQ, intT, fromGroup)) && CharacterProp[SourceType(
					fromQQ, intT, fromGroup)].count(strSkillName))
				{
					intSkillVal = CharacterProp[SourceType(fromQQ, intT, fromGroup)][strSkillName];
				}
				else if (SkillDefaultVal.count(strSkillName))
				{
					intSkillVal = SkillDefaultVal[strSkillName];
				}
				else
				{
					reply(GlobalMsg["strUnknownPropErr"]);
					return 1;
				}
			}
			else if (strSkillVal.length() > 3)
			{
				reply(GlobalMsg["strPropErr"]);
				return 1;
			}
			else
			{
				intSkillVal = stoi(strSkillVal);
			}
			int intFianlSkillVal = intSkillVal + intSkillModify;
			if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
			{
				reply(GlobalMsg["strSuccessRateErr"]);
				return 1;
			}
			RD rdMainDice(strMainDice);
			const int intFirstTimeRes = rdMainDice.Roll();
			if (intFirstTimeRes == ZeroDice_Err)
			{
				reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err)
			{
				reply(GlobalMsg["strDiceTooBigErr"]);
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
			reply(strReply);
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
			string strReason = strMsg.substr(intMsgCnt);
			int intSkillVal;
			if (strSkillVal.empty())
			{
				if (CharacterProp.count(SourceType(fromQQ, intT, fromGroup)) && CharacterProp[SourceType(
					fromQQ, intT, fromGroup)].count(strSkillName))
				{
					intSkillVal = CharacterProp[SourceType(fromQQ, intT, fromGroup)][strSkillName];
				}
				else if (SkillDefaultVal.count(strSkillName))
				{
					intSkillVal = SkillDefaultVal[strSkillName];
				}
				else
				{
					reply(GlobalMsg["strUnknownPropErr"]);
					return 1;
				}
			}
			else if (strSkillVal.length() > 3)
			{
				reply(GlobalMsg["strPropErr"]);
				return 1;
			}
			else
			{
				intSkillVal = stoi(strSkillVal);
			}
			int intFianlSkillVal = intSkillVal + intSkillModify;
			if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
			{
				reply(GlobalMsg["strSuccessRateErr"]);
				return 1;
			}
			RD rdMainDice(strMainDice);
			const int intFirstTimeRes = rdMainDice.Roll();
			if (intFirstTimeRes == ZeroDice_Err)
			{
				reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err)
			{
				reply(GlobalMsg["strDiceTooBigErr"]);
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
			reply(strReply);
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
			string strname = strMsg.substr(intMsgCnt);
			if (strname.empty())
				strname = strNickName;
			else
				strname = strip(strname);
			RD initdice(strinit);
			const int intFirstTimeRes = initdice.Roll();
			if (intFirstTimeRes == Value_Err)
			{
				reply(GlobalMsg["strValueErr"]);
				return 1;
			}
			if (intFirstTimeRes == Input_Err)
			{
				reply(GlobalMsg["strInputErr"]);
				return 1;
			}
			if (intFirstTimeRes == ZeroDice_Err)
			{
				reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			}
			if (intFirstTimeRes == ZeroType_Err)
			{
				reply(GlobalMsg["strZeroTypeErr"]);
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err)
			{
				reply(GlobalMsg["strDiceTooBigErr"]);
				return 1;
			}
			if (intFirstTimeRes == TypeTooBig_Err)
			{
				reply(GlobalMsg["strTypeTooBigErr"]);
				return 1;
			}
			if (intFirstTimeRes == AddDiceVal_Err)
			{
				reply(GlobalMsg["strAddDiceValErr"]);
				return 1;
			}
			if (intFirstTimeRes != 0)
			{
				reply(GlobalMsg["strUnknownErr"]);
				return 1;
			}
			ilInitList->insert(fromGroup, initdice.intTotal, strname);
			const string strReply = strname + "���ȹ����㣺" + strinit + '=' + to_string(initdice.intTotal);
			reply(strReply);
		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
		{
			intMsgCnt += 2;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string SanCost = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
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
				reply(GlobalMsg["strSCInvalid"]);
				return 1;
			}
			if (San.empty() && !(CharacterProp.count(SourceType(fromQQ, intT, fromGroup)) && CharacterProp[
				SourceType(fromQQ, intT, fromGroup)].count("����")))
			{
				reply(GlobalMsg["strSanInvalid"]);
				return 1;
			}
				for (const auto& character : SanCost.substr(0, SanCost.find("/")))
				{
					if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
					{
						reply(GlobalMsg["strSCInvalid"]);
						return 1;
					}
				}
				for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
				{
					if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
					{
						reply(GlobalMsg["strSCInvalid"]);
						return 1;
					}
				}
				RD rdSuc(SanCost.substr(0, SanCost.find("/")));
				RD rdFail(SanCost.substr(SanCost.find("/") + 1));
				if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
				{
					reply(GlobalMsg["strSCInvalid"]);
					return 1;
				}
				if (San.length() >= 3)
				{
					reply(GlobalMsg["strSanInvalid"]);
					return 1;
				}
				const int intSan = San.empty() ? CharacterProp[SourceType(fromQQ, intT, fromGroup)]["����"] : stoi(San);
				if (intSan == 0)
				{
					reply(GlobalMsg["strSanInvalid"]);
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
						CharacterProp[SourceType(fromQQ, intT, fromGroup)]["����"] = max(0, intSan - rdSuc.intTotal);
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
						CharacterProp[SourceType(fromQQ, intT, fromGroup)]["����"] = max(0, intSan - rdFail.intTotal);
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
						CharacterProp[SourceType(fromQQ, intT, fromGroup)]["����"] = max(0, intSan - rdFail.intTotal);
					}
				}
				reply(strAns);
		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
		{
			intMsgCnt += 2;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			if (intMsgCnt == strLowerMessage.length())
			{
				reply(GlobalMsg["strStErr"]);
				return 1;
			}
			if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
			{
				if (CharacterProp.count(SourceType(fromQQ, intT, fromGroup)))
				{
					CharacterProp.erase(SourceType(fromQQ, intT, fromGroup));
				}
				reply(GlobalMsg["strPropCleared"]);
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
					if (CharacterProp.count(SourceType(fromQQ, intT, fromGroup)) && CharacterProp[SourceType(
						fromQQ, intT, fromGroup)].count(strSkillName))
					{
						CharacterProp[SourceType(fromQQ, intT, fromGroup)].erase(strSkillName);
						reply(GlobalMsg["strPropDeleted"]);
					}
					else
					{
						reply(GlobalMsg["strPropNotFound"]);
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
					SourceType sCharProp = SourceType(fromQQ, intT, fromGroup);
					if (strSkillName.empty() && CharacterProp.count(sCharProp)) {
						string strReply = strNickName + "�������б�";
						for (auto each : CharacterProp[sCharProp]) {
							strReply += " " + each.first + ":" + to_string(each.second);
						}
						reply(strReply);
						return 1;
					}
					if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
					if (CharacterProp.count(SourceType(fromQQ, intT, fromGroup)) && CharacterProp[SourceType(
						fromQQ, intT, fromGroup)].count(strSkillName))
					{
						reply(format(GlobalMsg["strProp"], {
							strNickName, strSkillName,
							to_string(CharacterProp[SourceType(fromQQ, intT, fromGroup)][strSkillName])
							}));
					}
					else if (SkillDefaultVal.count(strSkillName))
					{
						reply(format(GlobalMsg["strProp"], { strNickName, strSkillName, to_string(SkillDefaultVal[strSkillName]) }));
					}
					else
					{
						reply(GlobalMsg["strPropNotFound"]);
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
				CharacterProp[SourceType(fromQQ, GroupT, fromGroup)][strSkillName] = stoi(strSkillVal);
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
			}
			if (boolError)
			{
				reply(GlobalMsg["strPropErr"]);
			}
			else
			{
				reply(GlobalMsg["strSetPropSuccess"]);
			}
		}
		else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
		{
			string strAns = strNickName + "�ķ����-��ʱ֢״:\n";
			TempInsane(strAns);
			reply(strAns);
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
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
				intMsgCnt++;

			int intTmpMsgCnt;
			for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != strMsg.length() && strMsg[intTmpMsgCnt] != ' ';
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
			while (isspace(static_cast<unsigned char>(strMsg[intTmpMsgCnt])))
				intTmpMsgCnt++;
			string strReason = strMsg.substr(intTmpMsgCnt);


			int intTurnCnt = 1;
			if (strMainDice.find("#") != string::npos)
			{
				string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
				if (strTurnCnt.empty())
					strTurnCnt = "1";
				strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
				const int intDefaultDice = DefaultDice.count(fromQQ) ? DefaultDice[fromQQ] : 100;
				RD rdTurnCnt(strTurnCnt, intDefaultDice);
				const int intRdTurnCntRes = rdTurnCnt.Roll();
				if (intRdTurnCntRes == Value_Err)
				{
					reply(GlobalMsg["strValueErr"]);
					return 1;
				}
				if (intRdTurnCntRes == Input_Err)
				{
					reply(GlobalMsg["strInputErr"]);
					return 1;
				}
				if (intRdTurnCntRes == ZeroDice_Err)
				{
					reply(GlobalMsg["strZeroDiceErr"]);
					return 1;
				}
				if (intRdTurnCntRes == ZeroType_Err)
				{
					reply(GlobalMsg["strZeroTypeErr"]);
					return 1;
				}
				if (intRdTurnCntRes == DiceTooBig_Err)
				{
					reply(GlobalMsg["strDiceTooBigErr"]);
					return 1;
				}
				if (intRdTurnCntRes == TypeTooBig_Err)
				{
					reply(GlobalMsg["strTypeTooBigErr"]);
					return 1;
				}
				if (intRdTurnCntRes == AddDiceVal_Err)
				{
					reply(GlobalMsg["strAddDiceValErr"]);
					return 1;
				}
				if (intRdTurnCntRes != 0)
				{
					reply(GlobalMsg["strUnknownErr"]);
					return 1;
				}
				if (rdTurnCnt.intTotal > 10)
				{
					reply(GlobalMsg["strRollTimeExceeded"]);
					return 1;
				}
				if (rdTurnCnt.intTotal <= 0)
				{
					reply(GlobalMsg["strRollTimeErr"]);
					return 1;
				}
				intTurnCnt = rdTurnCnt.intTotal;
				if (strTurnCnt.find("d") != string::npos)
				{
					string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
					if (!isHidden || intT == PrivateT)
					{
						reply(strTurnNotice);
					}
					else
					{
						strTurnNotice = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromGroup] : "��������(" + to_string(fromGroup)) + ")�� " + strTurnNotice;
						AddMsgToQueue(strTurnNotice, fromQQ, Private);
						const auto range = ObserveGroup.equal_range(fromGroup);
						for (auto it = range.first; it != range.second; ++it)
						{
							if (it->second != fromQQ)
							{
								AddMsgToQueue(strTurnNotice, it->second, Private);
							}
						}
					}
				}
			}
			if (strMainDice.empty())
			{
				reply(GlobalMsg["strEmptyWWDiceErr"]);
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
			const int intDefaultDice = DefaultDice.count(fromQQ) ? DefaultDice[fromQQ] : 100;
			RD rdMainDice(strMainDice, intDefaultDice);

			const int intFirstTimeRes = rdMainDice.Roll();
			if (intFirstTimeRes == Value_Err)
			{
				reply(GlobalMsg["strValueErr"]);
				return 1;
			}
			if (intFirstTimeRes == Input_Err)
			{
				reply(GlobalMsg["strInputErr"]);
				return 1;
			}
			if (intFirstTimeRes == ZeroDice_Err)
			{
				reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			}
			else
			{
				if (intFirstTimeRes == ZeroType_Err)
				{
					reply(GlobalMsg["strZeroTypeErr"]);
					return 1;
				}
				if (intFirstTimeRes == DiceTooBig_Err)
				{
					reply(GlobalMsg["strDiceTooBigErr"]);
					return 1;
				}
				if (intFirstTimeRes == TypeTooBig_Err)
				{
					reply(GlobalMsg["strTypeTooBigErr"]);
					return 1;
				}
				if (intFirstTimeRes == AddDiceVal_Err)
				{
					reply(GlobalMsg["strAddDiceValErr"]);
					return 1;
				}
				if (intFirstTimeRes != 0)
				{
					reply(GlobalMsg["strUnknownErr"]);
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
					reply(strAns);
				}
				else
				{
					strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromGroup] : "��������(" + to_string(fromGroup)) + ")�� " + strAns;
					AddMsgToQueue(strAns, fromQQ, Private);
					const auto range = ObserveGroup.equal_range(fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != fromQQ)
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
						reply(strAns);
					}
					else
					{
						strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromGroup] : "��������(" + to_string(fromGroup)) + ")�� " + strAns;
						AddMsgToQueue(strAns, fromQQ, Private);
						const auto range = ObserveGroup.equal_range(fromGroup);
						for (auto it = range.first; it != range.second; ++it)
						{
							if (it->second != fromQQ)
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
				reply(strReply);
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
			if (strMsg[intMsgCnt] == 's')
			{
				boolDetail = false;
				intMsgCnt++;
			}
			if (strLowerMessage[intMsgCnt] == 'h')
			{
				isHidden = true;
				intMsgCnt += 1;
			}
			if (intT == 0)isHidden = false;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
				intMsgCnt++;
			string strMainDice;
			string strReason;
			bool tmpContainD = false;
			int intTmpMsgCnt;
			for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != strMsg.length() && strMsg[intTmpMsgCnt] != ' ';
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
				while (isspace(static_cast<unsigned char>(strMsg[intTmpMsgCnt])))
					intTmpMsgCnt++;
				strReason = strMsg.substr(intTmpMsgCnt);
			}
			else
				strReason = strMsg.substr(intMsgCnt);

			int intTurnCnt = 1;
			if (strMainDice.find("#") != string::npos)
			{
				string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
				if (strTurnCnt.empty())
					strTurnCnt = "1";
				strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
				const int intDefaultDice = DefaultDice.count(fromQQ) ? DefaultDice[fromQQ] : 100;
				RD rdTurnCnt(strTurnCnt, intDefaultDice);
				const int intRdTurnCntRes = rdTurnCnt.Roll();
				if (intRdTurnCntRes == Value_Err)
				{
					reply(GlobalMsg["strValueErr"]);
					return 1;
				}
				if (intRdTurnCntRes == Input_Err)
				{
					reply(GlobalMsg["strInputErr"]);
					return 1;
				}
				if (intRdTurnCntRes == ZeroDice_Err)
				{
					reply(GlobalMsg["strZeroDiceErr"]);
					return 1;
				}
				if (intRdTurnCntRes == ZeroType_Err)
				{
					reply(GlobalMsg["strZeroTypeErr"]);
					return 1;
				}
				if (intRdTurnCntRes == DiceTooBig_Err)
				{
					reply(GlobalMsg["strDiceTooBigErr"]);
					return 1;
				}
				if (intRdTurnCntRes == TypeTooBig_Err)
				{
					reply(GlobalMsg["strTypeTooBigErr"]);
					return 1;
				}
				if (intRdTurnCntRes == AddDiceVal_Err)
				{
					reply(GlobalMsg["strAddDiceValErr"]);
					return 1;
				}
				if (intRdTurnCntRes != 0)
				{
					reply(GlobalMsg["strUnknownErr"]);
					return 1;
				}
				if (rdTurnCnt.intTotal > 10)
				{
					reply(GlobalMsg["strRollTimeExceeded"]);
					return 1;
				}
				if (rdTurnCnt.intTotal <= 0)
				{
					reply(GlobalMsg["strRollTimeErr"]);
					return 1;
				}
				intTurnCnt = rdTurnCnt.intTotal;
				if (strTurnCnt.find("d") != string::npos)
				{
					string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
					if (!isHidden)
					{
						reply(strTurnNotice);
					}
					else
					{
						strTurnNotice = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromGroup] : "��������(" + to_string(fromGroup)) + ")�� " + strTurnNotice;
						AddMsgToQueue(strTurnNotice, fromQQ, Private);
						const auto range = ObserveGroup.equal_range(fromGroup);
						for (auto it = range.first; it != range.second; ++it)
						{
							if (it->second != fromQQ)
							{
								AddMsgToQueue(strTurnNotice, it->second, Private);
							}
						}
					}
				}
			}
			const int intDefaultDice = DefaultDice.count(fromQQ) ? DefaultDice[fromQQ] : 100;
			RD rdMainDice(strMainDice, intDefaultDice);

			const int intFirstTimeRes = rdMainDice.Roll();
			if (intFirstTimeRes == Value_Err)
			{
				reply(GlobalMsg["strValueErr"]);
				return 1;
			}
			if (intFirstTimeRes == Input_Err)
			{
				reply(GlobalMsg["strInputErr"]);
				return 1;
			}
			if (intFirstTimeRes == ZeroDice_Err)
			{
				reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			}
			if (intFirstTimeRes == ZeroType_Err)
			{
				reply(GlobalMsg["strZeroTypeErr"]);
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err)
			{
				reply(GlobalMsg["strDiceTooBigErr"]);
				return 1;
			}
			if (intFirstTimeRes == TypeTooBig_Err)
			{
				reply(GlobalMsg["strTypeTooBigErr"]);
				return 1;
			}
			if (intFirstTimeRes == AddDiceVal_Err)
			{
				reply(GlobalMsg["strAddDiceValErr"]);
				return 1;
			}
			if (intFirstTimeRes != 0)
			{
				reply(GlobalMsg["strUnknownErr"]);
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
					reply(strAns);
				}
				else
				{
					strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromGroup] : "��������(" + to_string(fromGroup)) + ")�� " + strAns;
					AddMsgToQueue(strAns, fromQQ, Private);
					const auto range = ObserveGroup.equal_range(fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != fromQQ)
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
						reply(strAns);
					}
					else
					{
						strAns = (intT == GroupT ? "��Ⱥ\"" + getGroupList()[fromGroup] : "��������(" + to_string(fromGroup)) + ")�� " + strAns;
						AddMsgToQueue(strAns, fromQQ, Private);
						const auto range = ObserveGroup.equal_range(fromGroup);
						for (auto it = range.first; it != range.second; ++it)
						{
							if (it->second != fromQQ)
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
				reply(strReply);
			}
		}
		return 0;
	}
	//�ж��Ƿ���Ӧ
	bool DiceFilter() {
		init(strMsg);
		while (isspace(static_cast<unsigned char>(strMsg[0])))
			strMsg.erase(strMsg.begin());
		string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
		if (strMsg.substr(0, 6) == "[CQ:at")
		{
			if (strMsg.substr(0, strAt.length()) == strAt)
			{
				strMsg = strMsg.substr(strAt.length());
				isCalled = true;
			}
			else
			{
				return false;
			}
		}
		if (fromQQ == masterQQ) isMaster = true;
		init2(strMsg);
		return DiceReply();
	}


private:
	int intMsgCnt = 0;
	bool isCalled = false;
	bool isMaster = false;
};
#endif /*DICE_EVENT*/
