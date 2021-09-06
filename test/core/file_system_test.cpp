#include <gtest/gtest.h>

#include "core/file_system.h"

TEST( FileSystemTest, FileSizeToString )
   {
   EXPECT_EQ( fileSizeToString( 512 ), "512 B" );
   EXPECT_EQ( fileSizeToString( 512 * 1024 ), "512 KB" );
   EXPECT_EQ( fileSizeToString( 512 * 1024 * 1024 ), "512 MB" );
   EXPECT_EQ( fileSizeToString( 50000 ), "48.8 KB" );
   }
