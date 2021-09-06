#include <gtest/gtest.h>

#include "html_parser/html_parser.h"

using namespace testing;

class HtmlParserTest : public Test
   {
   protected:
      HttpClient httpClient;
      HtmlParser htmlParser;
   };

TEST_F( HtmlParserTest, Parse )
   {
   auto htmlInfo = htmlParser.parse( httpClient.getString( "https://www.nytimes.com/" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://en.wikipedia.org/wiki/Main_Page" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://www.youtube.com/c/automattic" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://en.planet.wikimedia.org/" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://jenkins.io/" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://developer.github.com/" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://www.fsf.org/" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://t.co/DcKYmuApCO" ) );
   htmlInfo = htmlParser.parse( httpClient.getString( "https://www.youtube.com/about/policies/" ) );
   }
