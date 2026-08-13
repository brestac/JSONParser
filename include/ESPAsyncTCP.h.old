// ESPAsyncTCP.h
// POSIX socket implementation for Linux/desktop testing
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "Stream.h"

class AsyncClient;

struct pbuf {};
enum : int {
  ERR_OK = 0,
  ERR_MEM = -1,
  ERR_BUF = -2,
  ERR_TIMEOUT = -3,
  ERR_RTE = -4,
  ERR_INPROGRESS = -5,
  ERR_VAL = -6,
  ERR_WOULDBLOCK = -7,
  ERR_USE = -8,
  ERR_ALREADY = -9,
  ERR_ISCONN = -10,
  ERR_CONN = -11,
  ERR_IF = -12,
  ERR_ABRT = -13,
  ERR_RST = -14,
  ERR_CLSD = -15,
  ERR_ARG = -16
};

typedef std::function<void(void*, AsyncClient*)>                          AcConnectHandler;
typedef std::function<void(void*, AsyncClient*, size_t, uint32_t)>        AcAckHandler;
typedef std::function<void(void*, AsyncClient*, int)>                     AcErrorHandler;
typedef std::function<void(void*, AsyncClient*, void*, size_t)>           AcDataHandler;
typedef std::function<void(void*, AsyncClient*, struct pbuf*)>            AcPacketHandler;
typedef std::function<void(void*, AsyncClient*, uint32_t)>               AcTimeoutHandler;

class AsyncClient : Stream {
public:
    AsyncClient();
    ~AsyncClient();

    void onConnect(AcConnectHandler cb, void* arg = nullptr);
    void onDisconnect(AcConnectHandler cb, void* arg = nullptr);
    void onAck(AcAckHandler cb, void* arg = nullptr)         {}
    void onError(AcErrorHandler cb, void* arg = nullptr);
    void onData(AcDataHandler cb, void* arg = nullptr);
    void onPacket(AcPacketHandler cb, void* arg = nullptr)   {}
    void onTimeout(AcTimeoutHandler cb, void* arg = nullptr) {}
    void onPoll(AcConnectHandler cb, void* arg = nullptr)    {}

    int write(uint8_t data) override;
    size_t write(const uint8_t* data, size_t len) override;
    int    availableForWrite() override;
    void flush() override;
    bool outputCanTimeout(bool) override;

    int    read() override;
    int    peek() override;
    int    available() override;

    bool        connect(const char* host, uint16_t port, bool secure = false);
    void        write(const char* data, size_t len);
    void        close();
    void        ack(size_t) {}
    bool        connected() const;
    void        setRxTimeout(uint32_t ms);
    void        setAckTimeout(uint32_t ms)              {}
    void        setKeepAlive(uint32_t ms, uint8_t cnt)  {}
    bool        canSend()                { return true; }
    size_t      space()                  { return 2048; }
    const char* errorToString(int error) const;

private:
    int                _sock         = -1;
    std::atomic<bool>  _connected    {false};
    std::atomic<bool>  _inProgress   {false};
    std::atomic<bool>  _stop         {false};
    uint32_t           _rxTimeoutMs  {20000};
    std::thread        _thread;
    std::mutex         _sockMutex;

    AcConnectHandler   _connectCb;
    void*              _connectArg    = nullptr;
    AcConnectHandler   _disconnectCb;
    void*              _disconnectArg = nullptr;
    AcErrorHandler     _errorCb;
    void*              _errorArg      = nullptr;
    AcDataHandler      _dataCb;
    void*              _dataArg       = nullptr;

    void _run(std::string host, uint16_t port);
    void _closeSocket();
};
