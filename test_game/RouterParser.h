//解析客户端消息
//解析游戏消息
#include "xx_uv.h"
#include "RPC_class.h"

namespace protocol
{
	class RouterParser : public xx::TSingleton<RouterParser>
	{
		friend class xx::TSingleton<RouterParser>;
		typedef bool(RouterParser::*OnMsgFunc)(xx::UvTcpPeer* peer, xx::BBuffer& bb, xx::String& gateUserId);
		std::array<OnMsgFunc, 0x000f> msgHandles;
		bool Ping(xx::UvTcpPeer* peer, xx::BBuffer& bb);
		bool PlayerLogin(xx::UvTcpPeer* peer, xx::BBuffer& bb, xx::String& gateUserId);
	public:
		xx::MemPool* mempool = nullptr;
		RouterParser()
		{
			msgHandles[xx::TypeId_v<RPC::Client_Login::Login>] = &RouterParser::PlayerLogin;
		}
		void Parse(xx::UvTcpPeer* peer, xx::BBuffer& bb)
		{
		}
		void Route(xx::UvTcpPeer* peer, xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t addrLen);
	};
}

