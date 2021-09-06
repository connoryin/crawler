#ifndef SEARCH_ENGINE_DISTRIBUTED_H
#define SEARCH_ENGINE_DISTRIBUTED_H

#include "core/concurrency.h"
#include "core/net/socket.h"
#include "core/net/url.h"
#include "core/string.h"
#include "core/vector.h"
#include "crawler/crawler.h"
#include <thread>

class Distributed {
public:
  Distributed(
      Vector<String> &hosts, Crawler &crawler, int serverID);

  ~Distributed();

  void sendURL(const Url &url);

  //    Vector<Url> &get(); //for testing only

  bool isAlive();

private:
  void handleRequest(Socket socket);

  void accept(int num, bool forever);

  void reconnect(int hostNum);

  void send(int serverNo);

  Vector<String> _hosts;
  Vector<UniquePtr<Socket>> serverSockets;
  Crawler &crawler;
  std::atomic<bool> _isAlive;
  std::atomic<int> numSockets;
  Hash<Url> hash;
  int serverID;
  Vector<Mutex *> locks;
  Vector<HashSet<Url>> urlCache;
  UniquePtr<StreamWriter> logger;
  Vector<ConditionVariable*> cvs;
  //    Vector<Url> urls; // for testing only
};

#endif // SEARCH_ENGINE_DISTRIBUTED_H
