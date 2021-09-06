#include <gtest/gtest.h>

#include "core/net/url.h"

TEST( UrlTest, ParseInfo )
   {
   Url url( "https://www.google.com/index.html?query=test" );
   EXPECT_TRUE( url.isAbsoluteUrl( ) );
   EXPECT_EQ( url.scheme( ), "https" );
   EXPECT_EQ( url.host( ), "www.google.com" );
   EXPECT_EQ( url.port( ), 443 );
   EXPECT_EQ( url.localPath( ), "/index.html" );
   EXPECT_EQ( url.query( ), "query=test" );
   EXPECT_EQ( url.pathAndQuery( ), "/index.html?query=test" );

   url = Url( "https://www.google.com:443" );
   EXPECT_TRUE( url.isAbsoluteUrl( ) );
   EXPECT_EQ( url.scheme( ), "https" );
   EXPECT_EQ( url.host( ), "www.google.com" );
   EXPECT_EQ( url.port( ), 443 );
   EXPECT_EQ( url.localPath( ), "/" );
   EXPECT_EQ( url.query( ), "" );
   EXPECT_EQ( url.pathAndQuery( ), "/" );

   url = Url( "http://www.usa.philips.com/content/corporate/en_US/terms-of-use.html" );
   EXPECT_TRUE( url.isAbsoluteUrl( ) );
   EXPECT_EQ( url.scheme( ), "http" );
   EXPECT_EQ( url.host( ), "www.usa.philips.com" );
   EXPECT_EQ( url.port( ), 80 );
   EXPECT_EQ( url.localPath( ), "/content/corporate/en_US/terms-of-use.html" );
   EXPECT_EQ( url.query( ), "" );
   EXPECT_EQ( url.pathAndQuery( ), "/content/corporate/en_US/terms-of-use.html" );

   url = Url( "//www.cnn.com/business" );
   EXPECT_TRUE( url.isAbsoluteUrl( ) );
   EXPECT_EQ( url.scheme( ), "http" );
   EXPECT_EQ( url.host( ), "www.cnn.com" );
   EXPECT_EQ( url.port( ), 80 );
   EXPECT_EQ( url.localPath( ), "/business" );
   EXPECT_EQ( url.query( ), "" );
   EXPECT_EQ( url.pathAndQuery( ), "/business" );

   url = Url( "/index.html" );
   EXPECT_FALSE( url.isAbsoluteUrl( ) );
   EXPECT_EQ( STRING( url ), "/index.html" );

   url = Url( "index.html" );
   EXPECT_FALSE( url.isAbsoluteUrl( ) );
   EXPECT_EQ( STRING( url ), "index.html" );
   }

TEST( UrlTest, CombineUrls )
   {
   EXPECT_EQ( STRING( Url( Url( "https://www.google.com" ), "index.html" ) ),
              "https://www.google.com/index.html" );
   EXPECT_EQ( STRING( Url( Url( "https://www.google.com" ), "/index.html" ) ),
              "https://www.google.com/index.html" );
   EXPECT_EQ( STRING( Url( Url( "https://www.google.com/?query=test" ), "index.html" ) ),
              "https://www.google.com/index.html" );
   EXPECT_EQ( STRING( Url( Url( "https://www.google.com/" ), "/index.html" ) ),
              "https://www.google.com/index.html" );
   EXPECT_EQ( STRING( Url( Url( "https://www.google.com/US/" ), "/index.html?query=test" ) ),
              "https://www.google.com/index.html?query=test" );
   EXPECT_EQ( STRING( Url( Url( "https://www.google.com/about/" ), "index.html" ) ),
              "https://www.google.com/about/index.html" );
   }
