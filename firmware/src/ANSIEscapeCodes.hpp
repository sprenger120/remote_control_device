#pragma once

class ANSIEscapeCodes
{
public:
    // https://en.wikipedia.org/wiki/ANSI_escape_code
    static constexpr const char *ClearTerminal = "\x1b[2J";
    static constexpr const char *MoveCursorToHome = "\x1b[H";
    static constexpr const char *ColorSection_WhiteText_BlackBackground = "\x1b[37;40m";
    static constexpr const char *ColorSection_WhiteText_RedBackground = "\x1b[37;41m";
    static constexpr const char *ColorSection_BlackText_GreenBackground = "\x1b[90;42m";
    static constexpr const char *ColorSection_BlackText_YellowBackground = "\x1b[90;43m";
    static constexpr const char *ColorSection_WhiteText_MagentaBackground = "\x1b[37;45m";
    static constexpr const char *ColorSection_WhiteText_GrayBackground = "\x1b[37;100m";
    static constexpr const char *ColorSection_End = "\x1b[m";
};