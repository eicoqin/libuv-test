﻿#include "xx_uv.h"
#include "RPC_class.h"
#include "Service.h"
#include "RouterParser.h"


int main()
{
	xx::MemPool::RegisterInternal();
	RPC::AllTypesRegister();
	xx::MemPool mp;
	xx::UvLoop loop(&mp);
	Service* pService = nullptr;
	mp.CreateTo(pService, std::move(loop));
	protocol::RouterParser::Instance()->mempool = loop.mempool;
	pService->Start();
	loop.Run();
	//---------------------------------------------------

	std::cin.get();
	return 0;
}