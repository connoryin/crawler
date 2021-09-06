#include <algorithm>

#include "html_parser/html_parser.h"

TagInfo TagInfo::parse( StringView tagString )
   {
   if ( tagString.size( ) < 2 )
      throw FormatException( "The tag is malformed." );

   TagInfo tagInfo;
   auto begin = tagString.cbegin( ), end = tagString.cend( );
   if ( *( begin + 1 ) != '/' && *( end - 2 ) != '/' )
      {
      tagInfo.type = TagType::Opening;
      begin++, end--;
      }
   else if ( *( begin + 1 ) == '/' && *( end - 2 ) != '/' )
      {
      tagInfo.type = TagType::Closing;
      begin += 2, end--;
      }
   else if ( *( begin + 1 ) != '/' && *( end - 2 ) == '/' )
      {
      tagInfo.type = TagType::SelfClosing;
      begin++, end -= 2;
      }
   else throw FormatException( "The tag is malformed." );

   const auto nameEnd = std::find_if( begin, end, [ ]( char c )
      { return std::isspace( c ); } );
   tagInfo.name = String( begin, nameEnd );
   for ( auto &c : tagInfo.name ) c = toLower( c );
   tagInfo._params = String( nameEnd, end );

   return tagInfo;
   }

std::optional<String> TagInfo::valueOf( StringView param ) const
   {
   auto beginPos = _params.find( param );
   if ( beginPos == StringView::npos ) return std::nullopt;
   if ( ( beginPos += param.size( ) + 1 ) >= _params.size( ) ) return std::nullopt;
   if ( _params.at( beginPos ) == '\'' || _params.at( beginPos ) == '"' ) ++beginPos;
   const auto endPos = _params.find_first_of( "'\"", beginPos );
   return _params.substr( beginPos, endPos - beginPos );
   }

String TagInfo::getClosingTagString( ) const
   {
   if ( type != TagType::Opening )
      throw InvalidOperationException( "The tag is not an opening tag." );
   return STRING( "</" << name << ">" );
   }

std::istream &operator>>( std::istream &stream, LinkInfo &linkInfo )
   {
   int numAnchorWords;
   stream >> linkInfo.url >> numAnchorWords;
   linkInfo.anchorWords.reserve( numAnchorWords );
   for ( auto i = 0; i < numAnchorWords; ++i )
      {
      String word;
      stream >> word;
      linkInfo.anchorWords.emplace_back( std::move( word ) );
      }
   return stream;
   }

std::ostream &operator<<( std::ostream &stream, const LinkInfo &linkInfo )
   {
   stream << linkInfo.url << '\n'
          << linkInfo.anchorWords.size( ) << ' ';
   for ( const auto &word: linkInfo.anchorWords ) stream << word << ' ';
   return stream;
   }

std::istream &operator>>( std::istream &stream, HtmlInfo &htmlInfo )
   {
   int numWords;
   stream >> numWords;
   htmlInfo.words.reserve( numWords );
   for ( auto i = 0; i < numWords; ++i )
      {
      String word;
      stream >> word;
      htmlInfo.words.emplace_back( std::move( word ) );
      }

   int numTitleWords;
   stream >> numTitleWords;
   htmlInfo.titleWords.reserve( numTitleWords );
   for ( auto i = 0; i < numTitleWords; ++i )
      {
      String word;
      stream >> word;
      htmlInfo.titleWords.emplace_back( std::move( word ) );
      }

   int numLinks;
   stream >> numLinks;
   for ( auto i = 0; i < numTitleWords; ++i )
      {
      LinkInfo linkInfo;
      stream >> linkInfo;
      htmlInfo.links.emplace_back( std::move( linkInfo ) );
      }

   bool hasBase;
   stream >> hasBase;
   if ( hasBase )
      {
      Url base;
      stream >> base;
      htmlInfo.base = base;
      }

   return stream;
   }

