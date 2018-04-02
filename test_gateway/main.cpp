#include "xx_uv.h"
#include "RPC_class.h"
#include "Router.h"
#include "ClientParser.h"
#include "GameServerParser.h"
int main()
{
	//
	xx::MemPool::RegisterInternal();
	RPC::AllTypesRegister();
	xx::MemPool mp;
	xx::UvLoop loop(&mp);
	protocol::ClientParser::Instance()->mempool = loop.mempool;
	protocol::GameServerParser::Instance()->mempool = loop.mempool;

	Router::Instance()->Init(&loop);
	loop.Run();
	//---------------------------------------------------
	std::cin.get();
	return 0;
}
