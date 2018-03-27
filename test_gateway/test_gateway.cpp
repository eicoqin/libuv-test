#include "xx_uv.h"
#include "RPC_class.h"
bool running = true;
const int USER_ID = 10000;
const int GAMESERVER_ID = 100;
const int GATESERVER_ID = 1;
//Gate相关
class GameSrv  {
public:
	GameSrv(){
		nSrvID = 0;
		nCulOnline = 0;
	}
	xx::UvTcpPeer* pTcpPeer;
	uint16_t nSrvID;
	uint32_t nCulOnline;
};
//using GameSrv_p = xx::Ptr<GameSrv>;
class GameSrvMgr :public xx::Object {
public:
	GameSrvMgr::GameSrvMgr(xx::MemPool* _mp) :Object(_mp) {
		pMP = _mp;
	}
	xx::MemPool* pMP;
	std::unordered_map<uint16_t, GameSrv*> gameSrvPool;
	void AddInGameSrvPool(uint16_t nSrvID, xx::UvTcpPeer* pTcpPeer) {
		std::unordered_map<uint16_t, GameSrv*>::const_iterator it = gameSrvPool.find(nSrvID);
		if (it == gameSrvPool.end()) {
			//
			GameSrv* pGameSrv = new GameSrv();
			//pMP->MPCreateTo(pGameSrv);
			pGameSrv->pTcpPeer = pTcpPeer;			
			gameSrvPool[nSrvID] = pGameSrv;//.pointer;
			//
		}
	}
	GameSrv* GetGameSrvInPool(uint16_t nSrvID) {
		auto it = gameSrvPool.find(nSrvID);
		if (it == gameSrvPool.end()) {
			return nullptr;
		}
		return it->second;
	}
	GameSrv* GetFreeGameSrvInPool() {
		for (auto& x : gameSrvPool) {//测试
			return x.second;
		}
		return nullptr;
	}
};
using GameSrvMgr_p = xx::Ptr<GameSrvMgr>;
GameSrvMgr_p pGameSrvMgr;
//消息处理函数
//----------------------------------------------------------------------------------------------------
//Gate消息处理
class GateProtocolParser:public xx::Object {	
	typedef bool(GateProtocolParser::*OnMsgFunc)(xx::UvTcpPeer* peer, xx::BBuffer& bb);
	OnMsgFunc msgHandles[0xffff] = { nullptr };
	xx::MemPool* pMsgMP;
	bool GSPing(xx::UvTcpPeer* peer, xx::BBuffer& bb)
	{
		RPC::Generic::Ping_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {

			std::cout << "recieve game server ping:  " << recvpkg->ticks << std::endl;
		}
		else
		{

		}
		return true;
	};
	bool GSRegsiter(xx::UvTcpPeer* peer, xx::BBuffer& bb)
	{
		RPC::Generic::ServiceInfo_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {	
			pGameSrvMgr->AddInGameSrvPool(recvpkg->serviceId, peer);
			std::cout << "recieve game server register.id:  " << recvpkg->serviceId << ",type:" << (int)recvpkg->type << std::endl;
			RPC::Generic::ServiceInfo_p serviceInfo;
			pMsgMP->MPCreateTo(serviceInfo);
			serviceInfo->serviceId = GATESERVER_ID;
			peer->Send(serviceInfo);
		}
		else
		{

		}
		return true;
	};	
	bool PlayerLogin_CliReq(xx::UvTcpPeer* peer, xx::BBuffer& bb)
	{
		RPC::Client_Login::Login_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			//收到前端登录游戏请求，修改包头，路由转发
			GameSrv* pSrv = pGameSrvMgr->GetFreeGameSrvInPool();
			if (pSrv) {
				pSrv->pTcpPeer->SendRoutePackage(recvpkg, USER_ID,4);
			}
		}
		else
		{

		}
		return true;
	};
	bool PlayerLogin_GSAck(xx::UvTcpPeer* peer, xx::BBuffer& bb)
	{
		RPC::Login_Client::LoginSuccess_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			//收到游戏登录返回请求，修改包头，路由转发
			std::cout << "recieve game server data.player login ack" << std::endl;
		}
		else
		{

		}
		return true;
	};
