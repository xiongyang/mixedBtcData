#include "logger.h"


src::severity_logger<logging::trivial::severity_level> lg;


void initLogger()
{
    logging::add_file_log(
                keywords::file_name = "btcorderLogger%N.log",
                keywords::rotation_size = 100 * 1024 * 1024,
              //  keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0)
                keywords::format = "[%TimeStamp%]: %Message%"
                );

    //TODO learn how the filter work in deep
    logging::core::get()->set_filter
    (
                logging::trivial::severity >= logging::trivial::trace
       //[]( logging::trivial::severity_level s){return  s>= logging::trivial::trace;}
    );
    logging::add_common_attributes();
}
