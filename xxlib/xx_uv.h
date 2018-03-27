﻿#pragma once
#include "xx.h"
#include <mutex>

namespace xx
{
	class UvLoop;
	class UvListenerBase;
	class UvTcpListener;
	class UvTcpUdpBase;
	class UvTcpBase;
	class UvTcpPeer;
	class UvTcpClient;
	class UvTimer;
	class UvTimeouterBase;
	class UvTimeouter;
	class UvAsync;
	class UvRpcManager;
	class UvTimeouter;
	class UvContextBase;
	class UvUdpListener;
	class UvUdpBase;
	class UvUdpPeer;
	class UvUdpClient;

	enum class UvTcpStates
	{
		Disconnected,
		Connecting,
		Connected,
		Disconnecting,
	};

	enum class UvRunMode
	{
		Default,
		Once,
		NoWait
	};

	class UvLoop : public Object
	{
	public:
		void* ptr = nullptr;
		List<UvTcpListener*> tcpListeners;
		List<UvTcpClient*> tcpClients;
		List<UvUdpListener*> udpListeners;
		List<UvUdpClient*> udpClients;
		List<UvTimer*> timers;
		List<UvAsync*> asyncs;
		UvTimeouter* timeouter = nullptr;
		UvRpcManager* rpcMgr = nullptr;
		UvTimer* udpTimer = nullptr;
		uint32_t udpTicks = 0;
		std::array<char,65536> udpRecvBuf;
		uint32_t kcpInterval = 0;

		explicit UvLoop(MemPool* mp);
		~UvLoop();

		void InitTimeouter(uint64_t intervalMS = 1000, int wheelLen = 6, int defaultInterval = 5);
		void InitRpcManager(uint64_t rpcIntervalMS = 1000, int rpcDefaultInterval = 5);
		void InitKcpFlushInterval(uint32_t interval = 10);

		void Run(UvRunMode mode = UvRunMode::Default);
		void Stop();
		bool alive() const;

		UvTcpListener* CreateTcpListener();
		UvTcpClient* CreateTcpClient();
		UvUdpListener* CreateUdpListener();
		UvUdpClient* CreateUdpClient();
		UvTimer* CreateTimer(uint64_t timeoutMS, uint64_t repeatIntervalMS, std::function<void()>&& OnFire = nullptr);
		UvAsync* CreateAsync();
	};

	class UvListenerBase : public Object
	{
	public:
		std::function<void()> OnDispose;

		UvLoop& loop;
		size_t index_at_container = -1;

		void* ptr = nullptr;
		void* addrPtr = nullptr;

		UvListenerBase(UvLoop& loop);
	};

	class UvTcpListener : public UvListenerBase
	{
	public:
		std::function<UvTcpPeer*()> OnCreatePeer;
		std::function<void(UvTcpPeer*)> OnAccept;
		List<UvTcpPeer*> peers;

		UvTcpListener(UvLoop& loop);
		~UvTcpListener();

		static void OnAcceptCB(void* server, int status);
		void Bind(char const* ipv4, int port);
		void Listen(int backlog = 128);
	};

	class UvTimeouterBase : public Object
	{
	public:
		UvTimeouterBase(MemPool* mp);
		~UvTimeouterBase();
		UvTimeouter* timerManager = nullptr;
		UvTimeouterBase* timerPrev = nullptr;
		UvTimeouterBase* timerNext = nullptr;
		int timerIndex = -1;
		std::function<void()> OnTimeout;

		void TimerClear();
		void TimeoutReset(int interval = 0);
		void TimerStop();
		void BindTimeouter(UvTimeouter* tm);
		void UnbindTimerManager();
		bool timering();
	};

	class UvTcpUdpBase : public UvTimeouterBase
	{
	public:
		std::function<void(BBuffer&)> OnReceivePackage;

		// uint32_t: 流水号
		std::function<void(uint32_t, BBuffer&)> OnReceiveRequest;

		// 4个 size_t 代表 包起始offset, 含包头的总包长, 地址起始偏移, 地址长度( 方便替换地址并 memcpy )
		// BBuffer 的 offset 停在数据区起始位置
		std::function<void(BBuffer&, size_t, size_t, size_t, size_t)> OnReceiveRoutingPackage;

		std::function<void()> OnDispose;



		UvLoop& loop;
		size_t index_at_container = -1;

		void* ptr = nullptr;
		void* addrPtr = nullptr;

		BBuffer bbRecv;
		BBuffer bbSend;

		UvTcpUdpBase(UvLoop& loop);

		virtual void DisconnectImpl() = 0;
		virtual bool Disconnected() = 0;
		virtual size_t GetSendQueueSize() = 0;
		virtual void SendBytes(char const* inBuf, int len = 0) = 0;

		virtual void ReceiveImpl(char const* bufPtr, int len);

		void SendBytes(BBuffer& bb);

