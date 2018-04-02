#pragma once
#include "xx_uv.h"
#include "RPC_class.h"

class ClientPeer :public  xx::UvTcpPeer
{
public:
	uint32_t id = 0;
	ClientPeer::ClientPeer(xx::UvTcpListener& listener) :xx::UvTcpPeer(listener)
	{
	}
	void DisconnectImpl() override
	{
		std::cout << "player disconnect.peer id:" << id << std::endl;
		Release();
	}
};

class ClientPeerMgr : public xx::Object
{	
public:
	std::unordered_map<uint64_t, ClientPeer*> pool;
	ClientPeerMgr::ClientPeerMgr(xx::MemPool* mp)
		: xx::Object(mp)
	{
	}
	uint64_t peerIndex = 0;
	void Add(ClientPeer* peer);
	ClientPeer* Get(uint64_t id);
	void Remove(ClientPeer* peer);
};
using ClientPeerMgr_p = xx::Ptr<ClientPeerMgr>;
