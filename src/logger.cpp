/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include "logger.h"


BOOST_LOG_GLOBAL_LOGGER_INIT(my_logger, SUSICAM_LOGGER) {
    SUSICAM_LOGGER lg;
    logging::core::get()->add_global_attribute("TimeStamp", logging::attributes::local_clock());

    logging::add_console_log
    (
            std::cout,
            boost::log::keywords::format =
            (
                    boost::log::expressions::stream
                            << "["
                            << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp","%Y-%m-%d %H:%M:%S.%f")
                            << "] ["
                            << logging::trivial::severity
                            << "] "
                            << boost::log::expressions::message // replace this with how you want your message to be formatted
            )
    );
    return lg;
}
