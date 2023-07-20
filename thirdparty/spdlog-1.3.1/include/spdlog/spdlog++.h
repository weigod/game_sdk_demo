#ifndef _SPDLOGGER_LOGGER_PP_H_
#define _SPDLOGGER_LOGGER_PP_H_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <thread>
#include "formatter.h"
#include "spdlog.h"
#include <memory>
#include <mutex>
#include "sinks/stdout_color_sinks.h"
#include "sinks/rotating_file_sink.h"

using namespace std;

#ifdef _WIN32
#include <windows.h>
static bool CreateDir(std::string path)
{
    BOOL r = CreateDirectoryA(path.c_str(), NULL);
    if (r == TRUE)
        return true;

    return GetLastError() == ERROR_ALREADY_EXISTS;
}
#else
static bool CreateDir(std::string path)
{
    int r = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (r == 0)
        return true;

    return (errno == EEXIST);
}
#endif //! #ifdef _WIN32

enum YLogLevel
{
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4
};

#define YLOG_MSG_BUFF_SIZE 1024 * 1024 * 1  // 1M
#define YLOG_MAX_LOG_SIZE 1024 * 1024 * 500 // 500M
#define YLOG_MAX_LOG_FILE 10

typedef void (*Core_LogCallback)(int priority, const char *message);

struct YLogMessageCacheItem
{
    int level;
    std::string msg;
};

class YLogger
{
public:
    YLogger(const std::string &name);
    YLogger(const std::string &name, Core_LogCallback cb);
    ~YLogger()
    {
        delete[] msg_buff;
    };

    bool InitLogger();

    void Step();

    void AttachOutputToStd();

    void SetRotating(int max_size, int max_file)
    {
        max_log_size = max_size;
        max_log_file = max_file;
    };
    void SetLogDir(const std::string &logdir, const std::string &logfile)
    {
        log_dir = logdir;
        log_file = logfile;
    };
    void SetLogName(const std::string &name)
    {
        log_name = name;
    };
    void SetLogLevel(int level)
    {
        log_level = level;
    };

    int LogMsg(int level, const char *msg);
    int LogFormatMsg(int level, const char *fmt, ...);
    int LogMsg(int level, const std::string &msg);

private:
    YLogger(const YLogger &src);
    YLogger &operator=(const YLogger &src);

private:
    std::shared_ptr<spdlog::logger> logger;
    std::vector<spdlog::sink_ptr> sinks;
    std::string log_name;
    std::string log_dir;
    std::string log_file;
    int log_level;

    int max_log_size;
    int max_log_file;

    Core_LogCallback callback;
    std::list<YLogMessageCacheItem> cacheMessages;
    std::thread::id thread_id;
    char *msg_buff;
    std::mutex buff_locker;
};

extern YLogger *defaultLogger;

class YLogMessage
{
public:
    YLogMessage(int level, const std::string &file, const std::string &func, int line, YLogger *logger);
    ~YLogMessage();
    std::ostream &stream()
    {
        return data;
    };

private:
    int level;
    YLogger *pLogger;
    std::ostringstream data;
    int line;
    std::string file;
    std::string func;
};

inline YLogger::YLogger(const std::string &name)
    : log_name(name)
    , max_log_size(YLOG_MAX_LOG_SIZE)
    , max_log_file(YLOG_MAX_LOG_FILE)
    , log_level(Debug)
    , callback(nullptr)
    , msg_buff(new char[YLOG_MSG_BUFF_SIZE])
{
    thread_id = std::this_thread::get_id();
};

inline YLogger::YLogger(const std::string &name, Core_LogCallback cb)
    : log_name(name)
    , max_log_size(YLOG_MAX_LOG_SIZE)
    , max_log_file(YLOG_MAX_LOG_FILE)
    , log_level(Info)
    , callback(cb)
    , msg_buff(new char[YLOG_MSG_BUFF_SIZE])
{
    thread_id = std::this_thread::get_id();
}

inline void YLogger::AttachOutputToStd()
{
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
}

inline bool YLogger::InitLogger()
{
    if (callback) // if callback != null_ptr it means working in unity, no need to create log files.
        return true;

    if (CreateDir(log_dir) == false)
    {
        std::cout << "Log initialization failed: can't create dir:" << log_dir << std::endl;
        return false;
    }

    string name = string(log_dir) + "/" + string(log_file);

    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(name, max_log_size, max_log_file));

    logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));

    logger->set_level((spdlog::level::level_enum)log_level);
    spdlog::set_pattern("[%H:%M:%S %z] [%^---%L---%$] [thread %t] %v");

