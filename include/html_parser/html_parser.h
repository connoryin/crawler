#pragma once

#include <optional>

#include "core/exception.h"
#include "core/io.h"
#include "core/net.h"
#include "core/string.h"
#include "core/vector.h"

/// Defines tag types.
enum class TagType
   {
      Opening,
      Closing,
      SelfClosing
   };

/// Represents the parsed information of a tag.
struct TagInfo
   {
   public:
      TagType type; ///< The type of the tag.
      String name; ///< The name of the tag.

      /// Parses a tag.
      /// \param tagString The string representation of the tag.
      /// \return The parsed information of the tag.
      /// \throw FormatException The tag is malformed.
      static TagInfo parse( StringView tagString );

      /// Gets the value of a parameter if it exists.
      /// \param param The parameter name.
      /// \return The value of a parameter if it exists.
      [[nodiscard]] std::optional<String> valueOf( StringView param ) const;

      /// Gets the string representation of the corresponding closing tag.
      /// \return The string representation of the corresponding closing tag.
      /// \throw InvalidOperationException The tag is not an opening tag.
      [[nodiscard]] String getClosingTagString( ) const;

   private:
      String _params;
   };

/// Represents the parsed information of a hyperlink.
struct LinkInfo
   {
   public:
      Url url; ///< The URL of the hyperlink.
      Vector<String> anchorWords; ///< The anchor words that appear in some HTML files.

      /// Initializes a `Link` with a specified URL.
      /// \param url The URL of the hyperlink.
      explicit LinkInfo( Url url ) noexcept: url( std::move( url ) )
         { }

      /// Initializes a `Link` with a specified URL string.
      /// \param url The URL string of the hyperlink.
      explicit LinkInfo( StringView urlString = "" ) noexcept: url( urlString )
         { }

      friend std::istream &operator>>( std::istream &stream, LinkInfo &linkInfo );

      friend std::ostream &operator<<( std::ostream &stream, const LinkInfo &linkInfo );
   };

/// Represents the parsed information from an HTML file.
struct HtmlInfo
   {
   public:
      Vector<String> words; ///< The words that appear in the HTML file, converted to lowercase.
      Vector<String> titleWords; ///< The words that appear in the HTML tile, converted to lowercase.
      Vector<LinkInfo> links; ///< The links that appear in the HTML file.
      std::optional<Url> base; ///< The base URL of the HTML file.

      friend std::istream &operator>>( std::istream &stream, HtmlInfo &htmlInfo );

      friend std::ostream &operator<<( std::ostream &stream, const HtmlInfo &htmlInfo );
   };

/// Parses information from an HTML file.
class HtmlParser
   {
   public:
      ///< Checks if a hyperlink will be included in the parsed result.
      std::function<bool( const Url &, const TagInfo & )> linkFilter = [ ]( const Url &, const TagInfo & )
         { return true; };

      /// Parses an HTML file.
      /// \param htmlString The string representation of an HTML file.
      /// \return The parsed information from the HTML file.
      /// \throw FormatException The HTML file is malformed.
      [[nodiscard]] HtmlInfo parse( StringView htmlString ) const;

   private:
      enum class TagAction
         {
            Anchor,
            Base,
            Discard,
            DiscardElement,
            Embed,
            Title
         };

      static Vector<String> tokenize( StringView string );

      static bool preprocessToken( String &token );

      static bool preprocessUrlString( String &urlString );

      static const HashMap<StringView, TagAction> _actionMap;
   };
