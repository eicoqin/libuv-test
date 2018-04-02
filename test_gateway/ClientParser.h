//解析客户端消息
//解析游戏消息
#include "xx_uv.h"
#include "RPC_class.h"
#include "Router.h"
namespace protocol
{
	class ClientParser : public xx::TSingleton<ClientParser>
	{
		friend class xx::TSingleton<ClientParser>;
		typedef bool(ClientParser::*OnMsgFunc)(xx::UvTcpPeer* peer, xx::BBuffer& bb);
		std::array<OnMsgFunc, 0x000f> msgHandles;
		bool Ping(xx::UvTcpPeer* peer, xx::BBuffer& bb);

	public:
		xx::MemPool* mempool = nullptr;
		ClientParser()
		{
			msgHandles[xx::TypeId_v<RPC::Generic::Ping>] = &ClientParser::Ping;
		}
		void Parse(xx::UvTcpPeer* peer, xx::BBuffer& bb)
		{
			auto offset = bb.offset;
			uint32_t id = 0; bb.Read(id);
			bb.offset = offset;
			bool bRet = (this->*msgHandles[id])(peer, bb);
		}
		void Route(xx::UvTcpPeer* peer, const xx::BBuffer& bb);
	};
}

