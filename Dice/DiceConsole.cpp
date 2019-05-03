/*
 * _______   ________  ________  ________  __
 * |  __ \  |__  __| |  _____| |  _____| | |
 * | | | |   | |   | |    | |_____  | |
 * | | | |   | |   | |    |  _____| |__|
 * | |__| |  __| |__  | |_____  | |_____  __
 * |_______/  |________| |________| |________| |__|
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
#pragma once
#include "CQEVE_ALL.h"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "DiceMsgSend.h"
#include "MsgFormat.h"

using namespace std;
using namespace CQ;

	long long masterQQ = 0;
	//ȫ�־�Ĭ
	bool boolDisabledGlobal = false;
	//ȫ�ֽ���.me
	bool boolDisabledMeGlobal = false;
	//ȫ�ֽ���.jrrp
	bool boolDisabledJrrpGlobal = false;
	//˽��ģʽ
	bool boolPreserve = false;
	//����������
	bool boolNoDiscuss = false;
	//��������Ϣ��¼
	std::map<long long, time_t> DiscussList;
	//���Ի����
	std::map<std::string, std::string> PersonalMsg;
	//botoff��Ⱥ
	std::set<long long> DisabledGroup;
	//botoff��������
	std::set<long long> DisabledDiscuss;
	//������Ⱥ��˽��ģʽ����
	std::set<long long> WhiteGroup;
	//������Ⱥ������������
	std::set<long long> BlackGroup;
	//�������û�������˽������
	std::set<long long> WhiteQQ;
	//�������û�������������
	std::set<long long> BlackQQ;
	//��ǰʱ��
	SYSTEMTIME stNow = { 0 };
	SYSTEMTIME stTmp = { 0 };
	//�ϰ�ʱ��
	std::pair<int, int> ClockToWork = { 24,0 };
	//�°�ʱ��
	std::pair<int, int> ClockOffWork = { 24,0 };

	string transClock(std::pair<int, int> clock) {
		string strClock=to_string(clock.first);
		strClock += ":";
		if(clock.second<10)strClock += "0";
		strClock += to_string(clock.second);
		return strClock;
	}

	//���׼�ʱ��
	void ConsoleTimer() {
		while (Enabled) {
			GetLocalTime(&stNow);
			//����ʱ��䶯
			if (stTmp.wMinute != stNow.wMinute) {
				stTmp = stNow;
				if (stNow.wHour == ClockOffWork.first&&stNow.wMinute == ClockOffWork.second && !boolDisabledGlobal) {
					boolDisabledGlobal = true;
					AddMsgToQueue(GlobalMsg["strSelfName"] + GlobalMsg["strClockOffWork"], masterQQ);
				}
				if (stNow.wHour == ClockToWork.first&&stNow.wMinute == ClockToWork.second&&boolDisabledGlobal) {
					boolDisabledGlobal = false;
					AddMsgToQueue(GlobalMsg["strSelfName"] + GlobalMsg["strClockToWork"], masterQQ);
				}
			}
			Sleep(10000);
		}
	}

	//һ������
	int clearGroup(string strPara) {
		int intCnt=0;
		map<long long,string> GroupList=getGroupList();
		if (strPara == "unpower" || strPara.empty()) {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions == 1) {
					AddMsgToQueue(GlobalMsg["strGroupClr"], eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
		}
		else if (isdigit(strPara[0])) {
			time_t tNow(NULL);
			time_t tLim= tNow - stoi(strPara) * 24 * 60 * 60;
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, getLoginQQ()).LastMsgTime < tLim) {
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSlefName"], strPara }), eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
			for (auto eachDiscuss : DiscussList) {
				if (eachDiscuss.second < tLim) {
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSlefName"], strPara }), eachDiscuss.first, Group);
					Sleep(10);
					setDiscussLeave(eachDiscuss.first);
					intCnt++;
				}
			}
		}
		else if (strPara == "preserve") {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, masterQQ).QQID != masterQQ&&WhiteGroup.count(eachGroup.first)==0) {
					AddMsgToQueue(GlobalMsg["strPreserve"], eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
		}
		else
			AddMsgToQueue("�޷�ʶ��ɸѡ������",masterQQ);
		return intCnt;
	}
void ConsoleHandler(std::string strMessage) {
	int intMsgCnt = 0;
	std::string strOption;
	while (!isspace(static_cast<unsigned char>(strMessage[intMsgCnt])) && intMsgCnt != strMessage.length() && !isdigit(static_cast<unsigned char>(strMessage[intMsgCnt])))
	{
		strOption += strMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (strOption.empty()) {
		AddMsgToQueue("��ʲô��ô�� Master��", masterQQ);
		return;
	}
	if (strOption == "delete") {
		AddMsgToQueue("�㲻����" + GlobalMsg["strSelfName"] + "��Master��", masterQQ);
		masterQQ = 0;
	}
	else if (strOption == "state") {
		string strReply= getLoginNick();
		strReply = strReply + "�ĵ�ǰ���" + "\n"
			+ (ClockToWork.first < 24 ? "��ʱ����" + transClock(ClockToWork) + "\n" : "")
			+ (ClockOffWork.first < 24 ? "��ʱ�ر�" + transClock(ClockOffWork) + "\n" : "")
			+ (boolPreserve ? "˽��ģʽ" : "����ģʽ") + "\n"
			+ (boolNoDiscuss ? "����������" : "����������") + "\n"
			+ "ȫ�ֿ��أ�" + (boolDisabledGlobal ? "����" : "����") + "\n"
			+ "ȫ��.me���أ�" + (boolDisabledMeGlobal ? "����" : "����") + "\n"
			+ "ȫ��.jrrp���أ�" + (boolDisabledJrrpGlobal ? "����" : "����") + "\n"
			+ "����Ⱥ������" + to_string(getGroupList().size()) + "\n"
			+ (DiscussList.size() ? "�м�¼������������" + to_string(DiscussList.size()) + "\n" : "")
			+ "�������û�����" + to_string(BlackQQ.size()) + "\n"
			+ "������Ⱥ����" + to_string(BlackGroup.size()) + "\n"
			+ "�������û�����" + to_string(WhiteQQ.size()) + "\n"
			+ "������Ⱥ����" + to_string(WhiteGroup.size());
		AddMsgToQueue(strReply, masterQQ);
	}
	else if (strOption == "clock") {
		string strReply = "����ʱ��" + to_string(stNow.wHour) + ":" + to_string(stNow.wMinute);
		AddMsgToQueue(strReply, masterQQ);
	}
	else {
		while (isspace(static_cast<unsigned char>(strMessage[intMsgCnt])))intMsgCnt++;
		if (strOption == "addgroup") {
			std::string strPersonalMsg = strMessage.substr(intMsgCnt);
			if (strPersonalMsg.empty()) {
				if (GlobalMsg.count("strAddGroup")) {
					GlobalMsg["strAddGroup"].clear();
					AddMsgToQueue("��Ⱥ�����������", masterQQ);
				}
				else AddMsgToQueue("��ǰδ������Ⱥ���ԡ�", masterQQ);
			}
			else {
				GlobalMsg["strAddGroup"] = strPersonalMsg;
				AddMsgToQueue("��Ⱥ������׼�����ˡ�", masterQQ);
			}
		}
		else if (strOption == "bot") {
			std::string strPersonalMsg = strMessage.substr(intMsgCnt);
			if (strPersonalMsg.empty()) {
				if (GlobalMsg.count("strBotMsg")) {
					GlobalMsg["strBotMsg"].clear();
					AddMsgToQueue("�����bot������Ϣ��", masterQQ);
				}
				else AddMsgToQueue("��ǰδ����bot������Ϣ��", masterQQ);
			}
			else {
				GlobalMsg["strBotMsg"] = strPersonalMsg;
				AddMsgToQueue("��Ϊbot������Ϣ��", masterQQ);
			}
		}
		else if (strOption == "on") {
			if (boolDisabledGlobal) {
				boolDisabledGlobal = false;
				AddMsgToQueue("�����ѽ�����Ĭ��", masterQQ);
			}
			else {
				AddMsgToQueue("���ﲻ�ھ�Ĭ�У�", masterQQ);
			}
		}
		else if (strOption == "off") {
			if (boolDisabledGlobal) {
				AddMsgToQueue("�����Ѿ���Ĭ��", masterQQ);
			}
			else {
				boolDisabledGlobal = true;
				AddMsgToQueue("���￪ʼ��Ĭ��", masterQQ);
			}
		}
		else if (strOption == "meon") {
			if (boolDisabledMeGlobal) {
				boolDisabledMeGlobal = false;
				AddMsgToQueue("����������.me��", masterQQ);
			}
			else {
				AddMsgToQueue("����������.me��", masterQQ);
			}
		}
		else if (strOption == "meoff") {
			if (boolDisabledMeGlobal) {
				AddMsgToQueue("�����ѽ���.me��", masterQQ);
			}
			else {
				boolDisabledMeGlobal = true;
				AddMsgToQueue("�����ѽ���.me��", masterQQ);
			}
		}
		else if (strOption == "jrrpon") {
			if (boolDisabledMeGlobal) {
				boolDisabledMeGlobal = false;
				AddMsgToQueue("����������.jrrp��", masterQQ);
			}
			else {
				AddMsgToQueue("����������.jrrp��", masterQQ);
			}
		}
		else if (strOption == "jrrpoff") {
			if (boolDisabledMeGlobal) {
				AddMsgToQueue("�����ѽ���.jrrp��", masterQQ);
			}
			else {
				boolDisabledMeGlobal = true;
				AddMsgToQueue("�����ѽ���.jrrp��", masterQQ);
			}
		}
		else if (strOption == "discusson") {
			if (boolNoDiscuss) {
				boolNoDiscuss = false;
				AddMsgToQueue("�����ѹر��������Զ��˳���", masterQQ);
			}
			else {
				AddMsgToQueue("�����ѹر��������Զ��˳���", masterQQ);
			}
		}
		else if (strOption == "discussoff") {
			if (boolNoDiscuss) {
				AddMsgToQueue("�����ѿ����������Զ��˳���", masterQQ);
			}
			else {
				boolNoDiscuss = true;
				AddMsgToQueue("�����ѿ����������Զ��˳���", masterQQ);
			}
		}
		else if (strOption == "groupclr") {
			std::string strPara = strMessage.substr(intMsgCnt);
			int intGroupCnt = clearGroup(strPara);
			string strReply = "ɸ��Ⱥ��" + to_string(intGroupCnt) + "����";
			AddMsgToQueue(strReply, masterQQ);
		}
		else if (strOption == "only") {
			if (boolPreserve) {
				AddMsgToQueue("�ѳ�ΪMaster��ר�����", masterQQ);
			}
			else {
				boolPreserve = true;
				AddMsgToQueue("�ѳ�ΪMaster��ר�������", masterQQ);
			}
		}
		else if (strOption == "public") {
			if (boolPreserve) {
				boolPreserve = false;
				AddMsgToQueue("�ѳ�Ϊ���������", masterQQ);
			}
			else {
				AddMsgToQueue("�ѳ�Ϊ�������", masterQQ);
			}
		}
		else if (strOption == "clockon") {
			string strHour;
			while (isdigit(strMessage[intMsgCnt])) {
				strHour += strMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strHour.empty()||stoi(strHour)> 23) {
				ClockToWork = { 24,00 };
				AddMsgToQueue("�������ʱ����", masterQQ);
				return;
			}
			int intHour = stoi(strHour);
			while (!isdigit(strMessage[intMsgCnt])) {
				intMsgCnt++;
			}
			string strMinute;
			while (isdigit(strMessage[intMsgCnt])) {
				strMinute += strMessage[intMsgCnt];
				intMsgCnt++;
			}
			if(strMinute.empty()) strMinute="00";
			int intMinute = stoi(strMinute);
			ClockToWork = { intHour,intMinute };
			AddMsgToQueue("�����ö�ʱ����ʱ��"+strHour+":"+strMinute,masterQQ);
		}
		else if (strOption == "clockoff") {
		string strHour;
		while (isdigit(strMessage[intMsgCnt])) {
			strHour += strMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strHour.empty() || stoi(strHour) > 23) {
			ClockOffWork = { 24,00 };
			AddMsgToQueue("�������ʱ�ر�", masterQQ);
			return;
		}
		int intHour = stoi(strHour);
		while (!isdigit(strMessage[intMsgCnt])) {
			intMsgCnt++;
		}
		string strMinute;
		while (isdigit(strMessage[intMsgCnt])) {
			strMinute += strMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strMinute.empty()) strMinute = "00";
		int intMinute = stoi(strMinute);
		ClockOffWork = { intHour,intMinute };
		AddMsgToQueue("�����ö�ʱ�ر�ʱ��" + strHour + ":" + strMinute, masterQQ);
		}
		else {
			if (intMsgCnt == strMessage.length()) {
				AddMsgToQueue("��ʲô��ô�� Master��", masterQQ);
				return;
			}
			bool boolErase = false;
			std::string strTargetID;
			if (strMessage[intMsgCnt] == '-') {
				boolErase = true;
				intMsgCnt++;
			}
			while (isspace(static_cast<unsigned char>(strMessage[intMsgCnt])))intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strMessage[intMsgCnt]))) {
				strTargetID += strMessage[intMsgCnt];
				intMsgCnt++;
			}
			long long llTargetID;
			if (strTargetID.empty()) {
				llTargetID = stoll(strTargetID);
			}
			if (strOption == "dismiss") {
				WhiteGroup.erase(llTargetID);
				if (getGroupList().count(llTargetID)) {
					setGroupLeave(llTargetID);
					AddMsgToQueue("�������˳���Ⱥ��", masterQQ);
				}
				else if (llTargetID > 1000000000 && setDiscussLeave(llTargetID) == 0) {
					AddMsgToQueue("�������˳����������", masterQQ);
					DiscussList.erase(llTargetID);
				}
				else {
					AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ);
				}
			}
			else if (strOption == "boton") {
				if (getGroupList().count(llTargetID)) {
					if (DisabledGroup.count(llTargetID)) {
						DisabledGroup.erase(llTargetID);
						AddMsgToQueue("�������ڸ�Ⱥ���á�", masterQQ);
					}
					else AddMsgToQueue("�������ڸ�Ⱥ����!", masterQQ);
				}
				else {
					AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ);
				}
			}
			else if (strOption == "botoff") {
				if (getGroupList().count(llTargetID)) {
					if (!DisabledGroup.count(llTargetID)) {
						DisabledGroup.insert(llTargetID);
						AddMsgToQueue("�������ڸ�Ⱥ��Ĭ��", masterQQ);
					}
					else AddMsgToQueue("�������ڸ�Ⱥ��Ĭ!", masterQQ);
				}
				else {
					AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ);
				}
			}
			else if (strOption == "whitegroup") {
				if (boolErase) {
					if (WhiteGroup.count(llTargetID)) {
						WhiteGroup.erase(llTargetID);
						AddMsgToQueue("��Ⱥ���Ƴ���������", masterQQ);
					}
					else {
						AddMsgToQueue("��Ⱥ�����ڰ�������", masterQQ);
					}
				}
				else {
					if (WhiteGroup.count(llTargetID)) {
						AddMsgToQueue("��Ⱥ�Ѽ��������!", masterQQ);
					}
					else {
						WhiteGroup.insert(llTargetID);
						AddMsgToQueue("��Ⱥ�Ѽ����������", masterQQ);
					}
				}
			}
			else if (strOption == "blackgroup") {
				if (boolErase) {
					if (BlackGroup.count(llTargetID)) {
						BlackGroup.erase(llTargetID);
						AddMsgToQueue("��Ⱥ���Ƴ���������", masterQQ);
					}
					else {
						AddMsgToQueue("��Ⱥ�����ں�������", masterQQ);
					}
				}
				else {
					if (BlackGroup.count(llTargetID)) {
						AddMsgToQueue("��Ⱥ�Ѽ��������!", masterQQ);
					}
					else {
						BlackGroup.insert(llTargetID);
						AddMsgToQueue("��Ⱥ�Ѽ����������", masterQQ);
					}
				}
			}
			else if (strOption == "whiteqq") {
				if (boolErase) {
					if (WhiteQQ.count(llTargetID)) {
						WhiteQQ.erase(llTargetID);
						AddMsgToQueue("���û����Ƴ���������", masterQQ);
					}
					else {
						AddMsgToQueue("���û������ڰ�������", masterQQ);
					}
				}
				else {
					if (WhiteQQ.count(llTargetID)) {
						AddMsgToQueue("���û��Ѽ��������!", masterQQ);
					}
					else {
						WhiteQQ.insert(llTargetID);
						AddMsgToQueue("���û��Ѽ����������", masterQQ);
					}
				}
			}
			else if (strOption == "blackqq") {
				if (boolErase) {
					if (BlackQQ.count(llTargetID)) {
						BlackQQ.erase(llTargetID);
						AddMsgToQueue("���û����Ƴ���������", masterQQ);
					}
					else {
						AddMsgToQueue("���û������ں�������", masterQQ);
					}
				}
				else {
					if (BlackQQ.count(llTargetID)) {
						AddMsgToQueue("���û��Ѽ��������!", masterQQ);
					}
					else {
						BlackQQ.insert(llTargetID);
						AddMsgToQueue("���û��Ѽ����������", masterQQ);
					}
				}
			}
			else AddMsgToQueue("��ʲô��ô�� Master��", masterQQ);
			return;
		}
	}
}

EVE_Menu(eventClearGroup) {
	int intGroupCnt = clearGroup();
	string strReply = "��������Ȩ��Ⱥ��" + to_string(intGroupCnt) + "����";
	MessageBoxA(nullptr, strReply.c_str(), "һ������", MB_OK | MB_ICONINFORMATION);
	return 0;
}

EVE_Menu(eventGlobalSwitch) {
	if (boolDisabledGlobal) {
		boolDisabledGlobal = false;
		MessageBoxA(nullptr, "�����ѽ�����Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}
	else {
		boolDisabledGlobal = true;
		MessageBoxA(nullptr, "������ȫ�־�Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}


