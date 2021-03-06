/*
 * $Id: NatThread.cpp 20479 2010-08-27 10:19:36Z huby $
 * 
 * this file is part of easyMule
 * Copyright (C)2002-2008 VeryCD Dev Team ( strEmail.Format("%s@%s", "emuledev", "verycd.com") / http: * www.easymule.org )
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
// NatThread.cpp : 实现文件
//

#include "stdafx.h"
#include "emule.h"
#include "NatThread.h"
#include "emuledlg.h"
#include "ClientList.h"
#include "updownclient.h"
#include "ListenSocket.h"
#include "sockets.h"
#include "GlobalVariable.h"
#include "UserMsgs.h"

#include "kademlia\kademlia\kademlia.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CNatTravHelper * CNatThread::m_pHelper = NULL;
#ifdef _DEBUG_NAT

inline void T_TRACE(char* fmt, ...)
{
	va_list argptr;
	char bufferline[1024];
	va_start(argptr, fmt);
	_vsnprintf(bufferline, 1023, fmt, argptr);
	va_end(argptr);

	char /*osDate[30],*/osTime[30]; 
	_strtime( osTime );
	//_strdate( osDate );

	char tempf[1024+512]; 
	_snprintf(tempf,1024+511,"###  %08d: %s\n", /*osDate, */GetTickCount(), bufferline);
	OutputDebugStringA(tempf);
}
#else
#define T_TRACE
#endif

//#if defined(_DEBUG) || defined(_BETA) || defined(_VCALPHA)
#if defined(_DEBUG) || defined(_BETA)
void _AddDebugLogLine(bool bAddToStatusBar, LPCTSTR pszLine, ...)
{
	ASSERT(pszLine != NULL);

	va_list argptr;
	va_start(argptr, pszLine);
	//AddLogTextV(LOG_DEBUG | (bAddToStatusBar ? LOG_STATUSBAR : 0), DLP_DEFAULT, pszLine, argptr);
	if(CNatThread::m_pHelper)
	{
		CNatThread::m_pHelper->AddLogTextV(LOG_DEBUG | (bAddToStatusBar ? LOG_STATUSBAR : 0), DLP_LOW, pszLine, argptr);
	}
	va_end(argptr);	
}
#endif

#ifdef _DEBUG_NAT
void _AddLogLine(bool bAddToStatusBar, LPCTSTR pszLine, ...)
{
	ASSERT(pszLine != NULL);

	va_list argptr;
	va_start(argptr, pszLine);
	if(CNatThread::m_pHelper)
	{
		CNatThread::m_pHelper->AddLogTextV(LOG_DEFAULT | (bAddToStatusBar ? LOG_STATUSBAR : 0), DLP_LOW, pszLine, argptr);
	}
	va_end(argptr);
}
#endif

void GetMyUserHash(uchar * id)
{
	memcpy(id, CPreferences::userhash, 16);
}

// CNatThread

IMPLEMENT_DYNCREATE(CNatThread, CWinThread)

CNatThread::CNatThread()
{
	m_bRegister = false;
	m_dwSendConnTime = m_RegisterTime = m_dwSendStaticsTime = time(NULL);
	m_dwSvrRetTime = m_RegisterTime;

	m_wSvrPort = 0;
	m_dwSvrIp = 0;

	m_dwSvrKeepalive = 0;
	m_dwTraverseBySvr = 0;

	m_wTraverseBySvr = 0;
	m_wTraverseBySE  = 0;
	m_tmNextRefreshSvr =0;

	m_dwRegAttemptTimes = 0;
	m_dwRegNextAttempInterval = INIT_ATTEMPT_INTERVAL;
}

CNatThread::~CNatThread()
{
	//MODIFIED by VC-fengwen on 2007/09/04 <begin> : 减少crash的机率，虽然这样还是有可能crash。
		//CSingleLock locker(&m_Mutex, TRUE);
		//CGlobalVariable::natthread = NULL;
	CGlobalVariable::natthread = NULL;
	CSingleLock locker(&m_Mutex, TRUE);
	//MODIFIED by VC-fengwen on 2007/09/04 <end> : 减少crash的机率，虽然这样还是有可能crash。
	TRACE("exit nat thread\n");

	try
	{
		POSITION pos = m_SockMap.GetHeadPosition();
		while(pos)
		{
			CNatSocket * nsock= m_SockMap.GetNextValue(pos);
			delete nsock;
		}
		m_SockMap.RemoveAll();

		pos = m_NatSockPool.GetHeadPosition();
		while(pos)
		{
			CNatSocket * nsock= m_NatSockPool.GetNextValue(pos);
			delete nsock;
		}
		m_NatSockPool.RemoveAll();

		pos=m_UnConnectSocket.GetHeadPosition();
		while(pos)
		{
			CUnconnSocket * usock=m_UnConnectSocket.GetNext(pos);
			delete usock;
		}
		m_UnConnectSocket.RemoveAll();

		pos = m_TempASock.GetHeadPosition();
		while(pos)
		{
			CAsyncSocketEx * asock=m_TempASock.GetNextValue(pos);
			delete asock;
		}
		m_TempASock.RemoveAll();

		pos = m_UnconnectBuffer.GetHeadPosition();
		m_UnconnectBuffer.RemoveAll();
	}
	catch(...){}
}

BOOL CNatThread::InitInstance()
{
#ifdef  _DEBUG
//	AfxBeginThread(RUNTIME_CLASS(CTestThread));
#endif
	TRACE("*** nat thread is %08x,  %d\n", GetCurrentThreadId(), GetCurrentThreadId());
	return TRUE;
}

int CNatThread::ExitInstance()
{
	CGlobalVariable::natthread = NULL;
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CNatThread, CWinThread)
END_MESSAGE_MAP()

void CNatThread::RefreshServerIp()
{
	//MODIFIED by VC-fengwen on 2007/09/04 : <begin> 这样不会有warning :)
	//while(true)
	for(;;)
	//MODIFIED by VC-fengwen on 2007/09/04 : <end> 这样不会有warning :)
	{
		hostent *pEnt=gethostbyname("nat.emule.org.cn");
		if(pEnt)
		{
			in_addr addr;
			if(*(pEnt->h_addr_list))
			{
				addr = *(struct in_addr *)*(pEnt->h_addr_list);

				DWORD sip=addr.s_addr;
				if(sip)
				{
					WORD sport=htons(2004);
					SetTraversalSvr(sip, sport);
					break;
				}
			}
		}

		Sleep(1000 * 10);
	}

	m_tmNextRefreshSvr = time(NULL) + 3*60;
}