public:
	GateProtocolParser(xx::MemPool* _mp) :Object(_mp) {
		pMsgMP = _mp;
	}
	void Init() {
		msgHandles[xx::TypeId_v<RPC::Generic::Ping>] = &GateProtocolParser::GSPing;
		msgHandles[xx::TypeId_v<RPC::Generic::ServiceInfo>] = &GateProtocolParser::GSRegsiter;
		msgHandles[xx::TypeId_v<RPC::Client_Login::Login>] = &GateProtocolParser::PlayerLogin_CliReq;
		msgHandles[xx::TypeId_v<RPC::Login_Client::LoginSuccess>] = &GateProtocolParser::PlayerLogin_GSAck;
	}
	void Parse(xx::UvTcpPeer* peer, xx::BBuffer& bb) {
		auto offset = bb.offset;
		uint32_t id = 0;bb.Read(id);
		bb.offset = offset;
		bool bRet = (this->*msgHandles[id])(peer, bb);
	}
	void ParseRoutingPackage(xx::UvTcpPeer* peer, xx::BBuffer& bb) {
		auto offset = bb.offset;
		uint32_t id = 0;bb.Read(id);
		bb.offset = offset;
		bool bRet = (this->*msgHandles[id])(peer, bb);
	}
};
using GateProtocolParser_p = xx::Ptr<GateProtocolParser>;
GateProtocolParser_p pGateProtocolParser;
//----------------------------------------------------------------------------------------------------
//Game消息处理
class GameProtocolParser :public xx::Object {
	typedef bool(GameProtocolParser::*OnMsgFunc)(xx::UvTcpClient* peer, xx::BBuffer& bb);
	OnMsgFunc msgHandles[0xffff] = { nullptr };
	xx::MemPool* pMsgMP;	
	bool GateRegsiter(xx::UvTcpClient* peer, xx::BBuffer& bb)
	{
		RPC::Generic::ServiceInfo_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			std::cout << "recieve gate server register.id:  " << recvpkg->serviceId << ",type:" << (int)recvpkg->type << std::endl;
			

		}
		else
		{

		}
		return true;
	};
	bool GatePlayerLogin(xx::UvTcpClient* peer, xx::BBuffer& bb)
	{
		RPC::Client_Login::Login_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			std::cout << "recieve gate server data.player login"  << std::endl;
			//
			RPC::Login_Client::LoginSuccess_p loginAck;			
			pMsgMP->MPCreateTo(loginAck);
			loginAck->id = 0;
			peer->SendRoutePackage(loginAck, USER_ID,4);
			//
		}
		else
		{

		}
		return true;
	};
