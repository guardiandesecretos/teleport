#ifndef TELEPORT_LOG
#define TELEPORT_LOG

#include "base/util_log.h"
#include "define.h"
#include <exception>
#include <set>

#ifdef LOG_CATEGORY
#undef LOG_CATEGORY
#endif
#define LOG_CATEGORY "unspecified"


class Logger
{
public:
    Logger& operator<<(unsigned long n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%lu", n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(unsigned long long n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%llu", n);
        return *this << std::string(buffer);
    }
    Logger& operator<<(long n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%ld", n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(long long n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%lld", n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(double n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%lf", n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(unsigned int n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%u", n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(int n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%d", n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(uint8_t n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%02x", n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(char n)
    {
        char buffer[BUFSIZ];
        sprintf(buffer, "%02x", (uint8_t)n);
        return *this << std::string(buffer);
    }

    Logger& operator<<(vch_t bytes)
    {
        for (uint32_t i = 0; i < bytes.size(); i++)
            *this << bytes[i];
        return *this;
    }

    Logger& operator<<(bool b)
    {
        return *this << std::string(b ? "true" : "false");
    }

    template <typename V>
    Logger& operator<<(V value)
    {
        return *this << value.ToString();
    }

    template <typename V>
    Logger& operator<<(std::vector<V> values)
    {
        *this << "(";
        for (uint32_t i = 0; i < values.size(); i++)
            *this << values[i] << ",";
        *this << ")";
        return *this;
    }

    template <typename V>
    Logger& operator<<(std::set<V> values)
    {
        *this << "(";
        for (auto it = values.begin(); it != values.end(); it++)
            *this << *it << ",";
        *this << ")";
        return *this;
    }

    template <typename V, typename W>
    Logger& operator<<(std::pair<V, W> value)
    {
        *this << "(";
        *this << value.first;
        *this << ",";
        *this << value.second;
        *this << ")";
        return *this;
    }

    Logger& operator<<(std::string value)
    {
        return *this << value.c_str();
    }

    Logger& operator<<(const char* value)
    {
        LogPrint(NULL, value);
        return *this;
    }
};


#define log_ Logger() << LOG_CATEGORY << ": "

#endif

#undef LOG_CATEGORY