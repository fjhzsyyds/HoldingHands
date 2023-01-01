
#pragma once
#include <afx.h>
#include <winsock2.h>
#include <MSWSock.h>
#include <mstcpip.h>
#include <fstream>
///////////////////////////////////////////////////////////////////////////////////////
//1.��������һ��socket,Read(����Write)����ͬʱֻ����һ���߳�ȥ����.
//2.ÿһ����ɵ�IO����(�Զ���Э���)����֪ͨ������ȥ������Ϣ
//3.SendMsg�Ļ�ֻ�ǰ����ݼӵ�SendMsgList����,Ȼ���ж��Ƿ�ȥPostһ��completionstatu
//4.ÿһ���ͻ��ˣ����Զ���Ƿ�Ͽ��������رգ���������

///////////////////////////////////////////////////////////////////////////////////////

#define PACKET_CHECK					0x534E4942
#define REMOTE_AND_LOCAL_ADDR_LEN		(2 * (sizeof(sockaddr)+16))
#define IDENTITY_LEN					4

#define HEART_BEAT		0x00abcdef

class CPacket;
struct CPacketHeader;
class CClientContext;
class CManager;
//----------------------------------------------------------------

#define IO_ESTABLISHED		1
#define IO_READ				2
#define IO_WRITE			4
#define IO_CLOSE			8

//--------------------------------------------------------

struct OVERLAPPEDPLUS
{
	OVERLAPPED	m_overlapped;
	DWORD		m_IOtype;
	BOOL		m_bManualPost;			//������¼�Ƿ�Ϊ�ֶ�������

	OVERLAPPEDPLUS(DWORD IO_TYPE,BOOL bManualPost = FALSE)
	{
		m_IOtype = IO_TYPE;
		m_bManualPost = bManualPost;
		m_overlapped.hEvent = 0;
		m_overlapped.Internal = 0;
		m_overlapped.InternalHigh = 0;
		m_overlapped.Offset = 0;
		m_overlapped.OffsetHigh = 0;
		m_overlapped.Pointer = 0;
	}
};



//CIOCPֻӦ����һ��ʵ��
class CIOCPServer
{
public:
	static LPFN_ACCEPTEX	lpfnAcceptEx;
private:
	//log
	//�ϴ���������ʵ��.
	volatile DWORD			m_ReadSpeed;
	volatile DWORD			m_WriteSpeed;
	volatile unsigned int	m_SpeedLock;
	
	//Listen Socket
	SOCKET			m_ListenSocket;
	//AcceptSocket=================================================================
	HANDLE			m_hStopRunning;								//֪ͨ�Ѿ�ֹͣ���з�����

	volatile SOCKET	m_AcceptSocket;								//���ڽ��տͻ��˵�socket

	char			m_AcceptBuf[REMOTE_AND_LOCAL_ADDR_LEN + IDENTITY_LEN];		//���ڴ�����յ��Ŀͻ��˵ĵ�ַ
	//==============================================================================
	//Context Manager:
	friend class CClientContext;
	CPtrList*		m_pFreeContextList;
	CPtrList*		m_pClientContextList;						//����clients
	CRITICAL_SECTION m_csContext;
	//==============================================================================
	//IOCP
	DWORD					m_BusyCount;							//�Ѿ���ȡIO�¼����߳���
	DWORD					m_MaxConcurrent;						//IOCP��󲢷�����
	DWORD					m_MaxThreadCount;						//�߳�����м���.

	//Ϊ�˷�ֹ�����߳��ڴ����¼���ʱ��ȴ�Write�¼����û������ȥ����Write���γ�����,��Ҫ���߳��þ���ʱ����������߳�
	//SendMessage,WaitForSingleObject,GetMessage,SuspendThread������߳�.
	//key	ThreadId;
	//value ThreadHandle;
	CMapPtrToPtr*		m_pThreadList;							//ID----Handle
	CRITICAL_SECTION	m_csThread;
	//==============================================================================
	HANDLE			m_hCompletionPort;							//��ɶ˿ھ��
	volatile ULONG	m_AssociateClientCount;						//��¼����ɶ˿ڳɹ������Ŀͻ��˵ĸ���
	//==============================================================================
	friend class	CManager;
	CManager*		m_pManager;
	//
	void UpdateSpeed(DWORD io_type, DWORD TranferredBytes);
	//�ӵ�list����
	void AddToList(CClientContext*pContext);
	//�Ƴ�
	BOOL RemoveFromList(CClientContext*pContext);
	//��������
	void OnAcceptComplete(DWORD dwFailedReason);
	//�����Ѿ���ɵ�IO��Ϣ
	void HandlerIOMsg(CClientContext*pClientContext, DWORD nTransferredBytes, DWORD IoType, DWORD dwFailedReason);
	




	//Ͷ��һ��Accept����
	void PostAccept();
	//Ͷ��һ��ReadIO����
	void PostRead(CClientContext*pClientContext);
	//Ͷ��һ��WriteIO����
	void PostWrite(CClientContext*pClientContext);
	//����һ��ClientContext,�Ͽ�����ʱ���Զ���list���Ƴ��������ͷ���Ҫ�ֶ�����
	CClientContext* AllocateContext(SOCKET ClientSocket);
	//Ҫȷ����FreeContext���ú󲻻�������صĲ���
	void FreeContext(CClientContext*pContext);
	//
	static unsigned int __stdcall WorkerThread(void*);
	//ֻ������һ��ʵ��

	CIOCPServer(HWND hWnd);
	~CIOCPServer();
	//֪ͨ�����ڴ�����Ӧ��Handler
	HWND		m_hNotifyWnd;
	static CIOCPServer* hInstance;
	static void CIOCPServer::async_svr_ctrl_proc(DWORD dwParam);
	
	
public:
	static BOOL SocketInit();						//��ʼ��WSASocket
	static void SocketTerm();						//����WSASocket
	
#define WM_IOCPSVR_START	(WM_USER+101)
#define WM_IOCPSVR_CLOSE	(WM_USER+102)

	//������������Ӧ����ͬһ���߳�����ִ��
	//-------------------------------------------------------------------------------
	DWORD GetWriteSpeed();
	DWORD GetReadSpeed();
	//����������
	BOOL StartServer(USHORT Port);
	//�رշ�����
	void StopServer();
	//����StartSvr �� StopSvr��ҪһЩʱ��,�����첽����
	void AsyncStartSrv(USHORT Prot);
	//
	void AsyncStopSrv();
	//-------------------------------------------------------------------------------
	CManager * GetMsgManager(){
		return m_pManager;
	}

	static CIOCPServer* CreateServer(HWND hNotifyWnd);
	static void DeleteServer();
};