// CNatThread 消息处理程序
bool g_bStartFindBuddy = false;
BOOL CNatThread::Run()
{
	RefreshServerIp();

	bool bBeginWork = false;
	DWORD tmBeginWork= time(NULL);
	DWORD tmCheckKad = tmBeginWork;
	//while (theApp.emuledlg != NULL && theApp.emuledlg->IsRunning())
	while(CGlobalVariable::IsRunning())
	{
		if(m_bRegister) 
			bBeginWork=true;
		else if(time(NULL) - tmBeginWork > 90) 
			bBeginWork=true;

		if(m_bRegister && time(NULL)- m_dwSvrRetTime>200 )
		{
			//失去NAT服务器连接
			AddLogLine(false, _T("Lost connection with NAT traversal server\n"));

			m_dwSvrRetTime = time(NULL);
			m_bRegister = false;

			//  也许需要重新获取一下服务器的地址
			RefreshServerIp();
		}
		else if(bBeginWork)
		{
			CSingleLock locker(&m_Mutex, TRUE);

			try
			{
				CheckConnection();
				SendKeepAlive();
				SendConnBuffer(! m_bRegister);
			}
			catch(...)
			{
				TRACE("Exception: %s\n", __FUNCTION__);
			}
		}

		if(!m_bRegister && time(NULL)-m_RegisterTime > m_dwRegNextAttempInterval)
		{
			if(time(NULL) > m_tmNextRefreshSvr)
				RefreshServerIp();

			RegisterMe(m_dwSvrIp, m_wSvrPort);
			//VC-fengwen on 2007/12/28 <begin> : 对服务器的注册重试改为指数退避。
			m_dwRegAttemptTimes++;
			int iReminder = m_dwRegAttemptTimes % 12;
			if (0 == iReminder)	// 尝试12后，过5分钟后再试。
				m_dwRegNextAttempInterval = 300;
			else
				m_dwRegNextAttempInterval = min(64, ExpoBackoff(INIT_ATTEMPT_INTERVAL, iReminder));

			//VC-fengwen on 2007/12/28 <end> : 对服务器的注册重试改为指数退避。
		}
		if(m_bRegister && time(NULL)-m_RegisterTime>90)
		{
			RegisterMe(m_dwSvrIp, m_wSvrPort);
		}

		Sleep(100);
	}
	return false;

}

void CNatThread::SendConnBuffer(bool /*bByBuddy*/)
{
	m_dwSendConnTime = time(NULL);
}

CUnconnSocket * CNatThread::FindUnconnectSocket(const uchar * id)
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	POSITION pos=m_UnConnectSocket.GetHeadPosition();
	while(pos)
	{
		CUnconnSocket * us=m_UnConnectSocket.GetNext(pos);
		if(us && memcmp(us->m_ClientId,id, 16)==0)	//id相同返回此对象
			return us;
	}

	return NULL;
}

bool CNatThread::SwitchNextStrategy(const USERHASH & UsrHash)
{
	CSingleLock locker(&m_Mutex, TRUE);

	CTraverseStrategy * strategy = NULL;
	m_TraverseStrategy.Lookup(UsrHash, strategy);

	if(strategy)
	{
		CTraverseStrategy * pNext = strategy->GetNextStrategy();
		if(! pNext)
		{
			CUpDownClient * client=CGlobalVariable::clientlist->FindClientByUserHash(strategy->GetUserHash());
			if(client && client->socket)
			{
				//client->socket->OnFailConnect(1);
				client->socket->TriggerEvent(FD_FAILED, 1);
				//client->socket->m_bUseNat = false;
			}

			delete strategy;
			m_TraverseStrategy.RemoveKey(UsrHash);
		}
		else
		{
			while(pNext)
			{
				if(pNext->Initialize()) break;

				CTraverseStrategy * tmp=pNext;
				pNext = pNext->GetNextStrategy();

				tmp->SetNextStrategy(NULL);
				delete tmp;
			}
			if(! pNext)
			{
				m_TraverseStrategy.RemoveKey(UsrHash);
				return false;
			}

			m_TraverseStrategy.SetAt(UsrHash, pNext);

			strategy->SetNextStrategy(NULL);
			delete strategy;
			return true;
		}
	}

	return false;
}

void CNatThread::CheckConnection()
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	POSITION pos = NULL, posCur=NULL;
	if(! m_UnConnectSocket.IsEmpty())
	{
		pos = m_UnConnectSocket.GetHeadPosition();
		for(; pos!=NULL; )
		{
			posCur = pos;
			CUnconnSocket * usock=m_UnConnectSocket.GetNext(pos);
			if(!usock)
			{
				m_UnConnectSocket.RemoveAt(posCur);
				continue;
			}

			if(usock->m_dwState & NAT_S_SYNC && time(NULL)-usock->m_SendPingTime >4)
			{
				if(usock->m_nPassivePing> 15)
				{
					if(usock->m_Sock)
					{
						usock->m_Sock->TriggerEvent(FD_FAILED, 1);
					}

					_AddLogLine(false, _T("Passive Unconnected NatSock was deleted. timeout. %s."), UserHashToString(usock->m_ClientId));
					
					RemoveSocketInUnconn(posCur);

				}
				else usock->SendPingPacket();
			}

		}
	}

	pos = m_SockMap.GetHeadPosition();
	while(pos)
	{
		posCur=pos;
		CNatSocket * nsock=m_SockMap.GetNextValue(pos);
		if(!nsock)
		{
			m_SockMap.RemoveAt(posCur);
			continue;
		}

		if(time(NULL) - nsock->m_dwRecvKeepalive > 4*TM_KEEPALIVE)
		{
			_AddLogLine(false, _T("NatSock was deleted. timeout. %s."), UserHashToString(nsock->GetUserHash()));

			m_SockMap.RemoveAt(posCur);
			delete nsock;
		}
	}

	//  check natsock pool
	pos = m_NatSockPool.GetHeadPosition();
	while(pos)
	{
		posCur=pos;
		CNatSocket * nsock=m_NatSockPool.GetNextValue(pos);
		if(!nsock)
		{
			m_SockMap.RemoveAt(posCur);
			continue;
		}

		if(time(NULL) - nsock->m_dwRecvKeepalive > 4*TM_KEEPALIVE)
		{
			_AddLogLine(false, _T("NatSock(in pool) was deleted. timeout. %s."), UserHashToString(nsock->GetUserHash()));
			m_NatSockPool.RemoveAt(posCur);
			delete nsock;
		}
	}

	if (NULL != theApp.emuledlg)
		PostMessage(theApp.emuledlg->GetSafeHwnd(), UM_NAT_CHECK_STRATEGIES, 0, 0);
}

