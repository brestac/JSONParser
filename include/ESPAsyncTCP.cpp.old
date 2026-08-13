#include "ESPAsyncTCP.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

AsyncClient::AsyncClient() {}

AsyncClient::~AsyncClient() {
    close();
}

void AsyncClient::onConnect(AcConnectHandler cb, void* arg) {
    _connectCb = cb;
    _connectArg = arg;
}

void AsyncClient::onDisconnect(AcConnectHandler cb, void* arg) {
    _disconnectCb = cb;
    _disconnectArg = arg;
}

void AsyncClient::onError(AcErrorHandler cb, void* arg) {
    _errorCb = cb;
    _errorArg = arg;
}

void AsyncClient::onData(AcDataHandler cb, void* arg) {
    _dataCb = cb;
    _dataArg = arg;
}

bool AsyncClient::connected() const {
    return _connected.load(std::memory_order_acquire);
}

void AsyncClient::setRxTimeout(uint32_t ms) {
    _rxTimeoutMs = ms;
}

bool AsyncClient::connect(const char* host, uint16_t port, bool) {
    if (_inProgress.load() || _connected.load())
        return false;

    _stop.store(false);
    _inProgress.store(true);

    if (_thread.joinable())
        _thread.join();

    _thread = std::thread(&AsyncClient::_run, this, std::string(host), port);
    DEBUG_PRINTF("[AsyncClient] ::connect: thread started\n");
    return true;
}

void AsyncClient::write(const char* data, size_t len) {
    std::lock_guard<std::mutex> lk(_sockMutex);
    if (_sock < 0 || !_connected.load())
        return;
    ::send(_sock, data, len, MSG_NOSIGNAL);
}

void AsyncClient::close() {
    _stop.store(true);
    _closeSocket();
    if (_thread.joinable())
        _thread.join();
    _inProgress.store(false);
}

void AsyncClient::_closeSocket() {
    std::lock_guard<std::mutex> lk(_sockMutex);
    if (_sock >= 0) {
        ::shutdown(_sock, SHUT_RDWR);
        ::close(_sock);
        _sock = -1;
    }
    _connected.store(false, std::memory_order_release);
}

void AsyncClient::_run(std::string host, uint16_t port) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;   // IPv4 only — matches ESP8266/lwIP
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(port);

    int gai = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (gai != 0 || !res) {
        DEBUG_PRINTF("[AsyncClient] ::_run: getaddrinfo failed: %s\n", gai_strerror(gai));
        if (!_stop.load() && _errorCb)
            _errorCb(_errorArg, this, ERR_CONN);
        _inProgress.store(false);
        return;
    }

    int sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        DEBUG_PRINTF("[AsyncClient] ::_run: socket() failed: %s\n", strerror(errno));
        ::freeaddrinfo(res);
        if (!_stop.load() && _errorCb)
            _errorCb(_errorArg, this, ERR_CONN);
        _inProgress.store(false);
        return;
    }

    struct timeval tv;
    tv.tv_sec  = _rxTimeoutMs / 1000;
    tv.tv_usec = (_rxTimeoutMs % 1000) * 1000;
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    DEBUG_PRINTF("[AsyncClient] ::_run: connecting to %s:%u\n", host.c_str(), port);
    if (::connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        DEBUG_PRINTF("[AsyncClient] ::_run: connect() failed: %s\n", strerror(errno));
        ::freeaddrinfo(res);
        ::close(sock);
        if (!_stop.load() && _errorCb)
            _errorCb(_errorArg, this, ERR_CONN);
        _inProgress.store(false);
        return;
    }
    DEBUG_PRINTF("[AsyncClient] ::_run: TCP connected\n");
    ::freeaddrinfo(res);

    {
        std::lock_guard<std::mutex> lk(_sockMutex);
        _sock = sock;
        _connected.store(true, std::memory_order_release);
    }

    if (!_stop.load() && _connectCb) {
        DEBUG_PRINTF("[AsyncClient] ::_run: calling _connectCb\n");
        _connectCb(_connectArg, this);
    }

    std::vector<char> buf(4096);
    while (!_stop.load()) {
        ssize_t n = ::recv(sock, buf.data(), buf.size(), 0);
        if (n > 0) {
            if (_dataCb)
                _dataCb(_dataArg, this, buf.data(), static_cast<size_t>(n));
        } else if (n == 0) {
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            break;
        }
    }

    _closeSocket();
    _inProgress.store(false);

    if (!_stop.load() && _disconnectCb)
        _disconnectCb(_disconnectArg, this);
}

const char* AsyncClient::errorToString(int err) const {
    switch (err) {
        case ERR_OK:      return "OK";
        case ERR_MEM:     return "Out of memory";
        case ERR_BUF:     return "Buffer error";
        case ERR_TIMEOUT: return "Timeout";
        case ERR_RTE:     return "Route error";
        case ERR_ALREADY: return "Already connecting";
        case ERR_ISCONN:  return "Already connected";
        case ERR_CONN:    return "Connection failed";
        case ERR_ABRT:    return "Connection aborted";
        case ERR_RST:     return "Connection reset";
        case ERR_CLSD:    return "Connection closed";
        case ERR_ARG:     return "Illegal argument";
        default:          return "Unknown error";
    }
}

// Stream methods
size_t AsyncClient::write(uint8_t data) {
    write(reinterpret_cast<const char*>(&data), 1);
    return 1;
}

size_t AsyncClient::write(const uint8_t* data, size_t len) {
    write(reinterpret_cast<const char*>(data), len);
    return len;
}

int AsyncClient::available() {
    // Not implemented for simplicity
    return UINT32_MAX;
}

int AsyncClient::availableForWrite() {
    // Not implemented for simplicity
    return UINT32_MAX;
}

int AsyncClient::read() {
    // Needs to be implemented for proper Stream compatibility
    return -1;
}

int AsyncClient::peek() {
    // Needs to be implemented for proper Stream compatibility
    return -1;
}


