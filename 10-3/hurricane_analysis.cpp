#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <regex>
#include <format>
#include <vector>
#include <stdexcept>
#include "ctre.hpp"

using std::getline;
using std::regex;
using std::regex_search;
using std::smatch;
using std::stod;
using std::stoi;
using std::string;
using std::vector;


constexpr int STARTYEAR = 1851;
constexpr int ENDYEAR = 2011;


constexpr double convertToSS(double knots) 
{
    if (knots >= 137) return 5.0;
    if (knots >= 113) return 4.0;
    if (knots >= 96) return 3.0;
    if (knots >= 83) return 2.0;
    if (knots >= 64) return 1.0;
    return 0.0;
}

double processRE(vector<double>& totals)
{

    auto start = std::chrono::steady_clock::now();

    std::ifstream file("hurdat_atlantic_1851-2011.txt");
    if (!file) 
    {
        throw std::runtime_error("Failed to open hurdat_atlantic_1851-2011.txt");
    }
    string line;

    /*
    REGEX USED:

    \b => Word boundary (start or end)
    \d{x} => Must be exactly x number of digits
    / => Literal / character
    \* => Literal asterisk
    (HR|TS) => matches HR or TS
    [A-Z0-9]* => Zero or more uppercase letters and/or digits
    \d+ => One or more digits
    \s+ => One or more whitespace characters
    \s* => Zero or more whitespace characters
    () => Capture group (in order from 1-n)
    */

    static const regex header(R"(\b\d{2}/\d{2}/(\d{4})\b)");
    static const regex body(R"(\b\d+\s+\d{2}/\d{2}\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*)");
    static const regex trailer(R"(\b(HR|TS)[A-Z0-9]*\b)");

    int currentYear = STARTYEAR;
    double currentTotal = 0.0;

    while (getline(file, line))
    {
        smatch m;

        if (regex_search(line, m, header))
        {
            int year = stoi(m[1].str());
            currentYear = year;

        } else if (regex_search(line, m, body))
        {
            double cum = 0.0;
            cum += convertToSS(stod(m[1].str()));
            cum += convertToSS(stod(m[2].str()));
            cum += convertToSS(stod(m[3].str()));
            cum += convertToSS(stod(m[4].str()));

            currentTotal += cum / 4.0;

        } else if (regex_search(line, m, trailer))
        {
            if (currentYear < STARTYEAR || currentYear > ENDYEAR) 
            {
                throw std::runtime_error("Parsed year out of range");
            }
            totals[currentYear - STARTYEAR] += currentTotal;
            currentTotal = 0.0;
        }
    }

    auto end = std::chrono::steady_clock::now();

    

    return std::chrono::duration<double, std::milli>(end - start).count();
}

double processCTRE(vector<double>& totals)
{

    auto start = std::chrono::steady_clock::now();

    std::ifstream file("hurdat_atlantic_1851-2011.txt");
    if (!file) 
    {
        throw std::runtime_error("Failed to open hurdat_atlantic_1851-2011.txt");
    }
    string line;

    int currentYear = STARTYEAR;
    double currentTotal = 0.0;

    while (getline(file, line))
    {
        if (auto m = ctre::search<R"(\b\d{2}/\d{2}/(\d{4})\b)">(line))
        {
            int year = stoi(string(m.get<1>().to_view()));
            currentYear = year;
        } else if (auto m = ctre::search<R"(\b\d+\s+\d{2}/\d{2}\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*\s*\d+\s+\d+\s+(\d+)\s+\d+\*)">(line))
        {
            double cum = 0.0;
            cum += convertToSS(stod(string(m.get<1>().to_view())));
            cum += convertToSS(stod(string(m.get<2>().to_view())));
            cum += convertToSS(stod(string(m.get<3>().to_view())));
            cum += convertToSS(stod(string(m.get<4>().to_view())));

            currentTotal += cum / 4.0;

        } else if (ctre::search<R"(\b(HR|TS)[A-Z0-9]*\b)">(line))
        {
            if (currentYear < STARTYEAR || currentYear > ENDYEAR) 
            {
                throw std::runtime_error("Parsed year out of range");
            }
            
            totals[currentYear - STARTYEAR] += currentTotal;
            currentTotal = 0.0;
        }
    }

    auto end = std::chrono::steady_clock::now();

    

    return std::chrono::duration<double, std::milli>(end - start).count();
}

void plot_annual_data(const vector<double>& data)
{
    std::cout << std::format("{:<10} {:>20}\n", "Year", "SaffirSimpsonTotal");
    std::cout << std::format("{:-<31}\n", "");

    for (size_t i = 0; i < data.size(); ++i) {
        int year = STARTYEAR + static_cast<int>(i);
        std::cout << std::format("{:<10} {:>20.1f}\n", year, data[i]);
    }


    std::cout << "-------------------------------------------------------------------------------";
}


int main()
{
    vector<double> yearToSaffirRE(ENDYEAR - STARTYEAR + 1);
    vector<double> yearToSaffirCTRE(ENDYEAR - STARTYEAR + 1);

    double RE_time = processRE(yearToSaffirRE);

    double CTRE_time = processCTRE(yearToSaffirCTRE);

    std::cout << "\nRegEx Implementation Time: " << RE_time << " ms\n" << std::endl;

    plot_annual_data(yearToSaffirRE);

    std::cout << "\nCTRE Implementation Time: " << CTRE_time << " ms\n" << std::endl;

    plot_annual_data(yearToSaffirCTRE);

    return 0;
}