#include "distributed/distributed.h"

Distributed::Distributed(Vector<String> &hosts, Crawler &crawler, int serverID)
    : crawler(crawler), _isAlive(true), serverID(serverID), _hosts(hosts), numSockets(0),
      logger(StreamWriter::synchronized(
          StreamWriter("distributed.log"))) {
  urlCache.resize(hosts.size());
  for (int i = 0; i < hosts.size(); ++i) {
    cvs.push_back(new ConditionVariable);
  }
  int num = hosts.size();
  bool forever = false;
  std::thread t([&](int num, bool forever){
    accept(num, forever);
  }, num, forever);
  for (auto &host : hosts) {
    auto socket = makeUnique<Socket>(AddressFamily::InterNetwork, SocketType::Stream,
                             ProtocolType::Tcp);
    bool success = false;
    while (!success) {
      success = true;
      try {
        socket->connect(host, 8888);
      } catch (const SocketException &e) {
        success = false;
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
    serverSockets.push_back(std::move(socket));
  };

  t.join();

  std::cout << "Other servers joined, crawling started\n";

  for (int i = 0; i < hosts.size(); ++i) {
    locks.push_back(new Mutex());
  }
  crawler.setDistributed(this);

  forever = true;
  std::thread listenThread([&](int num, bool forever){
    accept(num, forever);
  }, num, forever);
  listenThread.detach();

  for (int i = 0; i < hosts.size(); ++i) {
    if (i == serverID) continue;
    std::thread sendThread(&Distributed::send, this, i);
    sendThread.detach();
  }
}

Distributed::~Distributed() {
  _isAlive = false;
//  char message[] = "kill";
//  for (auto &socket : serverSockets) {
//    try {
//      socket->send(reinterpret_cast<const std::byte *>(message),
//                   strlen(message) + 1, SocketFlags::NoSignal);
//    } catch (const SocketException &e) {
//      // other servers might have already been kill
//    }
//  }
  for (auto lock : locks) {
    delete lock;
  }
  for (auto cv : cvs) {
    delete cv;
  }
  serverSockets.clear();
  crawler.endCrawl();
}

void Distributed::sendURL(const Url &url) {
  if (!_isAlive)
    return;
  if (!url.isAbsoluteUrl()) return;
  auto i = hash(url) % serverSockets.size();
  if (i == serverID) {
    crawler.insertFrontier(url);
//    logger->writeLine(
//        STRING("me: " << serverID << ", url: " << url << " keep\n"));
    //        urls.emplace_back(url);
    return;
  }
  UniqueLock lock(*locks[i]);
  urlCache[i].emplace(url);
  cvs[i]->notifyOne();
}

void Distributed::handleRequest(Socket socket) {
  std::cerr << "handle: " << socket.handle() << "\n";
  while (_isAlive) {
    std::byte buf;
    int num;
    String request;
    int numTried = 0;
    do {
      try {
        num = socket.receive(&buf, 1);
      } catch (const SocketException &e) {
        //            _isAlive = false;
        numTried++;
        std::cerr << "connection fails " << numTried << " times\n";
        if (numTried > 10) {
          std::cerr << "lost one server\n";
          numSockets--;
          return;
        }
        continue;
      }
      if (char(buf) == '\0')
        break;
      else
        request += char(buf);
    } while (num);
    if (request == "kill") {
      _isAlive = false;
      return;
    } else if (!request.empty()) {
//      logger->writeLine(STRING("got request: " << request << "\n"));
      Url url;
      try {
        url = Url(request);
      } catch (...) {
        return;
      }
      if (!url.isAbsoluteUrl()) return;
      crawler.insertFrontier(url);
      //            urls.emplace_back(url);
    }
  }
}

void Distributed::accept(int num, bool forever) {
  // accept connection from all the other servers
  Socket socket{AddressFamily::InterNetwork, SocketType::Stream,
                ProtocolType::Tcp};
  socket.setSocketOption(SocketOptionLevel::Socket,SocketOptionName::ReuseAddress,true);
  try {
    socket.bind(IPEndPoint(IPAddress::any, 8888));
    socket.listen(10);
  } catch (const SocketException &e) {
    throw HttpRequestException("Cannot connect to other servers", e);
  }

  // wait for all the servers to connect before returning
  while (num || forever) {
    Socket s = socket.accept();
    std::cout << "accepted one server" << "\n";
//    std::cerr << "handle: " << s.handle() << "\n";
    std::thread t([&](Socket socket){
      //handle request
      //            urls.emplace_back(url);
      handleRequest(std::move(socket));

    }, std::move(s));
    num--;
    t.detach();
  }
}

bool Distributed::isAlive() { return _isAlive; }

void Distributed::reconnect(int hostNum) {
  auto socket = makeUnique<Socket>(AddressFamily::InterNetwork, SocketType::Stream,
                           ProtocolType::Tcp);
  bool success = false;
  while (!success) {
    success = true;
    try {
      socket->connect(_hosts[hostNum], 8888);
    } catch (const SocketException &e) {
      success = false;
      UniqueLock lock(*locks[hostNum]);
      auto &cache = urlCache[hostNum];
      if ( cache.size( ) > 1000000 ) {
        while (cache.size() > 1000000 / 2)
          cache.erase(cache.cbegin());
        std::cerr << "cache cleared\n";
      }
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }
//    UniqueLock lock(*locks[hostNum]);
    serverSockets[hostNum].swap(socket);
//    while (!urlCache[hostNum].empty()) {
//      auto urlString = STRING(urlCache[hostNum].back());
//      urlString.push_back('\0');
//      lock.unlock();
//      try {
//        socket->send(reinterpret_cast<const std::byte *>(urlString.data()),
//                     urlString.length(), SocketFlags::NoSignal);
//      } catch (const SocketException &e) {
//        success = false;
//        std::cerr << "reconnection failed\n";
//        break;
//      }
//      lock.lock();
//      urlCache[hostNum].pop_back();
//    }
  }
  std::cerr << "reconnected\n";
}

void Distributed::send(int serverNo) {
  UniqueLock lock(*locks[serverNo]);
  auto &cache = urlCache[serverNo];
  auto socket = serverSockets[serverNo].get();
  while (_isAlive) {
    while (cache.empty()) {
      cvs[serverNo]->wait( lock );
    }
    if ( cache.size( ) > 1000000 ) {
      while (cache.size() > 1000000 / 2)
        cache.erase(cache.cbegin());
      std::cerr << "cache cleared\n";
    }
    auto url = STRING(*cache.cbegin());
    url.push_back('\0');
    cache.erase(cache.cbegin());
    lock.unlock();

//    logger->writeLine(STRING("me: " << serverID << ", url: " << url
//                                    << ", sending to: " << serverNo << "\n"));
    try {
      socket->send(reinterpret_cast<const std::byte *>(url.data()),
                                    url.length(), SocketFlags::NoSignal);
    } catch (const SocketException &e) {
      std::cerr << STRING("sending to " << serverNo << " failed\n");
      reconnect(serverNo);
      socket = serverSockets[serverNo].get();
    }

    lock.lock();
  }
}