void CNatThread::Connect(CAsyncSocketEx * sock, const uchar * clientid, CTraverseFactory * pFac)
{
	CSingleLock locker(&m_Mutex, TRUE);
	ASSERT(pFac);
	if(! pFac) return;

	try
	{
		if(IsSocketConnected(sock))
		{
			try
			{
				CClientReqSocket * pClientSock = DYNAMIC_DOWNCAST(CClientReqSocket, sock);
				if(pClientSock && pClientSock->client)
				{
					if (pClientSock->client->CheckHandshakeFinished())
						pClientSock->client->ConnectionEstablished();
				}
			}
			catch(...)
			{
				TRACE("Exception: %s\n", __FUNCTION__);
				return;
			}
			//sock->OnConnect(0);
			//sock->TriggerEvent(FD_CONNECT, 0);
			return;
		}

		CTraverseStrategy * ts;
		//  exists, return directly
		if(m_TraverseStrategy.Lookup(clientid, ts)) return;

		ts=pFac->CreateStrategy(clientid);
		ASSERT(ts);
		if(ts->Initialize())
			m_TraverseStrategy.SetAt(clientid, ts);
	}
	catch(...)
	{
		TRACE("Exception: %s\n", __FUNCTION__);
	}
}

int CNatThread::Send(CAsyncSocketEx * sock, const void * data, int len)
{
	CNatSocket * nsock=NULL;
	CSingleLock locker(&m_Mutex, TRUE);
	m_SockMap.Lookup(sock, nsock);

	int nRet = -1;
	if(nsock)
	{
//		ASSERT(nsock->m_MyVSock == sock->m_MyVSock);
#ifdef _DEBUG_NAT
		const uchar * uh=nsock->GetUserHash();
		T_TRACE("%s: len=%d to %02X%02X%02X%02X",__FUNCTION__, len,
			uh[0],uh[1],uh[2],uh[3]);
#endif
		nRet = nsock->Send(data, len);
	}
	else
	{
		WSASetLastError(WSAEWOULDBLOCK);
		TRACE("->%s:  but no peer to send\n",__FUNCTION__);
	}

	return nRet;
}

CNatSocket * CNatThread::FindNatSocket(DWORD ip, WORD port, DWORD ConnAck)
{
	CSingleLock locker(&m_Mutex, TRUE);	//ADDED by VC-fengwen on 2007/09/04 : 忘锁了吧 !?。

	POSITION pos = m_SockMap.GetHeadPosition();
	while(pos)
	{
		CNatSocket * nsock=m_SockMap.GetNext(pos)->m_value;
		if(nsock && nsock->GetTargetIP()==ip && nsock->GetTargetPort()==port
			&& ConnAck==nsock->m_dwConnAskNumber)
		{
			return nsock;
		}
	}

	return NULL;
}

CUnconnSocket * CNatThread::FindUnconnectSocketByIP(DWORD ip, WORD port)
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	POSITION pos=m_UnConnectSocket.GetHeadPosition();
	while(pos)
	{
		CUnconnSocket * us=m_UnConnectSocket.GetNext(pos);
		if(us && us->m_dwClientIp==ip && us->m_wClientPort==port)
			return us;
	}

	return NULL;
}

void CNatThread::OnSync(DWORD ip, WORD port, const uchar * pData, int nDataSize)
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	if(ip==m_dwSvrIp && port==m_wSvrPort)
	{
		m_dwSvrRetTime = time(NULL);
		T_TRACE("reset svr time in sync");
	}

	DWORD cip = *(DWORD*)(pData);
	WORD cport = *(WORD*)(pData+4);

	if(cip==0 || cport==0)
	{
		return;
	}

	const uchar * id=pData+6;
	DWORD ConnAck = 0;
	memcpy(&ConnAck, id+16, 4);

	CNatSocket * nsock=FindNatSocket(id);

	if(nsock)
	{
		nsock->SetTargetAddr(cip, cport);
		if(nsock->m_dwConnAskNumber!=ConnAck)
		{
			// VC-linhai[2007-08-06]:warning C4189: “pOldParent” : 局部变量已初始化但不引用
			//CAsyncSocketEx * pOldParent=nsock->GetParent();
			nsock->RenewNatSock();
			nsock->m_dwConnAskNumber = ConnAck;
		}

		uchar pack[20];
		GetMyUserHash(pack);
		memcpy(pack+16, &nsock->m_dwConnAskNumber, 4);

		nsock->SendPacket(OP_NAT_PING, pack, 20);

		return;
	}

	if(WakeupNatSocketFromPool(id, cip, cport, ConnAck))
		return;

	if(ProcessStrategyPacket(id, ip, port, pData-1, nDataSize+1))  // Include opcode
		return;

	uchar myid[16];
	GetMyUserHash(myid);
	if(memcmp(id, myid, 16)==0)
	{
		_AddLogLine(false, _T("ERROR: recv myself for traversal"));
		return;
	}

	//  create a new unconnect socket

	CAsyncSocketEx * pASock=NULL;
	pASock = GetTempASock(id);
	ASSERT(pASock);

	CUnconnSocket * usock=FindUnconnectSocket(id);// FindUnconnectSocketByIP(cip, cport);
	if(! usock)
	{
		TRACE("create unconnsock for recv sync. asyncsock=%08x\n", pASock);

		usock = new CUnconnSocket(ip, port, pASock, id);
		usock->m_dwConnAskNumber = 0;
		usock->m_dwClientIp = cip;
		usock->m_wClientPort =cport;
		m_UnConnectSocket.AddTail(usock);

		_AddDebugLogLine(false, _T("CNatThread::ProcessPacket Create CUnconnSocket Successful for %s!\n"),ipstr(cip,cport));
	}
	else
	{
		//ASSERT(usock->m_MyVSock==tag);
		usock->m_dwClientIp = cip;
		usock->m_wClientPort = cport;
	}

	usock->m_dwState |=NAT_S_SYNC;
	usock->SendPingPacket();
}