std::ostream &operator<<( std::ostream &stream, const HtmlInfo &htmlInfo )
   {
   stream << htmlInfo.words.size( ) << ' ';
   for ( const auto &word: htmlInfo.words ) stream << word << ' ';
   stream << '\n' << htmlInfo.titleWords.size( ) << ' ';
   for ( const auto &word: htmlInfo.titleWords ) stream << word << ' ';
   stream << '\n' << htmlInfo.links.size( ) << '\n';
   for ( const auto &linkInfo: htmlInfo.links ) stream << linkInfo << '\n';
   stream << std::boolalpha << htmlInfo.base.has_value( );
   if ( htmlInfo.base.has_value( ) ) stream << ' ' << htmlInfo.base.value( );
   return stream;
   }

HtmlInfo HtmlParser::parse( StringView htmlString ) const
   {
   HtmlInfo htmlInfo;
   LinkInfo *currentLinkInfo = nullptr;

   for ( auto beginPos = 0, endPos = beginPos; beginPos != StringView::npos; beginPos = endPos )
      {
      // Focuses the view on the text before a tag.
      endPos = htmlString.find( '<', beginPos );

      // Tokenizes the text before a tag.
      auto words = tokenize( htmlString.substr( beginPos, endPos - beginPos ) );
      if ( currentLinkInfo != nullptr )
         std::copy( words.cbegin( ), words.cend( ), std::back_inserter( currentLinkInfo->anchorWords ) );
      std::move( words.begin( ), words.end( ), std::back_inserter( htmlInfo.words ) );

      // Focuses the view on a tag.
      if ( ( beginPos = endPos ) == StringView::npos ) break;
      endPos = htmlString.find( '>', beginPos );
      if ( endPos++ == StringView::npos )
         throw FormatException( "A closing angle bracket is missing." );

      // Parses the tag.
      const auto tagInfo = TagInfo::parse( htmlString.substr( beginPos, endPos - beginPos ) );
      const auto tagAction = _actionMap.contains( tagInfo.name ) ? _actionMap.at( tagInfo.name ) : TagAction::Discard;
      if ( tagInfo.type == TagType::Opening )
         switch ( tagAction )
            {
            case TagAction::Anchor: // Parses the link in the anchor tag.
               if ( auto urlString = tagInfo.valueOf( "href" );
                     urlString.has_value( ) && preprocessUrlString( urlString.value( ) ) )
                  {
                  try
                     {
                     Url url( urlString.value( ) );
                     if ( linkFilter( url, tagInfo ) )
                        {
                        htmlInfo.links.emplace_back( std::move( url ) );
                        currentLinkInfo = &htmlInfo.links.back( );
                        }
                     }
                  catch ( ... )
                     { }
                  }
               break;
            case TagAction::Base: // Parses the base URL.
               if ( auto urlString = tagInfo.valueOf( "href" );
                     urlString.has_value( ) && preprocessUrlString( urlString.value( ) ) &&
                     !htmlInfo.base.has_value( ) )
                  {
                  try
                     { htmlInfo.base.emplace( urlString.value( ) ); }
                  catch ( ... )
                     { }
                  }
               break;
            case TagAction::Discard: // Discards the tag.
               break;
            case TagAction::DiscardElement: // Advances the view past the element.
               {
               const auto closingTagString = tagInfo.getClosingTagString( );
               beginPos = endPos;
               if ( ( endPos = htmlString.find( closingTagString, beginPos ) ) == StringView::npos )
                  throw FormatException( "A closing tag is missing" );
               endPos += closingTagString.size( );
               break;
               }
            case TagAction::Embed: // Parses the link in the embed tag.
               if ( auto urlString = tagInfo.valueOf( "src" );
                     urlString.has_value( ) && preprocessUrlString( urlString.value( ) ) )
                  {
                  try
                     {
                     Url url( urlString.value( ) );
                     if ( linkFilter( url, tagInfo ) )
                        htmlInfo.links.emplace_back( std::move( url ) );
                     }
                  catch ( ... )
                     { }
                  }
               break;
            case TagAction::Title: // Tokenizes the text in the title element and advances the view past it.
               {
               const auto closingTagString = tagInfo.getClosingTagString( );
               beginPos = endPos;
               if ( ( endPos = htmlString.find( closingTagString, beginPos ) ) == StringView::npos )
                  throw FormatException( "A closing tag is missing" );

               words = tokenize( htmlString.substr( beginPos, endPos - beginPos ) );
               std::move( words.begin( ), words.end( ), std::back_inserter( htmlInfo.titleWords ) );

               endPos += closingTagString.size( );
               break;
               }
            }
      else if ( tagInfo.type == TagType::Closing )
         {
         if ( tagAction == TagAction::Anchor ) // Stops adding anchor words to the current link.
            currentLinkInfo = nullptr;
         }
      else if ( tagInfo.type == TagType::SelfClosing )
         {
         if ( tagAction == TagAction::Base ) // Parses the base URL.
            {
            if ( auto urlString = tagInfo.valueOf( "href" );
                  urlString.has_value( ) && preprocessUrlString( urlString.value( ) ) &&
                  !htmlInfo.base.has_value( ) )
               try
                  { htmlInfo.base.emplace( urlString.value( ) ); }
               catch ( ... )
                  { }
            }
         }
      }

   return htmlInfo;
   }

