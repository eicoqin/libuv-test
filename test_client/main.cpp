#include "xx_uv.h"
#include "RPC_class.h"


bool running = true;
const uint32_t USER_ID = 10000;
const uint64_t GAMESERVER_ID = 100;
const uint32_t GATESERVER_ID = 1;




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
	client->SetAddress("127.0.0.1", 12345);
	client->Connect();
	//
	bool bLogining = false;

	//
	auto timer2 = loop.CreateTimer(1000, 1000, [&loop, client, &mp, &bLogining]()
	{
		if (client->state == xx::UvTcpStates::Connected) {
			if (bLogining == false)
			{
				RPC::Client_Login::Login_p login;
				mp.MPCreateTo(login);
				mp.MPCreateTo(login->password);
				mp.MPCreateTo(login->username);
				login->password->Assign("123");
				login->username->Assign("123");
				client->SendRoutePackage(login, GAMESERVER_ID, sizeof(GAMESERVER_ID));
				std::cout << "logining........." << std::endl;
				//bLogining = true;
			}
			//
			RPC::Generic::Ping_p ping;
			mp.MPCreateTo(ping);
			ping->ticks = time(NULL);
			client->Send(ping);
			std::cout << "ping........." << std::endl;
			//
		}
		if (!running) loop.Stop();
	});
	client->OnDispose = [client]() {
		std::cout << "gate disconnect....." << std::endl;
	};
	client->OnReceiveRequest = [&](uint32_t serial, xx::BBuffer& bb)
	{
		std::cout << "OnReceiveRequest:" << bb << std::endl;
	};
	client->OnReceivePackage = [client, &bLogining](xx::BBuffer& bb) {
		uint32_t id = 0;
		auto offsetbak = bb.offset;
		bb.Read(id);
		bb.offset = offsetbak;
		switch (id) {
		case xx::TypeId_v<RPC::Login_Client::LoginSuccess>: {
			RPC::Login_Client::LoginSuccess_p loginAck;
			int r = bb.ReadPackage(loginAck);
			if (r == 0) {
				std::cout << "player login result:" << loginAck->id << std::endl;
				//bLogining = true;
			}
			else
			{

			}
		}break;
		}
	};

	client->OnReceiveRoutingPackage = [client, &bLogining](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t ddrLen) {

		uint32_t id = 0;
		bb.Read(id);
		switch (id)
		{
		case xx::TypeId_v<RPC::Login_Client::LoginSuccess>:
		{
			RPC::Login_Client::LoginSuccess_p loginAck;
			int r = bb.ReadPackage(loginAck);
			if (r == 0) {
				std::cout << "player login result:" << loginAck->id << std::endl;
				//bLogining = true;
			}
			else
			{

			}

		}break;
		}


	};


	loop.Run();
}
int main()
{
	xx::MemPool::RegisterInternal();
	RPC::AllTypesRegister();

	xx::MemPool mp;

	//
	//
	f1();
	//---------------------------------------------------

	std::cin.get();
	return 0;
}
