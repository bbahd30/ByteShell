#include <iostream>
#include <string>
#include <map>
using namespace std;

string formatText(string formatAttribute, const string &foregroundColor, const string &text)
{
    const string FORMAT_START = "\033[";
    const string FORMAT_END = "m";
    const string RESET_FORMAT = "\033[0m";

    map<string, string> colorCodes = {
        {"black", "30"},
        {"red", "31"},
        {"green", "32"},
        {"yellow", "33"},
        {"blue", "34"},
        {"magenta", "35"},
        {"cyan", "36"},
        {"white", "37"}};

    map<string, string> formatCodes = {
        {"reset", "0"},
        {"bold", "1"},
        {"underline", "4"},
        {"inverse", "7"},
        {"bold_off", "21"},
        {"underline_off", "24"},
        {"inverse_off", "27"}};

    string formattedText = "";

    formattedText += FORMAT_START;
    formattedText += formatCodes[formatAttribute];
    formattedText += ";";
    formattedText += colorCodes[foregroundColor];
    formattedText += ";";
    formattedText += "49";
    formattedText += FORMAT_END;
    formattedText += text;
    formattedText += RESET_FORMAT;

    return formattedText;
}
