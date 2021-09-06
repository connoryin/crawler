#include <gtest/gtest.h>

#include "crawler/robots_catalog.h"

using namespace testing;

class RobotsCatalogTest : public Test
   {
   protected:
      RobotsCatalog robotsCatalog;
   };

TEST_F( RobotsCatalogTest, IsAllowed )
   {
   EXPECT_TRUE( robotsCatalog.isAllowed( "https://www.google.com" ) );
   EXPECT_TRUE( robotsCatalog.isAllowed( "https://www.amazon.com/wishlist/universal" ) );
   EXPECT_FALSE( robotsCatalog.isAllowed( "https://www.amazon.com/wishlist/private" ) );
   EXPECT_FALSE( robotsCatalog.isAllowed( "https://en.wikipedia.org/wiki/Special:Test" ) );
   }
