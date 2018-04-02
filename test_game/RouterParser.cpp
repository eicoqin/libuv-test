#include "RouterParser.h"
#include "RPC_class.h"

namespace protocol
{
	bool RouterParser::Ping(xx::UvTcpPeer* peer, xx::BBuffer& bb)
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
	void RouterParser::Route(xx::UvTcpPeer* peer, xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t addrLen)
	{
		auto offsetbak = bb.offset;
		bb.offset -= addrLen;
		xx::String gateUserId(mempool);
		bb.Read(gateUserId);
		bb.offset = offsetbak;
		uint32_t id = 0; bb.Read(id);
		bb.offset = offsetbak;
		bool bRet = (this->*msgHandles[id])(peer, bb, gateUserId);
	}

	bool RouterParser::PlayerLogin(xx::UvTcpPeer* peer, xx::BBuffer& bb, xx::String& gateUserId)
	{
		RPC::Client_Login::Login_p recvpkg;
		int r = bb.ReadPackage(recvpkg);
		if (r == 0) {
			RPC::Login_Client::LoginSuccess_p loginAck;
			mempool->MPCreateTo(loginAck);
			loginAck->id = 0;
			peer->SendRouting(gateUserId,loginAck);
			std::cout << "player login.gateUserId:" << gateUserId << std::endl;
		}
		else
		{

		}
		return true;
	};
}