#pragma once
#include "xx_uv.h"


class Client :public  xx::UvTcpPeer
{
public:
	uint32_t id = 0;
	Client::Client(xx::UvTcpListener& listener) :xx::UvTcpPeer(listener)
	{
		routingAddress = "client";
	}
	void DisconnectImpl() override
	{
		std::cout << "player disconnect.peer id:" << id << std::endl;
		Release();
	}
};

class GameService :public  xx::UvTcpClient
{
public:
	std::string strGuid;
	GameService::GameService(xx::UvLoop& loop) :xx::UvTcpClient(loop)
	{
		routingAddress = "gsservice";
	}
	void DisconnectImpl() override
	{
		std::cout << "game service disconnect.id:" << strGuid << std::endl;
		Release();
	}
};

// 路由器
class Router : public xx::TSingleton<Router>
{
	friend class xx::TSingleton<Router>;
public:
	xx::UvLoop* loop;
	xx::MemPool* mempool;
	xx::UvTcpListener* clientListener = nullptr;						// 接受客户端的连接
	std::unordered_map<std::string, GameService*> serviceClients;		// 连所有内部服务器
	std::unordered_map<uint64_t, Client*> clientpool;
	Router();
	~Router();
	void Init(xx::UvLoop* _loop);
	void RemoveClient(Client* peer);
	Client* GetClient(uint32_t peerId);
	void AddClient(Client* peer);

	void AddGameService(const std::string& strGuid, GameService* peer);
	GameService* GetGameService(const std::string& strGuid);
	void RemoveGameService(GameService* peer);
	//

	//
private:
	uint32_t peerIndex = 0;
	xx::UvTimer* timer;					// 帧逻辑驱动模拟
	void Update();
	void ConectToGameServer(const std::string& strIp, uint16_t nPort);
};
