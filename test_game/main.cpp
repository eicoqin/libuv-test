﻿#include "xx_uv.h"
#include "RPC_class.h"


bool running = true;
const uint32_t USER_ID = 10000;
const uint64_t GAMESERVER_ID = 100;
const uint32_t GATESERVER_ID = 1;


//----------------------------------------------------------------------------------------------------
//Game消息处理
class GameProtocolParser : public xx::Object
{
	typedef bool(GameProtocolParser::*OnMsgFunc)(xx::UvTcpClient* peer, xx::BBuffer& bb, uint64_t gateUserId);
	std::array<OnMsgFunc, 0x000f> msgHandles;
	bool GateRegsiter(xx::UvTcpClient* peer, xx::BBuffer& bb)
	{
		RPC::Generic::ServiceInfo_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {

		}
		else
		{

		}
		return true;
	};
	bool GatePlayerLogin(xx::UvTcpClient* peer, xx::BBuffer& bb, uint64_t gateUserId)
	{
		RPC::Client_Login::Login_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			RPC::Login_Client::LoginSuccess_p loginAck;
			mempool->MPCreateTo(loginAck);
			loginAck->id = 0;
			peer->SendRoutePackage(loginAck, gateUserId, sizeof(gateUserId));

			std::cout << "player login.gateUserId:" << gateUserId << std::endl;
		}
		else
		{

		}
		return true;
	};
public:
	GameProtocolParser(xx::MemPool* _mp)
		: Object(_mp)
	{
		msgHandles[xx::TypeId_v<RPC::Client_Login::Login>] = &GameProtocolParser::GatePlayerLogin;
	}
	void Parse(xx::UvTcpClient* peer, xx::BBuffer& bb) {
		auto offset = bb.offset;
		uint32_t id = 0; bb.Read(id);
		bb.offset = offset;
		switch (id)
		{
		case xx::TypeId_v<RPC::Generic::ServiceInfo>:
		{
			GateRegsiter(peer, bb);
		}break;
		}
	}
	void ParseRoutingPackage(xx::UvTcpClient* peer, xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen)
	{
		auto offsetbak = bb.offset;
		bb.offset -= ddrLen;
		uint64_t gateUserId = 0;bb.Read(gateUserId);
		bb.offset = offsetbak;
		uint32_t id = 0; bb.Read(id);
		bb.offset = offsetbak;
		bool bRet = (this->*msgHandles[id])(peer, bb, gateUserId);
		//
	}

};
using GameProtocolParser_p = xx::Ptr<GameProtocolParser>;
GameProtocolParser_p gameProtocolParser;
//----------------------------------------------------------------------------------------------------




void f1()
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
	auto timer2 = loop.CreateTimer(1000, 1000, [&loop, client, &mp, &bRegisted]()
	{
		if (client->state == xx::UvTcpStates::Connected) {
			RPC::Generic::Ping_p ping;
			mp.MPCreateTo(ping);
			ping->ticks = time(NULL);
			client->Send(ping);
			//
			if (bRegisted == false)
			{
				bRegisted = true;
				RPC::Generic::ServiceInfo_p serviceInfo;
				mp.MPCreateTo(serviceInfo);
				serviceInfo->type = RPC::Generic::ServiceTypes::Game;
				serviceInfo->serviceId = GAMESERVER_ID;//临时固定值
				client->Send(serviceInfo);
			}

		}
		if (!running) loop.Stop();
	});
	client->OnReceiveRequest = [client](uint32_t serial, xx::BBuffer& bb)
	{
	};
	client->OnReceivePackage = [client](xx::BBuffer& bb)
	{
		::gameProtocolParser->Parse(client, bb);
	};
	client->OnReceiveRoutingPackage = [client](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen)
	{
		::gameProtocolParser->ParseRoutingPackage(client, bb, pkgOffset, pkgLen, addrOffset, ddrLen);
	};
	client->OnDispose = [client]() {
		std::cout << "gate disconnect....." << std::endl;
	};
	loop.Run();
}
int main()
{
	xx::MemPool::RegisterInternal();
	RPC::AllTypesRegister();
	xx::MemPool mp;
	mp.MPCreateTo(::gameProtocolParser);
	//
	//
	f1();
	//---------------------------------------------------

	std::cin.get();
	return 0;
}
