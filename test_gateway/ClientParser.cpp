#include "ClientParser.h"
#include "RPC_class.h"
#include "Router.h"
namespace protocol
{
	bool ClientParser::Ping(xx::UvTcpPeer* peer, xx::BBuffer& bb)
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
	void ClientParser::Route(xx::UvTcpPeer* peer, const xx::BBuffer& bb)
	{
		std::string guid(peer->recvingAddress.c_str()+1);
		auto dataLen = bb.dataLen - bb.offset;
		Client* pClient = dynamic_cast<Client*>(peer);
		std::string sid = std::to_string(pClient->id);
		xx::String ptrsid(mempool, sid.c_str());
		xx::BBuffer bbSend(mempool);			
		bbSend.Reserve(bbSend.dataLen+3+ptrsid.dataLen+1);
		bbSend.buf[0] = 8 | ptrsid.dataLen << 4;
		bbSend.dataLen += 3;
		bbSend.Write(ptrsid);
		memcpy(bbSend.buf+ bbSend.dataLen, bb.buf+bb.offset, dataLen);
		bbSend.dataLen += dataLen;
		auto pkgLen = bbSend.dataLen - 3;
		memcpy(bbSend.buf + bbSend.dataLenBak + 1, &pkgLen, 2);
		GameService* pService = Router::Instance()->GetGameService(guid);
		if (pService)
			pService->SendBytes(bbSend.buf, bbSend.dataLen);
	}
}