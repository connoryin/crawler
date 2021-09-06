#include <getopt.h>
#include <sys/resource.h>

#include "core/file_system.h"
#include "core/io.h"
#include "crawler/crawler.h"
#include "distributed/distributed.h"

enum class OptionName
   {
      AssumeYes,
      SeedFile,
      NumThreads,
      LogPath,
      DataDir,
      CheckpointPath,
      StatsRefreshInterval,
      ExpectedNumUrls,
      CheckpointInterval,
      ServerID,
      HostNamePath
   };

bool isUserConfirmed( bool assumeYes );

int main( int argc, char **argv )
   {
   static const option options[] = {
         { "assume_yes",             no_argument,       nullptr, static_cast<int>(OptionName::AssumeYes) },
         { "seed_file",              required_argument, nullptr, static_cast<int>(OptionName::SeedFile) },
         { "num_threads",            required_argument, nullptr, static_cast<int>(OptionName::NumThreads) },
         { "log_path",               required_argument, nullptr, static_cast<int>(OptionName::LogPath) },
         { "data_dir",               required_argument, nullptr, static_cast<int>(OptionName::DataDir) },
         { "checkpoint_path",        required_argument, nullptr, static_cast<int>(OptionName::CheckpointPath) },
         { "stats_refresh_interval", required_argument, nullptr, static_cast<int>(OptionName::StatsRefreshInterval) },
         { "expected_num_urls",      required_argument, nullptr, static_cast<int>(OptionName::ExpectedNumUrls) },
         { "checkpoint_interval",    required_argument, nullptr, static_cast<int>(OptionName::CheckpointInterval) },
         { "serverID",               required_argument, nullptr, static_cast<int>(OptionName::ServerID) },
         { "hostname_path",          required_argument, nullptr, static_cast<int>(OptionName::HostNamePath) },
         { nullptr,                  no_argument,       nullptr, 0 }
   };

   bool assumeYes = false;
   String seedFile, hostnameFile;
   CrawlerConfiguration config;
   int numThreads = 1;
   int serverID;

   int optionVal;
   while ( ( optionVal = getopt_long_only( argc, argv, "", options, nullptr ) ) != -1 )
      {
      switch ( static_cast<OptionName>(optionVal) )
         {
         case OptionName::AssumeYes:
            assumeYes = true;
            break;
         case OptionName::SeedFile:
            seedFile = optarg;
            break;
         case OptionName::NumThreads:
            numThreads = std::stoi( optarg );
            break;
         case OptionName::LogPath:
            config.logPath = optarg;
            break;
         case OptionName::DataDir:
            config.dataDir = optarg;
            break;
         case OptionName::CheckpointPath:
            config.checkpointPath = optarg;
            break;
         case OptionName::StatsRefreshInterval:
            config.statsRefreshInterval = std::stoi( optarg );
            break;
         case OptionName::ExpectedNumUrls:
            config.expectedNumUrls = std::stoi( optarg );
            break;
         case OptionName::CheckpointInterval:
            config.checkpointInterval = std::stoi( optarg );
            break;
         case OptionName::ServerID:
            serverID = std::stoi( optarg );
            break;
         case OptionName::HostNamePath:
            hostnameFile = optarg;
            break;
         default:
            throw ArgumentException( "The option is unrecognized." );
         }
      }

   // Checks resource limits.
   static constexpr auto recommendedFileDescriptorLimit = 65536;
   rlimit limit{ };
   if ( getrlimit( RLIMIT_NOFILE, &limit ) == -1 ) throw SystemException( );
   if ( limit.rlim_cur < recommendedFileDescriptorLimit )
      {
      std::cout << "The resource limit for file descriptor number is too low (" << limit.rlim_cur << ").\n"
                << "Do you want to increase the limit to " << recommendedFileDescriptorLimit << "? [Y/n]" << std::endl;
      if ( isUserConfirmed( assumeYes ) )
         {
         limit.rlim_cur = recommendedFileDescriptorLimit;
         if ( setrlimit( RLIMIT_NOFILE, &limit ) == -1 ) throw SystemException( );
         std::cout << "The file descriptor limit is successfully updated to "
                   << recommendedFileDescriptorLimit << '.' << std::endl;
         }
      }

   // Checks if the data directory exists.
   if ( !std::filesystem::exists( config.dataDir ) )
      {
      std::cout << "The data directory " << config.dataDir << " does not exists.\n"
                << "Do you want to create the directory? [Y/n]" << std::endl;
      if ( isUserConfirmed( assumeYes ) )
         {
         std::filesystem::create_directories( config.dataDir );
         std::cout << "The data directory is created at " << config.dataDir << "." << std::endl;
         }
      }

   UniquePtr<Crawler> crawler;

   // Checks if the checkpoint file has already existed.
   if ( std::filesystem::exists( config.checkpointPath ) )
      {
      std::cout << "A checkpoint file is found at " << config.checkpointPath << ".\n"
                << "Do you want to load the checkpoint file? [Y/n]" << std::endl;
      if ( isUserConfirmed( assumeYes ) )
         crawler = makeUnique<Crawler>( config.checkpointPath.string( ), config );
      }

   if ( crawler == nullptr )
      {
      Vector<Url> seedList;
      std::ifstream seedFileStream( seedFile );
      if ( !seedFileStream.is_open( ) ) throw IOException( "The seed file cannot be opened." );
      for ( String line; std::getline( seedFileStream, line ); )
         seedList.emplace_back( line );
      crawler = makeUnique<Crawler>( seedList, config );
      }

   Vector<String> hosts;
   std::ifstream hostnames( hostnameFile );
   for ( String line; std::getline( hostnames, line ); )
      hosts.emplace_back( line );
   hostnames.close( );
   auto const distributed = makeUnique<Distributed>( hosts, *crawler, serverID );

   crawler->beginCrawl( numThreads );
   std::cout << "The crawler has begun crawling. Press any key to stop the crawler..." << std::endl;
   std::cin.ignore( std::numeric_limits<std::streamsize>::max( ) ), std::cin.get( );
   }

bool isUserConfirmed( bool assumeYes )
   {
   char response;
   if ( !assumeYes ) std::cin >> response;
   else response = 'y';
   return toLower( response ) == 'y';
   }
