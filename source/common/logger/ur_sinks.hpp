// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef UR_SINKS_HPP
#define UR_SINKS_HPP 1

#include <fstream>
#include <iostream>

namespace logger {

class Sink {
  public:
    template <typename... Args>
    void log(logger::Level level, const char *fmt, Args &&...args) {
        *ostream << "[" << level_to_str(level) << "]:";
        format(fmt, std::forward<Args &&>(args)...);
        *ostream << "\n";
        if (level >= flush_level) {
            ostream->flush();
        }
    }

    void setFlushLevel(logger::Level level) { this->flush_level = level; }

    virtual ~Sink() = default;

  protected:
    std::ostream *ostream;
    logger::Level flush_level;

    Sink() { flush_level = logger::Level::ERR; }

  private:
    void format(const char *fmt) {
        while (*fmt != '\0') {
            while (*fmt != '{' && *fmt != '}' && *fmt != '\0') {
                *ostream << *fmt++;
            }

            if (*fmt == '{') {
                if (*(++fmt) == '{') {
                    *ostream << *fmt++;
                } else {
                    throw std::runtime_error("No arguments provided and braces not escaped!");
                }
            } else if (*fmt == '}') {
                if (*(++fmt) == '}') {
                    *ostream << *fmt++;
                } else {
                    throw std::runtime_error("Closing curly brace not escaped!");
                }
            }
        }
    }

    template <typename Arg>
    void format(const char *fmt, Arg &&arg) {
        while (*fmt != '\0') {
            while (*fmt != '{' && *fmt != '}' && *fmt != '\0') {
                *ostream << *fmt++;
            }

            if (*fmt == '{') {
                if (*(++fmt) == '{') {
                    *ostream << *fmt++;
                } else if (*fmt != '}') {
                    throw std::runtime_error("Only empty braces are allowed!");
                } else {
                    *ostream << arg;
                    fmt++;
                }
            } else if (*fmt == '}') {
                if (*(++fmt) == '}') {
                    *ostream << *fmt++;
                } else {
                    throw std::runtime_error("Closing curly brace not escaped!");
                }
            }
        }
    }

    template <typename Arg, typename... Args>
    void format(const char *fmt, Arg &&arg, Args &&...args) {
        bool arg_printed = false;
        while (!arg_printed) {
            while (*fmt != '{' && *fmt != '}' && *fmt != '\0') {
                *ostream << *fmt++;
            }

            if (*fmt == '{') {
                if (*(++fmt) == '{') {
                    *ostream << *fmt++;
                } else if (*fmt != '}') {
                    throw std::runtime_error("Only empty braces are allowed!");
                } else {
                    *ostream << arg;
                    arg_printed = true;
                }
            } else if (*fmt == '}') {
                if (*(++fmt) == '}') {
                    *ostream << *fmt++;
                } else {
                    throw std::runtime_error("Closing curly brace not escaped!");
                }
            }
        }

        format(++fmt, std::forward<Args &&>(args)...);
    }
};

class StdoutSink : public Sink {
  public:
    StdoutSink() { this->ostream = &std::cout; }

    StdoutSink(Level flush_lvl) : StdoutSink() {
        this->flush_level = flush_lvl;
    }

    ~StdoutSink() = default;
};

class StderrSink : public Sink {
  public:
    StderrSink() { this->ostream = &std::cerr; }

    StderrSink(Level flush_lvl) : StderrSink() {
        this->flush_level = flush_lvl;
    }

    ~StderrSink() = default;
};

class FileSink : public Sink {
  public:
    FileSink(std::string file_path) {
        ofstream = std::ofstream(file_path, std::ofstream::out);
        if (ofstream.rdstate() != std::ofstream::goodbit) {
            throw std::invalid_argument(
                std::string("Failure while opening log file: ") + file_path +
                std::string(" Check if given path exists."));
        }
        this->ostream = &ofstream;
    }

    FileSink(std::string file_path, Level flush_lvl) : FileSink(file_path) {
        this->flush_level = flush_lvl;
    }

  private:
    std::ofstream ofstream;
};

} // namespace logger

#endif /* UR_SINKS_HPP */
