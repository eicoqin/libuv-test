#pragma once
#include "xx_uv.h"


class Service : public xx::Object
{
public:
	xx::UvLoop& loop;
	xx::UvTcpListener* routerListener;	// 接受路由的连接
	Service(xx::UvLoop& loop);
	~Service();
	bool Start();
private:
	xx::UvTimer* timer;					// 帧逻辑驱动模拟
	xx::Guid guid;
	void Update();
};