bool CNatThread::ProcessPacket(const BYTE* packet, UINT size, uint32 ip, uint16 port)
{
	uint8 opcode=packet[4];
	const uchar * realdata=packet+5;
	UINT realsize=size-5;
	
	CSingleLock locker(&m_Mutex, TRUE);
	
// modiefied by robert, 2010-08-03
// beta版屏蔽try，以求暴露更多问题
#ifndef _BETA
	try
	{
#endif

	switch(opcode)
	{
	case OP_NAT_REPING:
		OnReping(ip, port, realdata, realsize);
		break;
	case OP_NAT_REGISTER:

		// VC-SearchDream[2007-06-15]: Add Login Support
		if (realsize >= 2 && !m_bRegister && ip==m_dwSvrIp && port==m_wSvrPort )
		{
			if (realsize >= 6)
			{
				WORD	port	= *(WORD*)(realdata);
				DWORD	ip		= *(DWORD*)(realdata + 2);

				if (ip != m_dwSvrIp || port != m_wSvrPort)
				{
					m_dwSvrIp = ip;
					m_wSvrPort = port;
					m_bRegister	= false;
					RegisterMe(m_dwSvrIp,m_wSvrPort);
					m_dwRegAttemptTimes = 1;
					m_dwRegNextAttempInterval = INIT_ATTEMPT_INTERVAL*2;
					break;
				}
			}
			else
			{
				WORD  port = *(WORD*)(realdata);

				if (port != m_wSvrPort)
				{
					m_wSvrPort	= port;
					m_bRegister	= false;
					RegisterMe(m_dwSvrIp,m_wSvrPort);
					m_dwRegAttemptTimes = 1;
					m_dwRegNextAttempInterval = INIT_ATTEMPT_INTERVAL*2;
					T_TRACE("new server port : %d.", ntohs(port));
					break;
				}
			}
		}
	
		m_dwSvrRetTime = time(NULL);

		if(!m_bRegister)
		{
			m_bRegister=true;
			m_dwSvrKeepalive = time(NULL);
			m_dwRegAttemptTimes = 0;
			m_dwRegNextAttempInterval = INIT_ATTEMPT_INTERVAL;
			T_TRACE("reset svr time in register");
			AddLogLine(false, _T("Registered to NAT traversal server\n"));
		}

		break;
	case OP_NAT_FAILED:
		{
/*
			if(realdata[0]==2)
			{
				m_bRegister = false;
				break;
			}
			else */if(realdata[0]==1)
			{
				realdata++;
				realsize--;
				int n=realsize/16;

				if(ProcessStrategyPacket(realdata, ip, port, packet+4, size-4))
					break;

				for(int i=0; i<n; i++)
				{
					TRACE(_T("failed conn request for %s\n"),UserHashToString(realdata));
					CUnconnSocket * us= FindUnconnectSocket(realdata);
					if(us) us->m_dwState |= NAT_E_NOPEER;
					realdata+=16;
				}
			}
		}
		break;
	//case OP_NAT_SYNC2:
		//if(ProcessStrategyPacket(realdata+6, ip, port, packet+4, size-4))
		//	break;
		//OnSync2(ip, port, realdata, realsize);
		//break;
	case OP_NAT_SYNC2:
	case OP_NAT_SYNC:
		OnSync(ip, port, realdata, realsize);
		break;
	case OP_NAT_PING:
		OnPing(ip, port, realdata, realsize);
		break;
	case OP_NAT_DATA:
		{
			if(size < 4) // ACK (+ DATA)
				break;

			DWORD ConnAck = 0;
			memcpy(&ConnAck, realdata, 4);
			realdata += 4;
			realsize -= 4;
			CNatSocket* source = FindNatSocket(ip, port, ConnAck);
			if( source == NULL )
			{
#ifdef _DEBUG_NAT
				TRACE("\n*** no nat sock to recv NAT_DATA. connack=%08x\n", ConnAck);
#endif
				uint32 uSequenceNr = PeekUInt32(realdata);
				if(uSequenceNr==1)
				{
					uchar pack[16];
					GetMyUserHash(pack);
					CNatSocket::SendPacket(ip, port, OP_NAT_REPING, pack, 16);
				}
				else 
				{
					CNatSocket::SendPacket(ip, port, OP_NAT_RST, NULL, 0);
				}
				break;
			}


			if(source->ProcessDataPacket(realdata, realsize))
			{
				//TRACE("source->ProcessDataPacket is OK. tag=%08x. Asyncsock=%08x\n", tag,source->GetParent());
				//source->OnReceive(0);
				source->m_Parent->TriggerEvent(FD_READ);
				return true;
			}
			else
			{
				T_TRACE("\nFailed to source->ProcessDataPacket. ConnAck=%08x\n\n", ConnAck);
			}
			break;
		}
	case OP_NAT_ACK:
		{
			DWORD ConnAck = 0;
			memcpy(&ConnAck, realdata, 4);
			realdata+=4;
			realsize-=4;
			CNatSocket* source = FindNatSocket(ip, port, ConnAck);
			if( source == NULL )
			{
#ifdef _DEBUG_NAT
				TRACE("\nsend RST for no nat sock to do NAT_ACK\n");
#endif
				CNatSocket::SendPacket(ip, port, OP_NAT_RST,0,0);
				break;
			}
			source->ProcessAckPacket(realdata, realsize);
			break;
		}
	case OP_NAT_RST:
		{
			break;
		}
	default:
		TRACE("Unknown command\n");
		break;
	}

// modiefied by robert, 2010-08-03
// beta版屏蔽try，以求暴露更多问题
#ifndef _BETA
	}catch(...)
		{
			TRACE("Exception: %s\n", __FUNCTION__);
		}
#endif

	return true;
}

int CUnconnSocket::SendPingPacket()
{
	//ASSERT(m_pDataHandler);
	if(m_dwState & NAT_E_TIMEOUT) return 1;

	m_nPassivePing++;
	m_SendPingTime = time(NULL);

	T_TRACE("send ping packet for %02X%02X%02X%02X. ConnAck=%08x\n", m_ClientId[0],m_ClientId[1],m_ClientId[2],m_ClientId[3],
		m_dwConnAskNumber);

	if(m_dwClientIp && m_wClientPort)
	{
//		Packet pack(OP_NAT_PING, 20, OP_VC_NAT_HEADER,false);
		uchar pack[20];
		GetMyUserHash(pack);
		memcpy(pack+16, &m_dwConnAskNumber, 4);
		
		//TRACE("\nCUnconnSocket::SendPingPacket Send Ping Successful! IP : %d Port %d\n", m_dwClientIp, m_wClientPort);

		return CNatSocket::SendPacket(m_dwClientIp, m_wClientPort, OP_NAT_PING, pack, 20);
	}

	return -1;
}

