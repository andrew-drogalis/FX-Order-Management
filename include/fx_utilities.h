#ifndef FX_UTILITIES_H
#define FX_UTILITIES_H

#include <string>

namespace fxordermgmt {

class FXUtilities {

    public:

        FXUtilities();

        ~FXUtilities();

        void setup_password_first_time(std::string account_type, std::string username);

        void init_logging(std::string working_directory);

        std::string get_todays_date();

};

}

#endif