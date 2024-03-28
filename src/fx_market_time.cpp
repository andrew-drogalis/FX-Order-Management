// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_market_time.h"

#include <array>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <cmath>
#include <unistd.h> 

#include "fx_utilities.h"

namespace fxordermgmt {

FXMarketTime::FXMarketTime() { }

FXMarketTime::FXMarketTime(int update_frequency_seconds, FXUtilities fx_utilities) : update_frequency_seconds(update_frequency_seconds), fx_utilities(fx_utilities) { }

FXMarketTime::~FXMarketTime() { }

void FXMarketTime::forex_market_time_setup() {
    // 24 HR Start Trading Time & End Trading Time
    // In London Time 8 = 8:00 AM London Time
    int start_hr = 8;
    int end_hr = 20;
    // ----------------------------------------
    time_t now = time(0);
    struct tm *now_local = localtime(&now);
    std::string date = fx_utilities.get_todays_date();
    int day_of_week = now_local->tm_wday;

    std::chrono::zoned_time local_time = std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()};
    std::chrono::zoned_time london_time = std::chrono::zoned_time{"Europe/London", std::chrono::system_clock::now()};
    // --------
    int offset = (std::stoi(std::format("{:%z}", local_time)) - std::stoi(std::format("{:%z}", london_time))) / 100;
    int offset_seconds = -offset * 3600;
    start_hr += offset; 
    end_hr += offset;
    // ----------------------------------------
    // Holidays Must Be Updated as Required
    std::vector<std::string> holidays = {"2024_01_01","2024_01_15","2024_02_19","2024_03_29","2024_05_27","2024_06_19","2024_07_04","2024_09_02","2024_11_28","2024_11_29","2024_12_24","2024_12_25"};
    std::vector<int> days_of_week_to_trade = {6, 0, 1, 2, 3, 4};

    // Regular Market Hours
    if (std::find(days_of_week_to_trade.begin(), days_of_week_to_trade.end(), day_of_week) != days_of_week_to_trade.end() && std::find(holidays.begin(), holidays.end(), date) == holidays.end()) { 
        will_market_be_open_tomorrow = true;
        std::chrono::time_point time_now = std::chrono::system_clock::now();

        if ((std::stoi(std::format("{:%H}", local_time)) < end_hr || day_of_week == 4) && day_of_week != 6) {
            FX_market_start = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{start_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
            FX_market_end = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr-1} + std::chrono::minutes{58}).time_since_epoch()).count() * 60 + offset_seconds;
            market_close_time = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
        }
        else {
            time_now += std::chrono::days{1};
            FX_market_start = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{start_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
            FX_market_end = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr-1} + std::chrono::minutes{58}).time_since_epoch()).count() * 60 + offset_seconds;
            market_close_time = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
        }
    }
    // Market Closed
    else {
        will_market_be_open_tomorrow = false; 
        FX_market_start = 0;
        FX_market_end = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den - 1;
        market_close_time = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den - 1;
    }
}

bool FXMarketTime::market_closed() {
    unsigned int time_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
    if (market_close_time-1 < time_now && time_now < market_close_time+update_frequency_seconds+60) { return true; }
    else { return false; }    
}

bool FXMarketTime::forex_market_exit_only() {
    unsigned int time_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
    if (FX_market_start <= time_now && time_now <= FX_market_end) { return false; }
    else { return true; } 
}

bool FXMarketTime::pause_till_market_open() {
    bool market_is_closed = market_closed();
    bool forex_is_exit_only = forex_market_exit_only();

    if (!market_is_closed && forex_is_exit_only) {
        float time_to_wait_now = FX_market_start - (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
        // Outpuit Status Message
        std::cout << "Market Closed Today; Waiting for Tomorrow; Will be waiting for " << round(time_to_wait_now / 36) / 100 << " Hours..." << std::endl;
        // Pause Until Market Open
        if (time_to_wait_now > 0) { sleep(time_to_wait_now); }
        else { will_market_be_open_tomorrow = false; }   
    }  
    if (!will_market_be_open_tomorrow) {
        std::cout << "Market is Closed Today; Weekend Approaching; Terminating Program" << std::endl;
        return false;
    }
    return true;
}

}