BOOL CNatThread::IsSocketConnected(CAsyncSocketEx * sk)
{
	CSingleLock locker(&m_Mutex, TRUE);
	BOOL b=m_SockMap.Lookup(sk)!=NULL;

	return b;
}

void CNatThread::RegisterMe(DWORD sip, WORD sport)
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);
	
	if(!CGlobalVariable::clientudp) return;
	
	_AddDebugLogLine(false, _T("CNatThread::RegisterMe"));
	
	if(!m_bRegister)
		AddLogLine(false, _T("Registering to NAT traversal server [%s]\n"),ipstr(sip,ntohs(sport)));

	m_dwSvrKeepalive = m_RegisterTime = time(NULL);


	// VC-SearchDream[2007-06-08]: new way statics Begin
	if (m_RegisterTime - m_dwSendStaticsTime > 60 * 10)
	{
		uchar pack[22];
		GetMyUserHash(pack);

		WORD counter1 = m_wTraverseBySvr, counter2 = m_wTraverseBySvr, counter3 = m_wTraverseBySE;

		counter1 = htons(counter1);
		counter2 = htons(counter2);
		counter3 = htons(counter3);

		memcpy(pack+16, &counter1, 2);
		memcpy(pack+18, &counter2, 2);
		memcpy(pack+20, &counter3, 2);

		CNatSocket::SendPacket(sip, sport, OP_NAT_REGISTER, pack, 22);

		m_dwSendStaticsTime = m_RegisterTime;
		m_wTraverseBySvr = m_wTraverseBySE = 0;
	}
	else
	{
		uchar pack[16];
		GetMyUserHash(pack);

		CNatSocket::SendPacket(sip, sport, OP_NAT_REGISTER, pack, 16);
	}
}

void CNatThread::SendKeepAlive()
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	//  for server
	if(m_bRegister && m_dwSvrKeepalive && time(NULL) - m_dwSvrKeepalive > TM_KEEPALIVE)
	{
		uchar pack[1];

		CNatSocket::SendPacket(m_dwSvrIp, m_wSvrPort, pack, 1);
		m_dwSvrKeepalive = time(NULL);
#ifdef _DEBUG_NAT
		TRACE("send keepalive to svr\n");
#endif
	}

	//  for every nat sock clients
	POSITION pos = m_SockMap.GetHeadPosition(), posCur;
	while(pos)
	{
		posCur = pos;
		CNatSocket * nsock=m_SockMap.GetNextValue(pos);

		if(!nsock)
		{
			m_SockMap.RemoveAt(posCur);
			continue;
		}

		nsock->CheckForTimeOut();
	}

	//  for sock pool to keep connection
	pos = m_NatSockPool.GetHeadPosition();
	while(pos)
	{
		posCur = pos;
		CNatSocket * nsock=m_NatSockPool.GetNextValue(pos);

		if(!nsock)
		{
			m_NatSockPool.RemoveAt(posCur);
			continue;
		}

		nsock->CheckForTimeOut();
	}
}

void CNatThread::RemoveSocket(CAsyncSocketEx * sock,bool bDisconnectAll/*=false*/,const uchar* pUserHash/*=NULL*/)
{
	//AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CSingleLock locker(&m_Mutex, TRUE);
	CNatSocket *nsock=NULL;
	m_SockMap.Lookup(sock, nsock);
	if(nsock)
	{
#ifdef _DEBUG_NAT
		_AddLogLine(false, _T("NatSock was put into pool. by above application. %s."), UserHashToString(nsock->GetUserHash()));
#endif
		m_SockMap.RemoveKey(sock);
		RemoveTempASock(nsock->GetUserHash());

		if(bDisconnectAll)
		{
			delete nsock;
		}
		else
		{
			nsock->DumpData();
			nsock->m_Parent = NULL;
			nsock->m_dwConnAskNumber = 0;
			m_NatSockPool.SetAt(nsock->GetUserHash(), nsock);
		}				
	}

	RemoveSocketInUnconn(sock);

	if( bDisconnectAll && pUserHash )
	{
		RemoveStrategy( pUserHash );
		RemoveNatSocket( pUserHash );
	}
}

int CNatThread::Receive(CAsyncSocketEx * sock, void * data, int len)
{
	CNatSocket * nsock=NULL;

	CSingleLock locker(&m_Mutex, TRUE);
	m_SockMap.Lookup(sock, nsock);

	int nRet = -1;
	if(nsock)
	{
		nRet = nsock->Receive(data, len);
	}

	return nRet;

}

DWORD CNatThread::GetUsedTime(CAsyncSocketEx * sock)
{
	CSingleLock locker(&m_Mutex, TRUE);

	CNatSocket * nsock=NULL;
	m_SockMap.Lookup(sock, nsock);

	if(nsock)
	{
		return nsock->m_dwRecvKeepalive;
	}

	POSITION pos = m_UnConnectSocket.GetHeadPosition();
	while(pos)
	{
		CUnconnSocket * us= m_UnConnectSocket.GetNext(pos);
		if(us && us->m_Sock == sock)
		{
			return time(NULL);
		}
	}

	CClientReqSocket * cs=NULL;
	try
	{
		cs=DYNAMIC_DOWNCAST(CClientReqSocket, sock);
		if(cs && cs->client)
		{
			if(m_TraverseStrategy.Lookup(cs->client->GetUserHash()))
			{
				return time(NULL);
			}
		}
	}
	catch(...)
	{
		TRACE("Exception: %s\n", __FUNCTION__);
	}

	return 0;
}

void CNatThread::RemoveTempASock(const uchar * id)
{
	CSingleLock locker(&m_Mutex, TRUE);	//ADDED by VC-fengwen on 2007/09/04 : 忘锁了吧 !?。
	
	USERHASH k=id;
	m_TempASock.RemoveKey(k);
}

#ifdef _DEBUG
void CNatThread::Test()
{

}
#endif

