
#include "logger.h"

#include <mutex>
#include <iomanip>
#include <atomic>
#include <deque>
#include <string>

#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static std::atomic<LoggerStream::OutputHandler> outputHandler {nullptr};
static std::atomic<LoggerStream::Level> severityLevel {LoggerStream::Debug};
static std::atomic<FILE *> outputStream = {stderr};
static void logHandler(LoggerStream::Level, const char *s);
static char logLevelToChar(LoggerStream::Level level);

static std::mutex prefixMutex;
static std::string messagePrefix;
static std::string applicationPrefix;

static std::mutex logFileNameMutex;
static std::string logFileName;

thread_local std::deque<std::shared_ptr<LoggerStream::Stream>> LoggerStream::pool;

namespace
{
    struct CloseStream
    {
        ~CloseStream()
        {
            FILE *stream = std::atomic_exchange(&outputStream, stderr);
            if (stream != stderr)
                fclose(stream);
        }

    } closeStream;
}

LoggerStream::LoggerStream(Level level)
{
    Level currentSeverityLevel = std::atomic_load(&severityLevel);

    if (level >= currentSeverityLevel)
    {
        stream = getFromPool();
        stream->str.clear();
        stream->level = level;
        stream->space = true;
        stream->quote = false;

        timeval tv;
        ::gettimeofday(&tv, NULL);

        struct tm tm;
        localtime_r(&tv.tv_sec, &tm);

        char header[128];

        {
            std::lock_guard<std::mutex> lock(prefixMutex);
            snprintf(header, sizeof(header) - 1,
                     "%s%02d.%02d.%d %02d:%02d:%02d.%03d %c [%d] %s: ", applicationPrefix.c_str(),
                     tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
                     (int)tv.tv_usec / 1000, logLevelToChar(level), getpid(),
                     messagePrefix.c_str());
        }

        // ensure last char is null terminator
        header[sizeof(header) - 1] = 0;

        stream->str += header;

    }
}

LoggerStream::~LoggerStream()
{
    if (stream.unique())
    {
        logHandler(stream->level, stream->str.c_str());
        pushToPool(std::move(stream));
    }
}

void LoggerStream::setOutputHandler(OutputHandler handler)
{
    std::atomic_store(&outputHandler, handler);
}

void LoggerStream::setSeverityLevel(Level level)
{
    std::atomic_store(&severityLevel, level);
}

void LoggerStream::setSeverityLevel(const std::string &level)
{
    Level severity = Level::Info;

    if( level == "debug" )
        severity = Level::Debug;
    else if( level == "info" )
        severity = Level::Info;
    else if ( level == "warning" )
        severity = Level::Warning;
    else if( level == "error" )
        severity = Level::Error;

    setSeverityLevel(severity);
}

void LoggerStream::setApplicationPrefix(std::string prefix)
{
    if (!prefix.empty())
        prefix += ' ';

    std::lock_guard<std::mutex> lock(prefixMutex);
    applicationPrefix = std::move(prefix);
}

void LoggerStream::setMessagePrefix(std::string prefix)
{
    std::lock_guard<std::mutex> lock(prefixMutex);
    messagePrefix = std::move(prefix);
}

void LoggerStream::setLogFileName(std::string fileName)
{
    {
        std::lock_guard<std::mutex> lock(logFileNameMutex);
        logFileName = std::move(fileName);
    }
    rotateFile();
}

void LoggerStream::rotateFile()
{
    std::string fileName;

    {
        std::lock_guard<std::mutex> lock(logFileNameMutex);
        fileName = logFileName;
    }

    if (!fileName.empty())
    {
        FILE *newFile = fopen(fileName.c_str(), "a");

        if (!newFile)
        {
            std::string message = "cannot open log file '";
            message += logFileName;
            message += "'";

            fprintf(stderr, "%s: %s\n", message.c_str(), strerror(errno));
        }
        else
        {
            FILE *previousFile = std::atomic_exchange(&outputStream, newFile);

            if (previousFile && previousFile != stderr)
            {
                fclose(previousFile);
            }
        }
    }
}

std::shared_ptr<LoggerStream::Stream> LoggerStream::getFromPool()
{
    if (pool.empty())
    {
        return std::move(std::make_shared<LoggerStream::Stream>());
    }
    else
    {
        std::shared_ptr<LoggerStream::Stream> stream{std::move(pool.back())};
        pool.pop_back();
        return std::move(stream);
    }
}

void LoggerStream::pushToPool(std::shared_ptr<LoggerStream::Stream> stream)
{
    pool.push_back(std::move(stream));
}

LoggerStream logDebug()
{
    return LoggerStream(LoggerStream::Debug);
}

LoggerStream logInfo()
{
    return LoggerStream(LoggerStream::Info);
}

LoggerStream logWarning()
{
    return LoggerStream(LoggerStream::Warning);
}

LoggerStream logError()
{
    return LoggerStream(LoggerStream::Error);
}

LoggerStream logFatal()
{
    return LoggerStream(LoggerStream::Fatal);
}

static void logHandler(LoggerStream::Level level, const char *s)
{
    auto handler = std::atomic_load(&outputHandler);

    if (handler)
    {
        handler(level, s);
    }
    else
    {
        FILE *stream = std::atomic_load(&outputStream);
        fprintf(stream, "%s\n", s);
        fflush(stream);
    }

    if (level == LoggerStream::Fatal)
        abort();
}

static char logLevelToChar(LoggerStream::Level level)
{
    switch(level)
    {
    case LoggerStream::Debug:
        return 'D';
    case LoggerStream::Info:
        return 'I';
    case LoggerStream::Warning:
        return 'W';
    case LoggerStream::Error:
    case LoggerStream::Fatal:
        return 'E';
        break;
    default:
        return '?';
        break;
    }
}