		template<typename T>
		void Send(T const& pkg);

		template<typename T>
		uint32_t SendRequest(T const& pkg, std::function<void(uint32_t, BBuffer)> cb, int interval = 0);

		template<typename T>
		void SendResponse(uint32_t serial, T const& pkg);

		template<typename T>
		void SendRoutePackage(T const & pkg, uint32_t nParam, size_t nlen);
	};

	class UvTcpBase : public UvTcpUdpBase
	{
	public:
		UvTcpBase(UvLoop& loop);

		size_t GetSendQueueSize() override;
		void SendBytes(char const* inBuf, int len = 0) override;

		static void OnReadCBImpl(void* stream, ptrdiff_t nread, const void* buf_t);
	};

	class UvTcpPeer : public UvTcpBase
	{
	public:
		// bool alive 只能外部检测野指针

		UvTcpListener & listener;
		UvTcpPeer(UvTcpListener& listener);
		~UvTcpPeer();	// Release 取代 Dispose
		void DisconnectImpl() override;
		bool Disconnected() override;
		std::array<char, 64> ipBuf;
		char* ip();
	};

	class UvTcpClient : public UvTcpBase
	{
	public:
		std::function<void(int)> OnConnect;
		std::function<void()> OnDisconnect;

		UvTcpStates state = UvTcpStates::Disconnected;
		bool alive() const;	// state == UvTcpStates::Connected, 并不能取代外部野指针检测
		UvTcpClient(UvLoop& loop);
		~UvTcpClient();	// Release 取代 Dispose
		void SetAddress(char const* const& ipv4, int port);
		static void OnConnectCBImpl(void* req, int status);
		void Connect();
		void Disconnect();
		void DisconnectImpl() override;
		bool Disconnected() override;
	};

	class UvTimer : public Object
	{
	public:
		std::function<void()> OnFire;
		std::function<void()> OnDispose;

		UvLoop& loop;
		size_t index_at_container = -1;
		void* ptr = nullptr;
		UvTimer(UvLoop& loop, uint64_t timeoutMS, uint64_t repeatIntervalMS, std::function<void()>&& OnFire = nullptr);
		~UvTimer();
		static void OnTimerCBImpl(void* handle);
		void SetRepeat(uint64_t repeatIntervalMS);
		void Again();
		void Stop();
	};

	class UvTimeouter : public Object
	{
	public:
		UvTimer* timer = nullptr;
		List<UvTimeouterBase*> timerss;
		int cursor = 0;
		int defaultInterval;
		UvTimeouter(UvLoop& loop, uint64_t intervalMS, int wheelLen, int defaultInterval);
		~UvTimeouter();
		void Process();
		void Clear();
		void Add(UvTimeouterBase* t, int interval = 0);
		void Remove(UvTimeouterBase* t);
		void AddOrUpdate(UvTimeouterBase* t, int interval = 0);
	};

	class UvAsync : public Object
	{
	public:
		std::function<void()> OnFire;
		std::function<void()> OnDispose;

		UvLoop& loop;
		size_t index_at_container = -1;
		std::mutex mtx;
		Queue<std::function<void()>> actions;
		void* ptr = nullptr;
		UvAsync(UvLoop& loop);
		~UvAsync();
		static void OnAsyncCBImpl(void* handle);
		void Dispatch(std::function<void()>&& a);
		void OnFireImpl();
	};

	class UvRpcManager : public Object
	{
	public:
		UvTimer* timer = nullptr;
		uint32_t serial = 0;
		Dict<uint32_t, std::function<void(uint32_t, BBuffer const*)>> mapping;
		Queue<std::pair<int, uint32_t>> serials;
		int defaultInterval = 0;
		int ticks = 0;
		size_t Count();
		UvRpcManager(UvLoop& loop, uint64_t intervalMS, int defaultInterval);
		~UvRpcManager();
		void Process();
		uint32_t Register(std::function<void(uint32_t, BBuffer const*)>&& cb, int interval = 0);
		void Unregister(uint32_t serial);
		void Callback(uint32_t serial, BBuffer const* bb);
	};

	class UvContextBase : public Object
	{
	public:
		UvTcpUdpBase* peer = nullptr;

		UvContextBase(MemPool* mp);
		~UvContextBase();
		bool PeerAlive();
		bool BindPeer(UvTcpUdpBase* p);
		void KickPeer(bool immediately = true);
		void OnPeerReceiveRequest(uint32_t serial, BBuffer& bb);
		void OnPeerReceivePackage(BBuffer& bb);
		void OnPeerDisconnect();
		virtual void HandleRequest(uint32_t serial, Ptr<Object>& o) = 0;
		virtual void HandlePackage(Ptr<Object>& o) = 0;
		virtual void HandleDisconnect() = 0;
	};