void CNatThread::RemoveNatSocket(const uchar *UserId)
{
	USERHASH k(UserId);
	CNatSocket * nsock=NULL;

	CSingleLock locker(&m_Mutex, TRUE);
	if(m_NatSockPool.Lookup(k, nsock))
	{
		m_NatSockPool.RemoveKey(k);
		_AddLogLine(false, _T("NatSock (in sock pool) was deleted. by above application. %s."), UserHashToString(nsock->GetUserHash()));
		delete nsock;
	}
}

void CNatThread::OnReping(DWORD ip, WORD port, const uchar * pData, int nDataSize)
{
	CNatSocket * nsock=NULL;

	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	POSITION pos = m_SockMap.GetHeadPosition();

	while(pos)
	{
		CNatSocket * tmp=m_SockMap.GetNext(pos)->m_value;
		if(tmp && memcmp(pData, tmp->GetUserHash(), 16)==0)
		{
			nsock = tmp;
			break;
		}
	}

	T_TRACE("%s, nDataSize=%d. %02X%02X%02X%02X", __FUNCTION__ , nDataSize, pData[0],pData[1],pData[2],pData[3]);

	if(nDataSize==16)
	{
		if(!nsock)
		{
			T_TRACE("no NatSock for REPING 1. %02X%02X%02X%02X", pData[0],pData[1],pData[2],pData[3]);
			return;
		}
		uchar pack[20];
		GetMyUserHash(pack);
		memcpy(pack+16, &nsock->m_dwConnAskNumber, 4);
		CNatSocket::SendPacket(ip, port, OP_NAT_REPING, pack, 20);
	}
	else if(nDataSize==20)
	{
		DWORD dwConnAck=0;
		memcpy(&dwConnAck, pData+16, 4);

		T_TRACE("reping tell ConnAck=%d", dwConnAck);

		if(! dwConnAck) return;

		if(nsock)
		{
			nsock->m_dwConnAskNumber = dwConnAck;
			return;
		}

		//  from natsock pool 
		CNatSocket * nsock_inpool=NULL;
		if(m_NatSockPool.Lookup(pData, nsock_inpool))
		{
			m_NatSockPool.RemoveKey(pData);
			nsock_inpool->m_Parent = CGlobalVariable::listensocket->OnAcceptEx(0);
			nsock_inpool->m_dwConnAskNumber=dwConnAck;

			ASSERT(nsock_inpool->m_UserModeTCPConfig);
			nsock_inpool->m_UserModeTCPConfig->TargetIP = ip;
			nsock_inpool->m_UserModeTCPConfig->TargetPort = port;

			//m_SockMap.SetAt(nsock_inpool->m_Parent, nsock_inpool);
			AddNatSocket(nsock_inpool->m_Parent, nsock_inpool);
			RemoveUnconnSocket(pData); //ADDED by VC-fengwen on 2007/10/15 : sock连通后，在unconnectsocket里要清除它。

			return;
		}

		CAsyncSocketEx * pASock = RemoveUnconnSocket(pData);

		if(!pASock) pASock = CGlobalVariable::listensocket->OnAcceptEx(0);

		CNatSocket * nsock=new CNatSocket(pASock);
		nsock->m_dwConnAskNumber =dwConnAck ;
		memcpy(nsock->GetUserHash(), pData, 16);
		sUserModeTCPConfig * tc=new sUserModeTCPConfig(ip, port);
		nsock->SetConfig(tc);
		nsock->m_dwRecvKeepalive = time(NULL);

		//m_SockMap.SetAt(pASock, nsock);
		AddNatSocket(pASock, nsock);
	}
}

bool CNatThread::GetAddr(const uchar * UserHash, DWORD & ip, WORD & port)
{
	CSingleLock locker(&m_Mutex, TRUE);	//ADDED by VC-fengwen on 2007/09/04 : 忘锁了吧 !?。

	CNatSocket * nsock=NULL;
	if(m_NatSockPool.Lookup(UserHash, nsock))
	{
		if(!nsock)
		{
			m_NatSockPool.RemoveKey(UserHash);
		}
	}
	else
	{
		POSITION pos = m_SockMap.GetHeadPosition();
		while(pos)
		{
			nsock=m_SockMap.GetNextValue(pos);
			if(nsock && memcmp(UserHash, nsock->GetUserHash(), 16)==0)
			{
				break;
			}
		}
	}

	if(nsock)
	{
		ip = nsock->m_UserModeTCPConfig->TargetIP;
		port = nsock->m_UserModeTCPConfig->TargetPort;
		return true;
	}

	return false;
}

CUnconnSocket::~CUnconnSocket()
{
}

CUnconnSocket::CUnconnSocket(DWORD sip, WORD sport, CAsyncSocketEx * s, const uchar *id)
{
//	m_Client = NULL;
	m_nTraverseType = 0;
	m_dwSvrIp = sip;
	m_wSvrPort = sport;

	m_Sock = s;
	memcpy(m_ClientId, id, 16);

	m_SendPingTime = m_SendConnAckTime = 0;

	m_nPassivePing = 0;

	m_dwClientIp = 0;
	m_wClientPort = 0;

	m_dwState = 0;
	m_dwConnAskNumber = 0;//GetTickCount();

//	m_pDataHandler = NULL;
}

