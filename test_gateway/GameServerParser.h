//解析游戏消息
#include "xx_uv.h"
#include "RPC_class.h"
#include "Router.h"
namespace protocol
{
	class GameServerParser : public xx::TSingleton<GameServerParser>
	{
		friend class xx::TSingleton<GameServerParser>;
		typedef bool(GameServerParser::*OnMsgFunc)(xx::UvTcpClient* peer, xx::BBuffer& bb);
		std::array<OnMsgFunc, 0x000f> msgHandles;
		bool Ping(xx::UvTcpClient* peer, xx::BBuffer& bb);
		bool Regsiter(xx::UvTcpClient* peer, xx::BBuffer& bb);
	public:
		xx::MemPool* mempool = nullptr;
		GameServerParser()
		{
			msgHandles[xx::TypeId_v<RPC::Generic::Ping>] = &GameServerParser::Ping;
			msgHandles[xx::TypeId_v<RPC::Generic::ServiceInfo>] = &GameServerParser::Regsiter;
		}
		void Parse(xx::UvTcpClient* peer, xx::BBuffer& bb)
		{
			auto offset = bb.offset;
			uint32_t id = 0; bb.Read(id);
			bb.offset = offset;
			bool bRet = (this->*msgHandles[id])(peer, bb);
		}
		void Route(xx::UvTcpClient* peer, const xx::BBuffer& bb);
	};
}

