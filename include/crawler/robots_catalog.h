#pragma once

#include "core/concurrency.h"
#include "core/hash_table.h"
#include "core/net.h"
#include "core/string.h"
#include "core/vector.h"

/// Maintains a catalog of robots.txt rules.
class RobotsCatalog
   {
   public:
      /// Initializes a `RobotsCatalog`.
      RobotsCatalog( );

      RobotsCatalog( const RobotsCatalog & ) = delete;
      RobotsCatalog &operator=( const RobotsCatalog & ) = delete;
      RobotsCatalog( RobotsCatalog && ) = delete;
      RobotsCatalog &operator=( RobotsCatalog && ) = delete;

      ~RobotsCatalog( )
         {
         _isRunning = false;
         _cacheThread.join( );
         }

      /// Indicates if the request URL is allowed to be crawled.
      /// \param requestUrl The request URL.
      /// \return `true` if the request URL is allowed to be crawled.
      bool isAllowed( const Url &requestUrl );

      /// Indicates if the request URL is allowed to be crawled.
      /// \param requestUrl The request URL string.
      /// \return `true` if the request URL is allowed to be crawled.
      bool isAllowed( StringView requestUrl )
         { return isAllowed( Url( requestUrl ) ); }

   private:
      enum class RuleType
         {
            Allow,
            Disallow
         };

      struct Rule
         {
         public:
            RuleType type;
            String pattern;
         };

      struct CacheEntry
         {
         public:
            Vector<Rule> rules;
            int numHits = 0;

            explicit CacheEntry( Vector<Rule> rules ) noexcept: rules( std::move( rules ) )
               { }
         };

      static Vector<Rule> parseRobotsFile( StringView robotsString );

      static bool startsWithPattern( StringView string, StringView pattern );

      void refreshCache( );

      static constexpr auto _cacheHitRateThreshold = 1;
      static constexpr auto _cacheRefreshInterval = 5;

      std::atomic<bool> _isRunning = true;
      Thread _cacheThread;

      HttpClient _httpClient;

      HashMap<String, CacheEntry> _rulesCache;
      mutable Mutex _rulesCacheMutex;
   };