void CNatThread::OnPing(DWORD ip, WORD port, const uchar * pData, int nDataSize)
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	if(nDataSize==0)
	{
		//数据区长度为0

		if(ip==m_dwSvrIp && port==m_wSvrPort)
		{
			m_dwSvrRetTime = time(NULL);
			T_TRACE("reset svr time in ping");
			return;
		}

		POSITION pos = m_SockMap.GetHeadPosition();
		while(pos)
		{
			CNatSocket * nsock=m_SockMap.GetNext(pos)->m_value;
			if(nsock && nsock->GetTargetIP()==ip && nsock->GetTargetPort()==port)
			{
#ifdef _DEBUG_NAT
				const uchar * id=nsock->GetUserHash();
				TRACE("recv keepalive from %02X%02X%02X%02X \n", id[0],id[1],id[2],id[3]);
#endif
				nsock->m_dwRecvKeepalive = time(NULL);
			}
		}

		pos = m_NatSockPool.GetHeadPosition();
		while(pos)
		{
			CNatSocket * nsock=m_NatSockPool.GetNextValue(pos);
			if(nsock && nsock->GetTargetIP()==ip && nsock->GetTargetPort()==port)
			{
#ifdef _DEBUG_NAT
				const uchar * id=nsock->GetUserHash();
				TRACE("recv keepalive(sock pool) from %02X%02X%02X%02X \n", id[0],id[1],id[2],id[3]);
#endif
				nsock->m_dwRecvKeepalive = time(NULL);
			}
		}

		return;
	}

	//数据区长度非0，数据区为user hash, Connack
	const uchar * hash=pData;

	
	if(ProcessStrategyPacket(hash, ip, port, pData-1, nDataSize+1))  //  Include opcode
		return;//必然不返回

	DWORD ConnAck=0;
	memcpy(&ConnAck, hash+16, 4);
	TRACE("recv ping 1. %02X%02X%02X%02X, ConnAck=%d\n", hash[0], hash[1], hash[2], hash[3],
		ConnAck);

	CAsyncSocketEx * pASock=NULL;
	CUnconnSocket * usock=FindUnconnectSocket(hash);
	if(usock)
	{
		//发现此未连接用户

		if(ConnAck && !usock->m_dwConnAskNumber)
			usock->m_dwConnAskNumber = ConnAck;
		pASock = usock->m_Sock;

		if(!pASock)
		{
			//未发现此用户套接字

			TRACE("no AsyncsocketEx for unconnect sk\n");
			POSITION pos = m_UnConnectSocket.GetHeadPosition(), curPos;

			while(pos)
			{
				curPos = pos;
				CUnconnSocket * us= m_UnConnectSocket.GetNext(pos);
				if(us && memcmp(us->m_ClientId, hash, 16)==0)
				{
					TRACE("remove invalid unconnected sk\n");

					RemoveSocketInUnconn(curPos);
					break;
				}
			}
			return;
		}
	}
	else
	{
		//未发现此未连接用户

		if(nDataSize>20) return;

		CNatSocket * nsock = FindNatSocket(hash);

		if(! nsock)
		{
			//未发现此nat用户
			nsock =WakeupNatSocketFromPool(hash);
			if(! nsock)
			{
				TRACE("*** recv a invalid ping.  %02X%02X%02X%02X\n", hash[0], hash[1], hash[2], hash[3]);
				return;
			}
			if(ConnAck) nsock->m_dwConnAskNumber = ConnAck;
		}

		if(nsock->m_dwConnAskNumber!=ConnAck)
		{
			T_TRACE("invalid ConnAck for ping.ConnAck=%d", ConnAck);
			return;
			//nsock->DumpData();
		}

		nsock->m_UserModeTCPConfig->TargetIP = ip;
		nsock->m_UserModeTCPConfig->TargetPort = port;
		uchar pack[21];
		GetMyUserHash(pack);
		memcpy(pack+16, &ConnAck, 4);

		nsock->SendPacket(OP_NAT_PING, pack, 21);
		TRACE("I am connected, echo a ping for connecting.  %02X%02X%02X%02X\n", hash[0], hash[1], hash[2], hash[3]);

		//nsock->GetParent()->TriggerEvent(FD_CONNECT);
		return;
	}

	//发现此用户套接字,此部分不可能被执行到
	ASSERT(pASock);
	CNatSocket * nsock=new CNatSocket(pASock);
	nsock->m_dwConnAskNumber= usock->m_dwConnAskNumber;
	sUserModeTCPConfig * tc=new sUserModeTCPConfig(ip, port);
	nsock->SetConfig(tc);
	nsock->m_dwRecvKeepalive = time(NULL);
	//if(! usock->m_bByBuddy)
	//{
	//	nsock->m_bTraverseBySvr= true;
	//	m_dwTraverseBySvr++;
	//}
	nsock->m_TraversalType = CNatSocket::Traversal_bysvr;

	ASSERT(m_SockMap.Lookup(pASock)==0);
	//m_SockMap.SetAt(pASock, nsock);
	AddNatSocket(pASock, nsock);
	memcpy(nsock->GetUserHash(), hash, 16);
	POSITION pos =m_UnConnectSocket.Find(usock);
	if(pos)
	{
		m_UnConnectSocket.RemoveAt(pos);
	}

	_AddLogLine(false, _T("NAT traversal connected. %s."), UserHashToString(hash));

	RemoveTempASock(hash);

	//nsock->m_Parent->TriggerEvent(FD_CONNECT);
	//nsock->m_Parent->TriggerEvent(FD_WRITE);
	CClientReqSocket * pClientSock = DYNAMIC_DOWNCAST(CClientReqSocket, pASock);
	if(pClientSock && pClientSock->client)
	{
		pClientSock->client->SetIP(ip);
		pClientSock->client->ResetIP2Country();
	}
	delete usock;
}

bool CNatThread::ProcessStrategyPacket(USERHASH uh, DWORD ip, WORD port, const uchar * pData, int nDataSize)
{
	CSingleLock locker(&m_Mutex, TRUE);
	
	CTraverseStrategy * strategy=NULL;
	m_TraverseStrategy.Lookup(uh, strategy);
	if(strategy)
	{
		try
		{
			return strategy->ProcessPacket(pData, nDataSize, ip, port);
		}
		catch(...)
		{
			TRACE("Exception in %s\n", __FUNCTION__);
		}

		SwitchNextStrategy(uh);
	}

	return false;
}

CNatSocket * CNatThread::FindNatSocket(const uchar * pUserhash)
{
	// modified by robert, 2010-07-30 加锁
	CSingleLock locker(&m_Mutex, TRUE);

	POSITION pos = m_SockMap.GetHeadPosition();
	while(pos)
	{
		CNatSocket * nsock=m_SockMap.GetNextValue(pos);
		if(nsock && memcmp(pUserhash, nsock->GetUserHash(), 16)==0)
		{
			return nsock;
		}
	}

	return NULL;
}

bool CNatThread::WakeupNatSocketFromPool(const uchar * pUserhash, DWORD ip, WORD port, DWORD dwConnAck)
{
	CSingleLock locker(&m_Mutex, TRUE);	//ADDED by VC-fengwen on 2007/09/04 : 忘锁了吧 !?。

	CNatSocket * nsock_inpool=WakeupNatSocketFromPool(pUserhash);
	if(nsock_inpool)
	{
		ASSERT(dwConnAck);
		nsock_inpool->m_dwConnAskNumber=dwConnAck;

		ASSERT(nsock_inpool->m_UserModeTCPConfig);
		nsock_inpool->SetTargetAddr(ip, port);
		return true;
	}
	return false;
}

