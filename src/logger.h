#pragma once

#include <memory>
#include <sstream>
#include <deque>
#include <cassert>

/*!
 * Simple logger.
 *
 * Usage:
 * \code
 *  logInfo() << "string" << "to" << "log" << 10;
 *  logInfo().nospace() << "string" << "to" << "log" << 10;
 *  logInfo().quote() << "string" << "to" << "log" << 10;
 * \endcode
 *
 * Ouput:
 *  03.08.2017 12:44:15.737 I [26629] : string to log 10
 *  03.08.2017 12:44:15.737 I [26629] : stringtolog10
 *  03.08.2017 12:44:15.737 I [26629] : "string" "to" "log" "10"
 *
 *  For custom types you must privide std::to_string overload.
 */
class LoggerStream
{
public:
    enum Level
    {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    //! Create the debug stream. Default separate by spaces and without quote marks.
    LoggerStream(Level level);
    LoggerStream(const LoggerStream &) = default;
    ~LoggerStream();

    //! Separate by a space.
    LoggerStream &space();
    //! Don't separate by a space.
    LoggerStream &nospace();

    //! Insert a quote marks
    LoggerStream &quote();
    //! Don't insert quote marks
    LoggerStream &noquote();

    LoggerStream &operator << (const char *s);
    LoggerStream &operator << (char *s);
    LoggerStream &operator << (const std::string &s);
    LoggerStream &operator << (char c);

    template<typename T>
    LoggerStream &operator << (const T &t);

    //! Custom log handler type. Called from destructor, must be noexcept.
    typedef void(*OutputHandler)(Level level, const char *s);

    //! Sets the output handler function
    static void setOutputHandler(OutputHandler handler);

    //! Sets the severity level
    static void setSeverityLevel(Level level);

    //! Sets the severity level by string
    static void setSeverityLevel(const std::string &level);

    //! Set prefix for all log messages. Used for quick search by email.
    //! Example:
    //! \code
    //!     LoggerStream::setMessagePrefix("PREFIX");
    //!     logIndo() << "test";
    //! \endcode
    //!
    //! Output:
    //!     03.08.2017 12:44:15.737 I [26629] PREFIX: output.
    //!
    //! Put empty string to remove prefix.
    static void setMessagePrefix(std::string prefix);

    //! Sets application prefix
    static void setApplicationPrefix(std::string prefix);

    //! Set logger filename. By default used stderr.
    static void setLogFileName(std::string fileName);

    //! Reopen log file.
    static void rotateFile();

private:
    template<typename T>
    void addLogMessage(const T &s);

    struct Stream
    {
        Stream() = default;
        ~Stream() = default;

        Stream(Stream &&) = delete;
        Stream(const Stream &) = delete;
        Stream & operator = (const Stream &) = delete;

        std::string str;
        Level level;
        bool space;
        bool quote;
    };

    // +30% for perfomance
    static std::shared_ptr<Stream> getFromPool();
    static void pushToPool(std::shared_ptr<Stream> stream);
    static thread_local std::deque<std::shared_ptr<Stream>> pool;

    std::shared_ptr<Stream> stream;
};

//! Creates a debug stream.
LoggerStream logDebug();
//! Creates a debug stream for a info messages.
LoggerStream logInfo();
//! Creates a debug stream for warnings.
LoggerStream logWarning();
//! Creates a debug stream for error.
LoggerStream logError();
//! Creates a debug stream for fatal error. Never returns.
LoggerStream logFatal();

inline LoggerStream &LoggerStream::space()
{
    if (stream)
    {
        stream->space = true;
    }
    return *this;
}

inline LoggerStream &LoggerStream::nospace()
{
    if (stream)
    {
        stream->space = false;
    }
    return *this;
}

inline LoggerStream &LoggerStream::quote()
{
    if (stream)
    {
        stream->quote = true;
    }
    return *this;
}

inline LoggerStream &LoggerStream::noquote()
{
    if (stream)
    {
        stream->quote = false;
    }
    return *this;
}

inline LoggerStream &LoggerStream::operator << (const char *s)
{
    if (stream)
    {
        addLogMessage(s);
    }
    return *this;
}

inline LoggerStream &LoggerStream::operator << (char *s)
{
    if (stream)
    {
        addLogMessage(s);
    }
    return *this;
}


inline LoggerStream &LoggerStream::operator << (const std::string &s)
{
    if (stream)
    {
        addLogMessage(s);
    }
    return *this;
}

inline LoggerStream &LoggerStream::operator << (char c)
{
    if (stream)
    {
        addLogMessage(std::string{&c, 1});
    }
    return *this;
}

template<typename T>
inline LoggerStream &LoggerStream::operator << (const T &s)
{
    if (stream)
    {
        addLogMessage(std::to_string(s));
    }
    return *this;
}

template<typename T>
void LoggerStream::addLogMessage(const T &s)
{
    assert(stream);
    if (stream->space)
    {
        stream->str += ' ';
    }
    if (stream->quote)
    {
        stream->str += '"';
    }

    stream->str += s;

    if (stream->quote)
    {
        stream->str += '"';
    }
}

