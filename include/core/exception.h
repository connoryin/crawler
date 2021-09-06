#pragma once

#include <exception>

#include "core/string.h"

/// Represents errors that occur during application execution.
struct Exception : private std::exception
   {
   public:
      /// Initializes an `Exception`.
      Exception( ) noexcept = default;

      /// Initializes an `Exception` with a specified error message.
      /// \param message The error message.
      explicit Exception( String message ) noexcept: _message( std::move( message ) )
         { }

      /// Initializes an `Exception` with a specified error message and the exception that causes this exception.
      /// \param message The error message.
      /// \param innerException The exception that causes this exception
      Exception( std::string message, const Exception &innerException ) noexcept:
            _message( std::move( message ) ), _innerException( &innerException )
         { }

      /// Gets the error message.
      /// \return The error message.
      [[nodiscard]] virtual String message( ) const
         { return _message; }

      /// Gets the exception that caused this exception.
      /// \return The exception that caused this exception.
      [[nodiscard]] const Exception *innerException( ) const noexcept
         { return _innerException; }

   private:
      std::string _message;
      const Exception *_innerException = nullptr;
   };

/// The exception that is thrown when a function is not implemented.
struct NotImplementedException : public Exception
   {
   public:
      using Exception::Exception;
   };

/// The exception that is thrown when an argument is invalid.
struct ArgumentException : public Exception
   {
   public:
      using Exception::Exception;
   };

/// The exception that is thrown when a function call in invalid for the object's state.
struct InvalidOperationException : public Exception
   {
   public:
      using Exception::Exception;
   };

/// The exception that is thrown when the format of an argument is invalid.
struct FormatException : public Exception
   {
   public:
      using Exception::Exception;
   };

/// The exception that is thrown when a system error occurs.
struct SystemException : public Exception
   {
   public:
      /// Initializes a `SystemException` with the specified error code.
      /// \param errorCode The error code.
      explicit SystemException( int errorCode = errno ) noexcept: _errorCode( errorCode )
         { }

      /// Gets the error code.
      /// \return The error code.
      [[nodiscard]] int errorCode( ) const noexcept
         { return _errorCode; }

      [[nodiscard]] String message( ) const override
         { return std::strerror( _errorCode ); }

   private:
      int _errorCode;
   };

/// The exception that is thrown when an IO error occurs.
struct IOException : public Exception
   {
   public:
      using Exception::Exception;
   };