	class UvUdpListener : public UvListenerBase
	{
	public:
		std::function<UvUdpPeer*(Guid const&)> OnCreatePeer;
		std::function<void(UvUdpPeer*)> OnAccept;
		Dict<Guid, UvUdpPeer*> peers;

		UvUdpListener(UvLoop& loop);
		~UvUdpListener();
		static void OnRecvCBImpl(void* udp, ptrdiff_t nread, void* buf_t, void* addr, uint32_t flags);
		void OnReceiveImpl(char const* bufPtr, int len, void* addr);

		void Bind(char const* ipv4, int port);
		void Listen();
		void StopListen();

		UvUdpPeer* CreatePeer(Guid const& g
			, int sndwnd = 128, int rcvwnd = 128
			, int nodelay = 1/*, int interval = 10*/, int resend = 2, int nc = 1, int minrto = 100);
	};

	class UvUdpBase : public UvTcpUdpBase
	{
	public:
		Guid guid;
		uint32_t nextUpdateTicks = 0;
		UvUdpBase(UvLoop& loop);
	};

	class UvUdpPeer : public UvUdpBase
	{
	public:
		UvUdpListener& listener;

		UvUdpPeer(UvUdpListener& listener
			, Guid const& g = Guid()
			, int sndwnd = 128, int rcvwnd = 128
			, int nodelay = 1/*, int interval = 10*/, int resend = 2, int nc = 1, int minrto = 100);
		~UvUdpPeer();

		static int OutputImpl(char const* buf, int len, void* kcp);
		void Update(uint32_t current);
		void Input(char const* data, int len);
		void SendBytes(char const* data, int len = 0) override;
		void DisconnectImpl() override;
		size_t GetSendQueueSize() override;
		bool Disconnected() override;

		std::array<char, 64> ipBuf;
		char* ip();
	};

	class UvUdpClient : public UvUdpBase
	{
	public:
		void* kcpPtr = nullptr;
		UvUdpClient(UvLoop& loop);
		~UvUdpClient();

		void Connect(Guid const& guid
			, int sndwnd = 128, int rcvwnd = 128
			, int nodelay = 1/*, int interval = 10*/, int resend = 2, int nc = 1, int minrto = 100);

		static void OnRecvCBImpl(void* udp, ptrdiff_t nread, void* buf_t, void* addr, uint32_t flags);
		void OnReceiveImpl(char const* bufPtr, int len, void* addr);
		static int OutputImpl(char const* buf, int len, void* kcp);
		void Update(uint32_t current);
		void Disconnect();
		void SetAddress(char const* ipv4, int port);
		void SendBytes(char const* data, int len = 0) override;
		void DisconnectImpl() override;
		bool Disconnected() override;
		size_t GetSendQueueSize() override;
	};




	template<typename T>
	inline void UvTcpUdpBase::Send(T const & pkg)
	{
		if (!ptr) throw - 1;
		bbSend.Clear();
		bbSend.BeginWritePackage();
		bbSend.WriteRoot(pkg);
		bbSend.EndWritePackage();
		SendBytes(bbSend);
	}

	template<typename T>
	inline uint32_t UvTcpUdpBase::SendRequest(T const & pkg, std::function<void(uint32_t, BBuffer)> cb, int interval)
	{
		if (!ptr) throw - 1;
		auto serial = loop.rpcMgr.Register(cb, interval);
		bbSend.Clear();
		bbSend.BeginWritePackage(1, serial);
		bbSend.WriteRoot(pkg);
		bbSend.EndWritePackage();
		SendBytes(bbSend);
		return serial;
	}

	template<typename T>
	inline void UvTcpUdpBase::SendResponse(uint32_t serial, T const & pkg)
	{
		template<typename T>
		bbSend.Clear();
		bbSend.BeginWritePackage(2, serial);
		bbSend.WriteRoot(pkg);
		bbSend.EndWritePackage();
		SendBytes(bbSend);
	}
	template<typename T>
	inline void UvTcpUdpBase::SendRoutePackage(T const & pkg, uint32_t nParam, size_t nlen)
	{
		if (!ptr) throw - 1;
		bbSend.Clear();
		bbSend.BeginWritePackage();
		bbSend.WriteRoot(pkg);
		bbSend.EndWritePackage();
		//修改包头
		uint32_t ntypeid = 0;
		bbSend.Read(ntypeid);
		ntypeid = ntypeid | 8;
		ntypeid = ntypeid | nlen << 4;
		memcpy(bbSend.buf, &ntypeid, 1);
		bbSend.Reserve(bbSend.dataLen + nlen);
		memmove(bbSend.buf + 3 + nlen, bbSend.buf + 3, bbSend.dataLen - 3);
		memset(bbSend.buf + 3, 0, nlen);
		bbSend.WriteAt(bbSend.offset + 3, nParam);
		bbSend.dataLen += nlen;
		//
		SendBytes(bbSend);
	}

}
