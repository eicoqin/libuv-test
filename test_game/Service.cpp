#include "Service.h"
#include "RPC_class.h"
#include "RouterParser.h"
#include <tchar.h>
#include <atlstr.h>

Service::Service(xx::UvLoop& _loop) :
	xx::Object(_loop.mempool), loop(_loop)
{
	
}


Service::~Service()
{
	//todo
}
bool Service::Start()
{	
	TCHAR buffer[1024] = { 0 };
	GetModuleFileName(GetModuleHandle(NULL), buffer, sizeof(buffer));
	CString wFileName = CString(buffer);
	CString ROOTPATH = wFileName.Left(wFileName.ReverseFind('\\'));
	ROOTPATH += "\\game.ini";
	int nPort = GetPrivateProfileInt(_T("CONFIG"), _T("PORT"), 12345, ROOTPATH);
	std::cout << "listen port:" << nPort << std::endl;
	//
	routerListener = loop.CreateTcpListener();
	routerListener->Bind("0.0.0.0", nPort);
	routerListener->Listen();
	routerListener->OnCreatePeer = [&]()->xx::UvTcpPeer* {
		xx::UvTcpPeer* peer = nullptr;
		mempool->CreateTo(peer, *routerListener);
		return peer;
	};
	routerListener->OnAccept = [this](xx::UvTcpPeer *peer)
	{
		//自注册消息到Route
		RPC::Generic::ServiceInfo_p serviceInfo;
		mempool->MPCreateTo(serviceInfo);
		serviceInfo->type = RPC::Generic::ServiceTypes::Game;
		mempool->MPCreateTo(serviceInfo->serviceId);
		serviceInfo->serviceId->Assign(guid);
		peer->Send(serviceInfo);
		//
		peer->OnReceivePackage = [peer](xx::BBuffer& bb, uint8_t typeId) {

		};

		peer->OnReceiveRouting = [peer](xx::BBuffer& bb, size_t pkgOffset, size_t pkgLen, size_t addrOffset, size_t addrLen)
		{
			protocol::RouterParser::Instance()->Route(peer, bb, pkgOffset, pkgLen, addrOffset, addrLen);
		};

	};
	timer = loop.CreateTimer(1000, 1000, [this] { Update(); });
	return true;
}
void Service::Update()
{

}