#ifdef NODEBUG
    logger->flush_on(spdlog::level::info);
#else
    logger->flush_on(spdlog::level::trace);
#endif // DEBUG

    spdlog::register_logger(logger);

#ifndef WIN32
    string dst = string(log_dir) + "/lastest.log";
    unlink(dst.c_str());
    symlink(name.c_str(), dst.c_str());
#endif
    return true;
}

inline void YLogger::Step()
{
    for (auto item : cacheMessages)
    {
        if (thread_id == std::this_thread::get_id())
            (callback)(item.level, item.msg.c_str());
    }
    cacheMessages.clear();
}

inline int YLogger::LogMsg(int level, const char *msg)
{
    if (callback != nullptr)
    {
        if (thread_id == std::this_thread::get_id())
            (callback)(level, msg);
        else
            cacheMessages.push_back({level, msg});
        return 0;
    }

    switch (level)
    {
    case YLogLevel::Trace:
        logger->trace(msg);
        break;
    case YLogLevel::Debug:
        logger->debug(msg);
        break;
    case YLogLevel::Info:
        logger->info(msg);
        break;
    case YLogLevel::Warn:
        logger->warn(msg);
        break;
    case YLogLevel::Error:
        logger->error(msg);
        break;
    default:
        logger->debug(msg);
        break;
    }
    return 0;
}

inline int YLogger::LogFormatMsg(int level, const char *fmt, ...)
{
    if (!fmt || strlen(fmt) == 0)
        return 0;

    std::string msg;
    {
        std::lock_guard<std::mutex> g(buff_locker);
        va_list ap;
        va_start(ap, fmt);
        int size = vsnprintf(msg_buff, YLOG_MSG_BUFF_SIZE, fmt, ap);
        va_end(ap);
        msg = msg_buff;
    }
    LogMsg(level, msg);
    return 0;
}

inline int YLogger::LogMsg(int level, const string &msg)
{
    return LogMsg(level, msg.c_str());
}

inline YLogMessage::YLogMessage(int level, const std::string &file, const std::string &func, int line, YLogger *logger)
    : level(level)
    , file(file)
    , func(func)
    , line(line)
{
    pLogger = logger;
}

inline YLogMessage::~YLogMessage()
{
    if (!pLogger)
        return;

    pLogger->LogFormatMsg(level, "[%s:%d#%s] %s", file.c_str(), line, func.c_str(), data.str().c_str());
}

#define SPDLOG_TRACE YLogMessage(YLogLevel::Trace, __FILE__, __FUNCTION__, __LINE__, defaultLogger).stream()
#define SPDLOG_DEBUG YLogMessage(YLogLevel::Debug, __FILE__, __FUNCTION__, __LINE__, defaultLogger).stream()
#define SPDLOG_INFO YLogMessage(YLogLevel::Info, __FILE__, __FUNCTION__, __LINE__, defaultLogger).stream()
#define SPDLOG_WARNING YLogMessage(YLogLevel::Warn, __FILE__, __FUNCTION__, __LINE__, defaultLogger).stream()
#define SPDLOG_ERROR YLogMessage(YLogLevel::Error, __FILE__, __FUNCTION__, __LINE__, defaultLogger).stream()

#define SPDLOG(x) SPDLOG_##x

#define SPDLOG_TRACE_(logger) YLogMessage(YLogLevel::Trace, __FILE__, __FUNCTION__, __LINE__, logger).stream()
#define SPDLOG_DEBUG_(logger) YLogMessage(YLogLevel::Debug, __FILE__, __FUNCTION__, __LINE__, logger).stream()
#define SPDLOG_INFO_(logger) YLogMessage(YLogLevel::Info, __FILE__, __FUNCTION__, __LINE__, logger).stream()
#define SPDLOG_WARNING_(logger) YLogMessage(YLogLevel::Warn, __FILE__, __FUNCTION__, __LINE__, logger).stream()
#define SPDLOG_ERROR_(logger) YLogMessage(YLogLevel::Error, __FILE__, __FUNCTION__, __LINE__, logger).stream()

#define SPDLOG_(x, logger) SPDLOG_##x##_(logger)

#endif //! _SPDLOGGER_LOGGER_PP_H_
