#pragma once

#include <fstream>

#include "core/concurrency/mutex.h"
#include "core/exception.h"
#include "core/memory.h"
#include "core/string.h"

/// Writes characters to a stream.
class StreamWriter
   {
   public:
      /// Initializes a `StreamWriter` for the specified stream.
      /// \param stream The stream to write to.
      explicit StreamWriter( std::ostream &stream ) noexcept: _stream( &stream ), _ownsStream( false )
         { }

      /// Initializes a `StreamWriter` for the specified file.
      /// \param path The file path to write to.
      /// \param append `true` to append to the file; `false` to overwrite the file.
      /// \throw IOException The file cannot be opened.
      explicit StreamWriter( StringView path, bool append = false ) :
            _stream( new std::ofstream( path.data( ), append ? std::ofstream::app : std::ofstream::out ) ),
            _ownsStream( true )
         {
         if ( !dynamic_cast<std::ofstream *>(_stream)->is_open( ) )
            throw IOException( "The file cannot be opened." );
         }

      StreamWriter( const StreamWriter & ) = delete;
      StreamWriter &operator=( const StreamWriter & ) = delete;

      StreamWriter( StreamWriter &&other ) noexcept:
            _stream( std::exchange( other._stream, nullptr ) ), _ownsStream( std::exchange( other._ownsStream, false ) )
         { }
      StreamWriter &operator=( StreamWriter &&other ) noexcept
         {
         if ( this != &other )
            {
            if ( _stream != nullptr ) _stream->flush( );
            if ( _ownsStream ) delete _stream;
            _stream = std::exchange( other._stream, nullptr );
            _ownsStream = std::exchange( other._ownsStream, false );
            }
         return *this;
         }

      ~StreamWriter( )
         {
         if ( _stream != nullptr ) _stream->flush( );
         if ( _ownsStream ) delete _stream;
         }

      /// Creates a thread-safe wrapper around the specified `StreamWriter`.
      /// \param writer The `StreamWriter` to synchronize.
      /// \return A thread-safe wrapper.
      static UniquePtr<StreamWriter> synchronized( StreamWriter &&writer );

      /// Indicates whether the `StreamWriter` will flush after each write.
      /// \return `true` if the `StreamWriter` will flush after each write.
      [[nodiscard]] bool autoFlush( ) const
         { return _stream->flags( ) & std::ostream::unitbuf; }

      /// Sets whether the `StreamWriter` will flush after each write.
      /// \param value `true` if the `StreamWriter` will flush after each write.
      void setAutoFlush( bool value )
         { *_stream << ( value ? std::unitbuf : std::nounitbuf ); }

      /// Gets the underlying stream.
      /// \return The underlying stream.
      std::ostream &baseStream( ) noexcept
         { return *_stream; }

      /// Writes a string to the stream.
      /// \param value The string to write.
      virtual void write( StringView value )
         { *_stream << value; }

      /// Writes a string and a new line to the stream.
      /// \param value The string to write.
      virtual void writeLine( StringView value )
         { *_stream << value << '\n'; }

      /// Causes any buffered data to be written to the underlying stream.
      virtual void flush( )
         { _stream->flush( ); }

      /// Closes the underlying stream.
      void close( )
         { if ( auto *const stream = dynamic_cast<std::ofstream *>(_stream); stream != nullptr ) stream->close( ); }

   private:
      class SyncStreamWriter;

      std::ostream *_stream;
      bool _ownsStream;
   };

class StreamWriter::SyncStreamWriter : public StreamWriter
   {
   public:
      explicit SyncStreamWriter( StreamWriter &&writer ) noexcept: StreamWriter( std::move( writer ) )
         { }

      void write( StringView value ) override
         {
         UniqueLock lock( _mutex );
         StreamWriter::write( value );
         }

      void writeLine( StringView value ) override
         {
         UniqueLock lock( _mutex );
         StreamWriter::writeLine( value );
         }

      void flush( ) override
         {
         UniqueLock lock( _mutex );
         StreamWriter::flush( );
         }

   private:
      mutable Mutex _mutex;
   };

inline UniquePtr<StreamWriter> StreamWriter::synchronized( StreamWriter &&writer )
   { return makeUnique<SyncStreamWriter>( std::move( writer ) ); }