Vector<String> HtmlParser::tokenize( StringView string )
   {
   Vector<String> tokens;
   std::istringstream stream( String{ string } );
   String token;
   while ( stream >> token )
      if ( preprocessToken( token ) )
         tokens.emplace_back( std::move( token ) );
   return tokens;
   }

bool HtmlParser::preprocessToken( String &token )
   {
   const auto begin = std::find_if( token.cbegin( ), token.cend( ), [ ]( char c )
      { return std::isalnum( c ); } );
   if ( begin == token.cend( ) ) return false;
   const auto end = std::find_if( token.crbegin( ), token.crend( ), [ ]( char c )
      { return std::isalnum( c ); } ).base( );
   token = String( begin, end );
   for ( auto &c : token ) c = toLower( c );
   return true;
   }

bool HtmlParser::preprocessUrlString( String &urlString )
   {
   if ( std::any_of( urlString.cbegin( ), urlString.cend( ), [ ]( char c )
      { return std::isspace( c ); } ) )
      return false;

   if ( const auto pos = urlString.find( '#' ); pos != String::npos )
      {
      if ( pos == 0 ) return false;
      urlString = urlString.substr( 0, pos );
      }
   return true;
   }

const HashMap<StringView, HtmlParser::TagAction> HtmlParser::_actionMap = {
      { "!--",        TagAction::Discard },
      { "!doctype",   TagAction::Discard },
      { "a",          TagAction::Anchor },
      { "abbr",       TagAction::Discard },
      { "acronym",    TagAction::Discard },
      { "address",    TagAction::Discard },
      { "applet",     TagAction::Discard },
      { "area",       TagAction::Discard },
      { "article",    TagAction::Discard },
      { "aside",      TagAction::Discard },
      { "audio",      TagAction::Discard },
      { "b",          TagAction::Discard },
      { "base",       TagAction::Base },
      { "basefont",   TagAction::Discard },
      { "bdi",        TagAction::Discard },
      { "bdo",        TagAction::Discard },
      { "bgsound",    TagAction::Discard },
      { "big",        TagAction::Discard },
      { "blink",      TagAction::Discard },
      { "blockquote", TagAction::Discard },
      { "body",       TagAction::Discard },
      { "br",         TagAction::Discard },
      { "button",     TagAction::Discard },
      { "canvas",     TagAction::Discard },
      { "caption",    TagAction::Discard },
      { "center",     TagAction::Discard },
      { "cite",       TagAction::Discard },
      { "code",       TagAction::Discard },
      { "col",        TagAction::Discard },
      { "colgroup",   TagAction::Discard },
      { "content",    TagAction::Discard },
      { "data",       TagAction::Discard },
      { "datalist",   TagAction::Discard },
      { "dd",         TagAction::Discard },
      { "del",        TagAction::Discard },
      { "details",    TagAction::Discard },
      { "dfn",        TagAction::Discard },
      { "dialog",     TagAction::Discard },
      { "dir",        TagAction::Discard },
      { "div",        TagAction::Discard },
      { "dl",         TagAction::Discard },
      { "dt",         TagAction::Discard },
      { "em",         TagAction::Discard },
      { "embed",      TagAction::Embed },
      { "fieldset",   TagAction::Discard },
      { "figcaption", TagAction::Discard },
      { "figure",     TagAction::Discard },
      { "font",       TagAction::Discard },
      { "footer",     TagAction::Discard },
      { "form",       TagAction::Discard },
      { "frame",      TagAction::Discard },
      { "frameset",   TagAction::Discard },
      { "h1",         TagAction::Discard },
      { "h2",         TagAction::Discard },
      { "h3",         TagAction::Discard },
      { "h4",         TagAction::Discard },
      { "h5",         TagAction::Discard },
      { "h6",         TagAction::Discard },
      { "head",       TagAction::Discard },
      { "header",     TagAction::Discard },
      { "hgroup",     TagAction::Discard },
      { "hr",         TagAction::Discard },
      { "html",       TagAction::Discard },
      { "i",          TagAction::Discard },
      { "iframe",     TagAction::Discard },
      { "img",        TagAction::Discard },
      { "input",      TagAction::Discard },
      { "ins",        TagAction::Discard },
      { "isindex",    TagAction::Discard },
      { "kbd",        TagAction::Discard },
      { "keygen",     TagAction::Discard },
      { "label",      TagAction::Discard },
      { "legend",     TagAction::Discard },
      { "li",         TagAction::Discard },
      { "link",       TagAction::Discard },
      { "listing",    TagAction::Discard },
      { "main",       TagAction::Discard },
      { "map",        TagAction::Discard },
      { "mark",       TagAction::Discard },
      { "marquee",    TagAction::Discard },
      { "menu",       TagAction::Discard },
      { "menuitem",   TagAction::Discard },
      { "meta",       TagAction::Discard },
      { "meter",      TagAction::Discard },
      { "nav",        TagAction::Discard },
      { "nobr",       TagAction::Discard },
      { "noframes",   TagAction::Discard },
      { "noscript",   TagAction::Discard },
      { "object",     TagAction::Discard },
      { "ol",         TagAction::Discard },
      { "optgroup",   TagAction::Discard },
      { "option",     TagAction::Discard },
      { "output",     TagAction::Discard },
      { "p",          TagAction::Discard },
      { "param",      TagAction::Discard },
      { "picture",    TagAction::Discard },
      { "plaintext",  TagAction::Discard },
      { "pre",        TagAction::Discard },
      { "progress",   TagAction::Discard },
      { "q",          TagAction::Discard },
      { "rp",         TagAction::Discard },
      { "rt",         TagAction::Discard },
      { "rtc",        TagAction::Discard },
      { "ruby",       TagAction::Discard },
      { "s",          TagAction::Discard },
      { "samp",       TagAction::Discard },
      { "script",     TagAction::DiscardElement },
      { "section",    TagAction::Discard },
      { "select",     TagAction::Discard },
      { "shadow",     TagAction::Discard },
      { "slot",       TagAction::Discard },
      { "small",      TagAction::Discard },
      { "source",     TagAction::Discard },
      { "spacer",     TagAction::Discard },
      { "span",       TagAction::Discard },
      { "strike",     TagAction::Discard },
      { "strong",     TagAction::Discard },
      { "style",      TagAction::DiscardElement },
      { "sub",        TagAction::Discard },
      { "summary",    TagAction::Discard },
      { "sup",        TagAction::Discard },
      { "svg",        TagAction::DiscardElement },
      { "table",      TagAction::Discard },
      { "tbody",      TagAction::Discard },
      { "td",         TagAction::Discard },
      { "template",   TagAction::Discard },
      { "textarea",   TagAction::Discard },
      { "tfoot",      TagAction::Discard },
      { "th",         TagAction::Discard },
      { "thead",      TagAction::Discard },
      { "time",       TagAction::Discard },
      { "title",      TagAction::Title },
      { "tr",         TagAction::Discard },
      { "track",      TagAction::Discard },
      { "tt",         TagAction::Discard },
      { "u",          TagAction::Discard },
      { "ul",         TagAction::Discard },
      { "var",        TagAction::Discard },
      { "video",      TagAction::Discard },
      { "wbr",        TagAction::Discard },
      { "xmp",        TagAction::Discard },
};
