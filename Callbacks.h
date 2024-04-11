#ifndef __CALLBACKS_H_
#define __CALLBACKS_H_

#include <memory>
#include <functional>

class Buffer;
class TimeStamp;
class TcpConnection;


typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const TcpConnectionPtr&, Buffer*, TimeStamp)> MessageCallback;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

#endif // !__CALLBACKS_H_
