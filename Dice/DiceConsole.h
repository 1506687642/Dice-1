/*
 * Copyright (C) 2019 String.Empty
 */
#pragma once
#ifndef Dice_Console
#define Dice_Console
#include <string>
#include <vector>
#include <map>
#include <set>
#include <Windows.h>
#include "GlobalVar.h"
#include "DiceMsgSend.h"
#include "CQEVE_ALL.h"

	//Masterģʽ
	extern bool boolMasterMode;
	//Master��QQ������ʱΪ0
	extern long long masterQQ;
	//����Ա�б�
	extern std::set<long long> AdminQQ;
	//��ش����б�
	extern std::set<std::pair<long long, CQ::msgtype>> MonitorList;
	//ȫ��Ⱥ��Ĭ
	extern bool boolDisabledGlobal;
	//ȫ�ֽ���.ME
	extern bool boolDisabledMeGlobal;
	//ȫ�ֽ���.jrrp
	extern bool boolDisabledJrrpGlobal;
	//��ռģʽ���������������Master���ڵ�Ⱥ������
	extern bool boolPreserve;
	//�Զ��˳�һ��������
	extern bool boolNoDiscuss;
	//��������Ϣ��¼
	extern std::map<long long, time_t> DiscussList;
	//���Ի����
	extern std::map<std::string, std::string> PersonalMsg;
	//botoff��Ⱥ
	extern std::set<long long> DisabledGroup;
	//botoff��������
	extern std::set<long long> DisabledDiscuss;
	//������Ⱥ��˽��ģʽ����
	extern std::set<long long> WhiteGroup;
	//������Ⱥ������������
	extern std::set<long long> BlackGroup;
	//�������û�������˽������
	extern std::set<long long> WhiteQQ;
	//�������û�������������
	extern std::set<long long> BlackQQ;
	//֪ͨ����Ա 
	void sendAdmin(std::string strMsg, long long fromQQ = 0);
	//֪ͨ��ش��� 
	void NotifyMonitor(std::string strMsg);
	//һ������
	extern int clearGroup(std::string strPara = "unpower", long long fromQQ = 0);
	//�����Ϣ��¼
	extern std::map<chatType, time_t> mLastMsgList;
	//���ӵ����촰��
	extern std::map<chatType, chatType> mLinkedList;
	//����ת���б�
	extern std::multimap<chatType, chatType> mFwdList;
	//��ǰʱ��
	extern SYSTEMTIME stNow;
	//�ϰ�ʱ��
	extern std::pair<int, int> ClockToWork;
	//�°�ʱ��
	extern std::pair<int, int> ClockOffWork;
	std::string printClock(std::pair<int, int> clock);
	extern void ConsoleTimer();
#endif /*Dice_Console*/


