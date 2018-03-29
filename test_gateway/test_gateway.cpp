#include "xx_uv.h"
#include "RPC_class.h"


bool running = true;
const uint32_t USER_ID = 10000;
const int GAMESERVER_ID = 100;
const int GATESERVER_ID = 1;


//Gate相关
class GameSrv : public xx::Object
{
public:
	xx::UvTcpPeer* peer = nullptr;
	uint16_t id = 0;
	uint32_t numOnlines = 0;

	GameSrv::GameSrv(xx::MemPool* mp)
		: xx::Object(mp)
	{
	}
};
using GameSrv_p = xx::Ptr<GameSrv>;
class GameSrvMgr : public xx::Object
{
public:
	std::unordered_map<uint16_t, xx::Ptr<GameSrv>> pool;

	GameSrvMgr::GameSrvMgr(xx::MemPool* mp)
		: xx::Object(mp)
	{
	}

	void Add(uint16_t id, xx::UvTcpPeer* peer)
	{
		auto it = pool.find(id);
		if (it == pool.end())
		{
			auto gs = mempool->MPCreatePtr<GameSrv>();
			gs->peer = peer;
			pool[id] = std::move(gs);
		}
	}
	xx::Ptr<GameSrv> Get(uint16_t id)
	{
		auto it = pool.find(id);
		if (it == pool.end())
		{
			return xx::Ptr<GameSrv>();
		}
		return it->second;
	}
	xx::Ptr<GameSrv> GetFree()
	{
		for (auto& x : pool)
		{//测试
			return x.second;
		}
		return xx::Ptr<GameSrv>();
	}
};
using GameSrvMgr_p = xx::Ptr<GameSrvMgr>;
GameSrvMgr_p gameSrvMgr;
//
class Client :public  xx::UvTcpPeer
{
public:
	uint32_t id = 0;
	Client::Client(xx::UvTcpListener& listener) :xx::UvTcpPeer(listener)
	{
	}
	void DisconnectImpl() override
	{
		std::cout << "player disconnect.peer id:" << id << std::endl;
		Release();
	}
};

class ClientMgr : public xx::Object
{
public:
	std::unordered_map<uint64_t, Client*> pool;
	ClientMgr::ClientMgr(xx::MemPool* mp)
		: xx::Object(mp)
	{
	}
	uint64_t peerIndex = 0;
	void Add(Client* peer)
	{
		peer->id = ++peerIndex;
		//uint64_t id = peer->memHeader().versionNumber;
		std::cout << "player connect.peer id:" << peer->id << std::endl;
		auto it = pool.find(peer->id);
		if (it == pool.end())
		{
			pool[peer->id] = peer;
		}
	}
	Client* Get(uint64_t id)
	{
		auto it = pool.find(id);
		if (it == pool.end())
		{
			return nullptr;
		}
		return it->second;
	}
	void Remove(Client* peer) 
	{
		auto it = pool.find(peer->id);
		if (it == pool.end())
			return;
		pool.erase(it);
	}
};
using ClientMgr_p = xx::Ptr<ClientMgr>;
ClientMgr_p clientMgr;

//----------------------------------------------------------------------------------------------------
//Gate消息处理
class GateProtocolParser : public xx::Object
{
	typedef bool(GateProtocolParser::*OnMsgFunc)(xx::UvTcpPeer* peer, xx::BBuffer& bb);
	std::array<OnMsgFunc, 0x000f> msgHandles;
	bool GSPing(xx::UvTcpPeer* peer, xx::BBuffer& bb)
	{
		RPC::Generic::Ping_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			//std::cout << "game server ping..........." << std::endl;
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
			std::cout << "game server register.id:" << recvpkg->serviceId << std::endl;
			::gameSrvMgr->Add(recvpkg->serviceId, peer);
			mempool->MPCreateTo(recvpkg);
			recvpkg->serviceId = GATESERVER_ID;
			peer->Send(recvpkg);
		}
		else
		{

		}
		return true;
	};
