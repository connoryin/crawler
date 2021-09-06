#include <gtest/gtest.h>

#include "core/net/http.h"

using namespace testing;

class HttpClientTest : public Test
   {
   protected:
      HttpClient httpClient;
   };

TEST_F( HttpClientTest, Get )
   {
   auto response = httpClient.get( "https://google.com/" );
   EXPECT_EQ( response.statusCode, 301 );
   response = httpClient.get( "https://en.wikipedia.org/wiki/Main_Page" );
   EXPECT_EQ( response.statusCode, 200 );
   response = httpClient.get( "https://www.nytimes.com/" );
   EXPECT_EQ( response.statusCode, 200 );
   response = httpClient.get( "https://openxcom.org/" );
   EXPECT_EQ( response.statusCode, 200 );
   response = httpClient.get( "https://play.google.com/store/apps/details?id=com.reddit.frontpage" );
   EXPECT_EQ( response.statusCode, 200 );
   response = httpClient.get( "https://www.youtube.com/about/policies/" );
   EXPECT_EQ( response.statusCode, 200 );
   response = httpClient.get( "//tours.cnn.com/" );
   EXPECT_EQ( response.statusCode, 301 );
   response = httpClient.get( "https://www.instagram.com/cnn/" );
   EXPECT_EQ( response.statusCode, 200 );
   EXPECT_THROW( response = httpClient.get( "https://wii.ign.com/" ), HttpRequestException );
   }
