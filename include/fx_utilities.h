#ifndef FX_UTILITIES_H
#define FX_UTILITIES_H

#include <string>

namespace std {

class FXUtilities {

    public:

        FXUtilities();

        ~FXUtilities();

        void setup_password_first_time(string account_type, string username);

        void init_logging(string working_directory);

        string get_todays_date();

};

}

#endif