public:
	GateProtocolParser(xx::MemPool* _mp)
		: Object(_mp)
	{
		msgHandles[xx::TypeId_v<RPC::Generic::Ping>] = &GateProtocolParser::GSPing;
		msgHandles[xx::TypeId_v<RPC::Generic::ServiceInfo>] = &GateProtocolParser::GSRegsiter;
	}
	void Parse(xx::UvTcpPeer* peer, xx::BBuffer& bb) 
	{
		auto offset = bb.offset;
		uint32_t id = 0; bb.Read(id);
		bb.offset = offset;
		bool bRet = (this->*msgHandles[id])(peer, bb);
	}
	void ParseRoutingPackage(xx::UvTcpPeer* peer, xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen) 
	{
		Client *pClient = dynamic_cast<Client *>(peer);

		uint64_t peerid = pClient->id;
		bb.offset -= ddrLen;
		uint32_t gsId = 0;bb.Read(gsId);
		auto pSrv = ::gameSrvMgr->Get(gsId);
		if (pSrv)
		{
			memset(bb.buf + 3, 0, ddrLen);
			bb.WriteAt(pkgOffset + 3, peerid);
			pSrv->peer->SendBytes(bb.buf,bb.dataLen);
		}
	}
	void ParseRoutingPackageFromGS(xx::UvTcpPeer* peer, xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen)
	{	
		//
		xx::BBuffer bbSend(mempool);
		bbSend.Resize(bb.dataLen );
		memcpy(bbSend.buf, bb.buf, bb.dataLen);
		bbSend.offset = 3;
		bbSend.dataLen = bb.dataLen;
		uint64_t gateUserId = 0;
		bbSend.Read(gateUserId);
		xx::UvTcpPeer* pSrv = ::clientMgr->Get(gateUserId);
		if (pSrv)
		{			
			uint32_t ntypeid = 0;
			memcpy(bbSend.buf, &ntypeid, 1);
			memmove(bbSend.buf + 3, bbSend.buf + 3+ ddrLen, bbSend.dataLen-ddrLen-3);
			bbSend.dataLen -= ddrLen;
			pSrv->SendBytes(bbSend.buf, bbSend.dataLen);
		}
	}
};
using GateProtocolParser_p = xx::Ptr<GateProtocolParser>;
GateProtocolParser_p gateProtocolParser;
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
		xx::UvTcpPeer* peer = nullptr;
		mp.CreateTo(peer, *gslistener);
		return peer;
	};
	gslistener->OnAccept = [&](xx::UvTcpPeer* peer) {
		peer->OnReceiveRoutingPackage = [peer](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen) {
			::gateProtocolParser->ParseRoutingPackageFromGS(peer, bb,pkgOffset,pkgLen,addrOffset,ddrLen);
		};
		peer->OnReceivePackage = [peer](xx::BBuffer& bb) {
			::gateProtocolParser->Parse(peer, bb);
		};
	};
	//listen client
	auto listener = loop.CreateTcpListener();
	listener->Bind("0.0.0.0", 12345);//
	listener->Listen();
	listener->OnCreatePeer = [&]()->xx::UvTcpPeer*{
		Client* peer = nullptr;
		mp.CreateTo(peer, *listener);
		return peer;
	};
	uint64_t nPeerId = 1;
	listener->OnAccept = [&nPeerId](xx::UvTcpPeer* peer){
		//
		Client *pClient = dynamic_cast<Client *>(peer);
		clientMgr->Add(pClient);
		//
		peer->OnReceivePackage = [peer](xx::BBuffer& bb){
			::gateProtocolParser->Parse(peer, bb);
		};
		peer->OnReceiveRequest = [peer](uint32_t serial, xx::BBuffer& bb){
		};
		peer->OnReceiveRoutingPackage = [peer](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen){
			::gateProtocolParser->ParseRoutingPackage(peer, bb, pkgOffset,pkgLen,addrOffset, ddrLen);
		};
		peer->OnDispose = [peer]() {
			//Client *pClient = dynamic_cast<Client *>(peer);
			//clientMgr->Remove(pClient);
		};
	};
	loop.Run();
}
int main()
{	
	//
	xx::MemPool::RegisterInternal();
	RPC::AllTypesRegister();
	xx::MemPool mp;
	mp.MPCreateTo(::clientMgr);
	mp.MPCreateTo(::gameSrvMgr);
	mp.MPCreateTo(::gateProtocolParser);
	//
	//GateProtocolParser* pp = new GateProtocolParser(&mp);
	//pp->Release();
	//
	f1();
	//---------------------------------------------------
	std::cin.get();
	return 0;
}
