#include "GameServerParser.h"
#include "Router.h"

namespace protocol
{

	bool GameServerParser::Ping(xx::UvTcpClient* peer, xx::BBuffer& bb)
	{
		RPC::Generic::Ping_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
		}
		else
		{

		}
		return true;
	};
	bool GameServerParser::Regsiter(xx::UvTcpClient* peer, xx::BBuffer& bb)
	{		
		RPC::Generic::ServiceInfo_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			GameService* pService = dynamic_cast<GameService*>(peer);			
			std::cout << "game server register,service id:   " << recvpkg->serviceId << std::endl;
			std::string guid(recvpkg->serviceId->c_str());
			pService->strGuid = guid;
			Router::Instance()->AddGameService(guid, pService);
		}
		else
		{

		}
		return true;
	};
	void GameServerParser::Route(xx::UvTcpClient* peer, const xx::BBuffer& bb)
	{
		//返回正常包
		std::string routeUserId(peer->recvingAddress.c_str() + 1);
		auto dataLen = bb.dataLen - bb.offset;		
		xx::BBuffer bbSend(mempool);
		bbSend.Reserve(bbSend.dataLen + 3 + dataLen);
		bbSend.buf[0] = 0;
		bbSend.dataLen += 3;
		memcpy(bbSend.buf + bbSend.dataLen, bb.buf + bb.offset, dataLen);
		bbSend.dataLen += dataLen;
		auto pkgLen = bbSend.dataLen - 3;
		memcpy(bbSend.buf + bbSend.dataLenBak + 1, &pkgLen, 2);
		uint32_t iid = std::atoi(routeUserId.c_str());
		Client* pClient = Router::Instance()->GetClient(iid);
		if (pClient)
			pClient->SendBytes(bbSend.buf, bbSend.dataLen);
	}
}


