#include <gtest/gtest.h>

#include "core/net/socket.h"

TEST( DnsTest, GetHostAddresses )
   {
   auto addresses [[gnu::unused]] = Dns::getHostAddresses( "www.google.com" );
   }
