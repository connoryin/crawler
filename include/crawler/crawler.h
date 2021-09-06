#pragma once

#include <optional>

#include <thread>
#include "core/concurrency.h"
#include "core/exception.h"
#include "core/file_system.h"
#include "core/hash_table.h"
#include "core/io.h"
#include "core/net.h"
#include "core/string.h"
#include "core/vector.h"
#include "crawler/robots_catalog.h"
#include "html_parser/html_parser.h"

class Distributed;

/// Represents configuration for the crawler.
struct CrawlerConfiguration
   {
   public:
      std::optional<std::filesystem::path> logPath; ///< The log path to write to; std::clog if `nullopt`.
      std::filesystem::path dataDir; ///< The directory to store parsed html data.
      std::filesystem::path checkpointPath; ///< The checkpoint path to write to.
      int statsRefreshInterval = 5; ///< The interval in seconds at which the statistics refreshes.
      int expectedNumUrls = 1'000'000; ///< The expected total number of URLs to crawl.
      int checkpointInterval = 600; ///< The interval in seconds at which the crawler creates a createCheckpoint.
   };

/// Retrieves HTML files from the Internet recursively by traversing links within HTML files.
class Crawler
   {
   public:
      /// Initializes a `Crawler` with the specified seed list and configuration.
      /// \param seedList The seed list to initialize the frontier.
      /// \param config The crawler configuration.
      Crawler( const Vector<Url> &seedList, const CrawlerConfiguration &config );

      /// Initializes a `Crawler` from the specified checkpoint file and configuration.
      /// \param checkpointFilePath The checkpoint file path.
      /// \param config The crawler configuration.
      Crawler( StringView checkpointFilePath, const CrawlerConfiguration &config );

      Crawler( const Crawler & ) = delete;
      Crawler &operator=( const Crawler & ) = delete;
      Crawler( Crawler && ) = delete;
      Crawler &operator=( Crawler && ) = delete;

      ~Crawler( )
         { if ( _isRunning ) endCrawl( ); }

      /// Begins crawling HTML files using the specified number of worker threads.
      /// \param numThreads The number of threads to use.
      void beginCrawl( int numThreads );

      /// Ends crawling HTML files and waits for worker threads to finish.
      void endCrawl( );

      void insertFrontier( const Url &url );

      void setDistributed( Distributed *distributed );

   private:
      explicit Crawler( CrawlerConfiguration config );

      void doWork( int threadId, int numThreads );

      [[nodiscard]] static int getUrlScore( const Url &url );

      [[nodiscard]] Vector<Url> getNextUrlBatch( int batchSize, int sampleFactor = 2 );

      [[nodiscard]] HttpResponseMessage getHttpResponse( Url &requestUrl );

      [[nodiscard]] static bool filterLink( const Url &url, const TagInfo &tagInfo );

      void createCheckpoint( ) const;

      static constexpr auto _frontierSizeLimit = 1'000'000;
      static constexpr auto _filterFalsePositiveRate = 1e-3;
      static constexpr auto _hostHitRateLimit = 2'048;
      static constexpr auto _garbageCollectionInterval = 30;

      CrawlerConfiguration _config;
      UniquePtr<StreamWriter> _logger;

      std::atomic<int> _numCrawledDuringLastInterval = 0;
      std::atomic<int> _numCrawledTotal = 0;

      ConditionVariable _cv;
      std::atomic<bool> _isRunning = false;
      Vector<Thread> _threadPool;
      Thread _gcThread, _statsThread, _checkpointThread;

      HttpClient _httpClient;
      HtmlParser _htmlParser;

      HashSet<Url> _frontier;
      mutable Mutex _frontierMutex;

      BloomFilter<Url> _scheduledUrls;
      mutable Mutex _scheduledUrlsMutex;

      HashMap<String, int> _hitsCache;
      mutable Mutex _hitsCacheMutex;

      RobotsCatalog _robotsCatalog;

      Distributed *_distributed;
   };
