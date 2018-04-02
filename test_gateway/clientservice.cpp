#include "clientservice.h"

void ClientPeerMgr::Add(ClientPeer* peer)
{
	peer->id = ++peerIndex;
	std::cout << "player connect.peer id:" << peer->id << std::endl;
	auto it = pool.find(peer->id);
	if (it == pool.end())
	{
		pool[peer->id] = peer;
	}
}
ClientPeer* ClientPeerMgr::Get(uint64_t id)
{
	auto it = pool.find(id);
	if (it == pool.end())
	{
		return nullptr;
	}
	return it->second;
}
void ClientPeerMgr::Remove(ClientPeer* peer)
{
	auto it = pool.find(peer->id);
	if (it == pool.end())
		return;
	pool.erase(it);
}