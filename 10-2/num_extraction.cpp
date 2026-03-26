#include <iostream>
#include <string>
#include <regex>
#include <string_view>
#include <chrono>
#include "ctre.hpp"


double extractAndPrintCTRE(std::string_view line)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    bool foundAny = false;

    // ctre::search_all lazily iterates all non-overlapping matches in 'line'.
    // The pattern is validated at compile-time. Named capture groups <left> and <right>
    // capture the integer digits before and after the decimal point respectively.
    for (auto match : ctre::search_all<R"((?<left>\d+)\.(?<right>\d+))">(line))
    {
        foundAny = true;
        std::cout << match.get<"left">() << " is before the decimal and " << match.get<"right">() << " is after the decimal" << std::endl;
    }

    if (!foundAny) std::cout << "\nNo decimal numbers found in your input." << std::endl;



    auto endTime = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
}

double extractAndPrintRE(std::string_view line)
{
    
    auto startTime = std::chrono::high_resolution_clock::now();

    std::regex nums(R"((\d+)\.(\d+))");

    // match_result stores results from regex_search, regex_match, etc.
    // Iterates via const iterator over non-owning string_view
    std::match_results<std::string_view::const_iterator> match; 
    
    auto start = line.begin();
    bool foundAny = false;

    while (std::regex_search(start, line.end(), match, nums))
    {
        foundAny = true;
        std::cout << match[1] << " is before the decimal and " << match[2] << " is after the decimal" << std::endl;
        start = match[0].second; // move start to one character past the matched instance
    }

    if (!foundAny) std::cout << "\nNo decimal numbers found in your input." << std::endl;

    auto endTime = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
}





int main() 
{

    while (true)
    {
        std::cout << "\nEnter the text you would like to extract the numbers with decimal points from (or 'q' to quit the program): " << std::endl;
        std::string input;
        std::getline(std::cin, input);  // Will only allow one line to be entered at a time!

        if (input == "q") break;
        
        std::cout << "\n\nRegular Expression Extraction: " << std::endl;
        double time = extractAndPrintRE(input);

        std::cout << "\nRegEx Extraction Timing: " << time << " ns" << std::endl;

        std::cout << "\n\nCTRE Extraction: " << std::endl;
        double timeCTRE = extractAndPrintCTRE(input);

        std::cout << "\nCTRE Extraction Timing: " << timeCTRE << " ns" << std::endl;
    }
    std::cout << "\nGoodbye!" << std::endl;
}