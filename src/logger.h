/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef XILENS_LOGGER_H
#define XILENS_LOGGER_H

#include <boost/filesystem.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

enum my_severity_level
{
    trace = boost::log::trivial::trace,
    debug = boost::log::trivial::debug,
    info = boost::log::trivial::info,
    warning = boost::log::trivial::warning,
    error = boost::log::trivial::error,
    fatal = boost::log::trivial::fatal
};

namespace logging = boost::log;

typedef logging::sources::severity_channel_logger_mt<logging::trivial::severity_level> XILENS_LOGGER;

BOOST_LOG_GLOBAL_LOGGER(my_logger, XILENS_LOGGER)

/**
 * Defines custom logger that includes filenames, functions and line numbers
 */
#define LOG_XILENS(lvl)                                                                                                \
    BOOST_LOG_CHANNEL_SEV(my_logger::get(), "XILENS", logging::trivial::lvl)                                           \
        << __FILE__ << ":" << (__LINE__) << " @ " << (__FUNCTION__) << "\t"

#define HANDLE_CONNECTION_RESULT(status) HandleConnectionResult(status, __FILE__, __LINE__, __FUNCTION__)

#endif // XILENS_LOGGER_H
