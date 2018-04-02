#include "Router.h"
#include "GameServerParser.h"
#include "ClientParser.h"
#include <tchar.h>
#include <atlstr.h>

Router::Router() 
{
	
}
Router::~Router()
{
	//todo
}
void Router::Init(xx::UvLoop* _loop)
{
	//
	TCHAR buffer[1024] = { 0 };
	GetModuleFileName(GetModuleHandle(NULL), buffer, sizeof(buffer));
	CString wFileName = CString(buffer);
	CString ROOTPATH = wFileName.Left(wFileName.ReverseFind('\\'));
	ROOTPATH += "\\router.ini";
	const static DWORD MAX_SECTIONS = 1024;
	TCHAR sectionNames[MAX_SECTIONS] = { _T("") };
	uint32_t sectionCount = GetPrivateProfileSectionNames(sectionNames, MAX_SECTIONS, ROOTPATH);
	//
	int nPort = GetPrivateProfileInt(_T("CONFIG"), _T("PORT"), 12345, ROOTPATH);
	std::cout << "listen port:" << nPort << std::endl;
	//
	loop = _loop;
	mempool = loop->mempool;
	clientListener = loop->CreateTcpListener();
	clientListener->Bind("0.0.0.0", nPort);
	clientListener->Listen();
	clientListener->OnCreatePeer = [&]()->xx::UvTcpPeer* {
		Client* peer = mempool->Create<Client>(*clientListener);
		AddClient(peer);
		return peer;
	};
	clientListener->OnAccept = [this](xx::UvTcpPeer *peer)
	{
		peer->OnReceivePackage = [peer](xx::BBuffer& bb,uint8_t typeId) {
			if ((typeId & 8) > 0)//直接转发
			{
				protocol::ClientParser::Instance()->Route(peer, bb);
			}
			else
			{
				protocol::ClientParser::Instance()->Parse(peer, bb);
			}
		};
		peer->OnReceiveRequest = [peer](uint32_t serial, xx::BBuffer& bb,uint8_t typeId) {
		};
		peer->OnReceiveRouting = [peer](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen) {
			std::cout << "OnReceiveRouting" << std::endl;
		};
		peer->OnDispose = [peer]() {
		};
	};
	timer = loop->CreateTimer(1000, 1000, [this] { Update(); });

	//读取配置 连接各个gameserver
	//todo
	for (int i = 0; i < sectionCount; i++)
	{
		int n = _tcsncmp(&sectionNames[i], _T("GS"), _tcslen(_T("GS")));
		if (n == 0) {
			int gamePort = GetPrivateProfileInt(&sectionNames[i], _T("PORT"), 0, ROOTPATH);
			ConectToGameServer("127.0.0.1", gamePort);			
		}
		while (sectionNames[i] != 0) { i++; }
	}
}
//-------------------------------------------------------------
void Router::ConectToGameServer(const std::string& strIp, uint16_t nPort)
{
	std::cout << "connect to game server." << strIp.c_str() << ":" << nPort << std::endl;
	GameService* gsclient = mempool->Create<GameService>(*loop);
	gsclient->SetAddress(strIp.c_str(), nPort);
	gsclient->Connect();
	gsclient->OnReceiveRequest = [gsclient](uint32_t serial, xx::BBuffer& bb, uint8_t typeId)
	{
	};
	gsclient->OnReceivePackage = [gsclient](xx::BBuffer& bb, uint8_t typeId)
	{		

		if ((typeId & 8) > 0)//直接转发
		{
			protocol::GameServerParser::Instance()->Route(gsclient, bb);
		}
		else
		{
			protocol::GameServerParser::Instance()->Parse(gsclient, bb);
		}

		
	};
	gsclient->OnReceiveRouting = [gsclient](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t addrLen)
	{
		//protocol::GameServerParser::Instance()->ParseRouting(gsclient, bb,pkgOffset, pkgLen,addrOffset, addrLen);
	};
	gsclient->OnDispose = [gsclient]() {
		std::cout << "gs disconnect....." << std::endl;
	};
}
void Router::Update()
{

}
//-------------------------------------------------------------
void Router::AddClient(Client* peer)
{
	peer->id = ++peerIndex;
	std::cout << "player connect.peer id:" << peer->id << std::endl;
	auto it = clientpool.find(peer->id);
	if (it == clientpool.end())
	{
		clientpool[peer->id] = peer;
	}
}
Client* Router::GetClient(uint32_t id)
{
	auto it = clientpool.find(id);
	if (it == clientpool.end())
	{
		return nullptr;
	}
	return it->second;
}
void Router::RemoveClient(Client* peer)
{
	auto it = clientpool.find(peer->id);
	if (it == clientpool.end())
		return;
	clientpool.erase(it);
}
void Router::AddGameService(const std::string& strGuid, GameService* peer)
{
	auto it = serviceClients.find(strGuid);
	if (it == serviceClients.end())
	{
		serviceClients[strGuid] = peer;
	}
}
GameService* Router::GetGameService(const std::string& strGuid)
{
	auto it = serviceClients.find(strGuid);
	if (it != serviceClients.end())
	{
		return it->second;
	}
	//临时测试 没有找到随机返回一个
	int n = rand() % serviceClients.size();
	it = serviceClients.begin();
	for (int i = 0;i < n;++i)
		it++;
	return it->second;
	//return nullptr;
}
void Router::RemoveGameService(GameService* peer)
{
	auto it = serviceClients.find(peer->strGuid);
	if (it == serviceClients.end())
		return;
	serviceClients.erase(it);
}