public:
	GameProtocolParser(xx::MemPool* _mp) :Object(_mp) {
		pMsgMP = _mp;
	}
	void Init() {
		msgHandles[xx::TypeId_v<RPC::Generic::ServiceInfo>] = &GameProtocolParser::GateRegsiter;
		msgHandles[xx::TypeId_v<RPC::Client_Login::Login>] = &GameProtocolParser::GatePlayerLogin;
	}
	void Parse(xx::UvTcpClient* peer, xx::BBuffer& bb) {
		auto offset = bb.offset;
		uint32_t id = 0;bb.Read(id);
		bb.offset = offset;
		bool bRet = (this->*msgHandles[id])(peer, bb);
	}
	void ParseRoutingPackage(xx::UvTcpClient* peer, xx::BBuffer& bb) {
		auto offset = bb.offset;
		uint32_t id = 0;bb.Read(id);
		bb.offset = offset;
		bool bRet = (this->*msgHandles[id])(peer, bb);
	}
};
using GameProtocolParser_p = xx::Ptr<GameProtocolParser>;
GameProtocolParser_p pGameProtocolParser;
//----------------------------------------------------------------------------------------------------
void f1()
{
	// gate server
	xx::MemPool mp;
	xx::UvLoop loop(&mp);
	loop.InitTimeouter();
	loop.InitKcpFlushInterval(10);
	//
	//listen game server
	auto gslistener = loop.CreateTcpListener();
	gslistener->Bind("0.0.0.0", 12346);//
	gslistener->Listen();
	uint64_t counter = 0;
	gslistener->OnCreatePeer = [&]()->xx::UvTcpPeer* {
		return new xx::UvTcpPeer(*gslistener);
	};
	gslistener->OnAccept = [&](xx::UvTcpPeer* peer) {
		peer->OnReceiveRoutingPackage = [&](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen) {
			std::cout << "route data from game server" << std::endl;
			pGateProtocolParser->Parse(peer, bb);
		};
		peer->OnReceivePackage = [peer](xx::BBuffer& bb) {
			std::cout << "data from game server" << std::endl;
			pGateProtocolParser->Parse(peer, bb);
		};
	};
	//listen client
	auto listener = loop.CreateTcpListener();
	listener->Bind("0.0.0.0", 12345);//
	listener->Listen();	
	listener->OnCreatePeer = [&]()->xx::UvTcpPeer* {
		return new xx::UvTcpPeer(*listener);
	};
	listener->OnAccept = [&](xx::UvTcpPeer* peer) {//从客户端获取数据
		peer->OnReceivePackage = [&](xx::BBuffer& bb) {
			pGateProtocolParser->Parse(peer, bb);
		};
		peer->OnReceiveRequest = [&](uint32_t serial, xx::BBuffer& bb) {
			std::cout << "msg from client:" << bb << std::endl;
		};
	};
	loop.Run();
}
void f2()
{
	// game server
	xx::MemPool mp;
	xx::UvLoop loop(&mp);
	loop.InitTimeouter();
	loop.InitKcpFlushInterval(10);
	bool bRegisted = false;
	//	
	auto client = loop.CreateTcpClient();
	client->SetAddress("127.0.0.1", 12346);
	client->Connect();

	auto timer2 = loop.CreateTimer(1000, 1000, [&loop, client, &mp]()
	{
		if (client->state == xx::UvTcpStates::Connected) {
			std::cout << "ping gatewaty server......." << std::endl;
			RPC::Generic::Ping_p ping;
			mp.MPCreateTo(ping);
			ping->ticks = time(NULL);
			client->Send(ping);
			//
			RPC::Generic::ServiceInfo_p serviceInfo;
			mp.MPCreateTo(serviceInfo);
			serviceInfo->type = RPC::Generic::ServiceTypes::Game;
			serviceInfo->serviceId = GAMESERVER_ID;//临时固定值
			client->Send(serviceInfo);
		}
		if (!running) loop.Stop();
	});
	client->OnReceiveRequest = [&](uint32_t serial, xx::BBuffer& bb)
	{
		std::cout << "OnReceiveRequest:" << bb << std::endl;
	};
	client->OnReceivePackage = [client](xx::BBuffer& bb) {
		std::cout << "data from gate server" << std::endl;
		pGameProtocolParser->Parse(client, bb);
	};
	
	client->OnReceiveRoutingPackage = [&](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen) {
		std::cout << "route data from gate server" << std::endl;
		//
		pGameProtocolParser->Parse(client, bb);
		//
	};


	loop.Run();
}
int main()
{
	xx::MemPool::RegisterInternal();
	RPC::AllTypesRegister();

	xx::MemPool mp;
	
	mp.MPCreateTo(pGateProtocolParser);
	pGateProtocolParser->Init();
	mp.MPCreateTo(pGameProtocolParser);
	pGameProtocolParser->Init();

	mp.MPCreateTo(pGameSrvMgr);
	//处理函数注册 
	//msgHandles[xx::TypeId_v<RPC::Generic::Ping>] = GSPing;
	//msgHandles[xx::TypeId_v<RPC::Generic::ServiceInfo>] = GSRegsiter;
	// ctrl + break 事件处理. ctrl + c, 直接 x 拦截不能
	SetConsoleCtrlHandler([](DWORD CtrlType)->BOOL
	{
		running = false;
		return TRUE;
	}, 1);

	std::thread t(f2);
	t.detach();
	f1();
	//---------------------------------------------------

	int x = 0;
	std::cin >> x;
	return 0;
}