CNatSocket *CNatThread::WakeupNatSocketFromPool(const uchar * pUserhash)
{
	CSingleLock locker(&m_Mutex, TRUE);	//ADDED by VC-fengwen on 2007/09/04 : 忘锁了吧 !?。
	
	CNatSocket * nsock_inpool=NULL;
	if(m_NatSockPool.Lookup(pUserhash, nsock_inpool))
	{
		m_NatSockPool.RemoveKey(pUserhash);
		nsock_inpool->m_Parent = CGlobalVariable::listensocket->OnAcceptEx(0);

		AddNatSocket(nsock_inpool->m_Parent, nsock_inpool);
		RemoveUnconnSocket(pUserhash);	//ADDED by VC-fengwen on 2007/10/15 : sock连通后，在unconnectsocket里要清除它。
	}

	return nsock_inpool;
}

CAsyncSocketEx * CNatThread::GetTempASock(const uchar * pUserhash)
{
	CSingleLock locker(&m_Mutex, TRUE);	//ADDED by VC-fengwen on 2007/09/04 : 忘锁了吧 !?。
	
	CAsyncSocketEx * pASock=NULL;
	if(! m_TempASock.Lookup(pUserhash, pASock))
	{
		pASock = CGlobalVariable::listensocket->OnAcceptEx(0);
		m_TempASock.SetAt(pUserhash, pASock);
	}
	return pASock;
}

void CNatThread::AddNatSocket(CAsyncSocketEx * asock, CNatSocket * nsock)
{
	CSingleLock locker(&m_Mutex, TRUE);

	// VC-SearchDream[2007-06-08]: new way statics Begin
	if (nsock->m_TraversalType == CNatSocket::Traversal_bysvr)
	{
		m_wTraverseBySvr++; 
	}
	else if (nsock->m_TraversalType == CNatSocket::Traversal_byexchangesource)
	{
		m_wTraverseBySE++; 
	}
	// VC-SearchDream[2007-06-08]: new way statics End

	m_SockMap.SetAt(asock, nsock);
	nsock->m_Parent->TriggerEvent(FD_CONNECT);	
	nsock->m_Parent->TriggerEvent(FD_WRITE);
	CClientReqSocket * pClientSock = DYNAMIC_DOWNCAST(CClientReqSocket, asock);
	if(pClientSock && pClientSock->client)
	{
		pClientSock->client->SetIP(nsock->GetTargetIP());
		pClientSock->client->ResetIP2Country();
	}
}

void CNatThread::ResetNatSocket(CAsyncSocketEx * pOldKey, CAsyncSocketEx * pNewKey)
{
	CSingleLock locker(&m_Mutex, TRUE);	//ADDED by VC-fengwen on 2007/09/04 : 忘锁了吧 !?。

	CNatSocket * nsock=NULL;
	if(m_SockMap.Lookup(pOldKey, nsock))
	{
		m_SockMap.RemoveKey(pOldKey);
		m_SockMap.SetAt(pNewKey, nsock);
		nsock->m_Parent=pNewKey;
	}
}

int CNatThread::GetTimeout(CAsyncSocketEx * sock)
{
	CSingleLock locker(&m_Mutex, TRUE);
	CNatSocket * nsock=NULL;
	if(m_SockMap.Lookup(sock, nsock) && nsock)
	{
		return nsock->GetTimeout();
	}
	return 0;
}

uint16 CNatThread::GetUDPPort(CAsyncSocketEx * sock)
{
	CSingleLock locker(&m_Mutex, TRUE);
	CNatSocket * nsock=NULL;

	if(m_SockMap.Lookup(sock, nsock) && nsock)
	{
		return nsock->GetTargetPort();
	}

	return 0;
}

void CNatThread::CheckStrategies()
{
	CSingleLock locker(&m_Mutex, TRUE);

	POSITION pos = NULL, posCur=NULL;
	
	pos=m_TraverseStrategy.GetHeadPosition();
	while(pos)
	{
		posCur = pos;
		CTraverseStrategy * strategy = m_TraverseStrategy.GetNextValue(pos);

		try
		{
			if(strategy->IsFailed())
			{
				SwitchNextStrategy(m_TraverseStrategy.GetKeyAt(posCur));
				continue;
			}

			if(strategy->IsFinish())
			{
				m_TraverseStrategy.RemoveAt(posCur);
				delete strategy;
				continue;
			}

			strategy->SendPacket();
		}
		catch(...)
		{
			TRACE("Exception: CheckConnection\n");
		}
	}
}

CAsyncSocketEx* CNatThread::RemoveUnconnSocket(const uchar * id)
{
	CSingleLock locker(&m_Mutex, TRUE);

	CAsyncSocketEx * pASock = NULL;
	POSITION curPos;
	POSITION pos = m_UnConnectSocket.GetHeadPosition();
	while(pos)
	{
		curPos = pos;
		CUnconnSocket * us= m_UnConnectSocket.GetNext(pos);
		if(us && memcmp(us->m_ClientId, id, 16)==0)
		{
			pASock = us->m_Sock;
			m_UnConnectSocket.RemoveAt(curPos);
			RemoveTempASock(us->m_ClientId);
			delete us;
			return pASock;
		}
	}
	return NULL;
}

void CNatThread::RemoveSocketInUnconn(POSITION pos)
{
	CSingleLock locker(&m_Mutex, TRUE);

	CUnconnSocket *us = m_UnConnectSocket.GetAt(pos);
	m_UnConnectSocket.RemoveAt(pos);
	if (NULL != us)
	{
		RemoveTempASock(us->m_ClientId);
		delete us;
		us = NULL;
	}
}

void CNatThread::RemoveSocketInUnconn(CAsyncSocketEx * sock)
{
	CSingleLock locker(&m_Mutex, TRUE);

	POSITION curPos = NULL;
	POSITION pos = m_UnConnectSocket.GetHeadPosition();
	while(pos)
	{
		curPos = pos;
		CUnconnSocket * us= m_UnConnectSocket.GetNext(pos);
		if(us && us->m_Sock==sock)
		{
			RemoveSocketInUnconn(curPos);
			return;
		}
	}
}

void CNatThread::RemoveStrategy( const uchar * pUserhash )
{
	CSingleLock locker(&m_Mutex, TRUE);

	CTraverseStrategy * strategy = NULL;
	if( m_TraverseStrategy.Lookup(pUserhash, strategy) )
	{
		m_TraverseStrategy.RemoveKey(pUserhash);
		if(strategy)
			delete strategy;
	}
}