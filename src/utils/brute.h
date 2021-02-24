#ifndef FEATHER_BRUTE_H
#define FEATHER_BRUTE_H

#include <string>

class brute {
public:
    explicit brute(const std::string &chars);

    std::string next();

    std::string chars;
    std::string current;
    int len;
    int count = 0;
};

class bruteword {
public:
    enum strategy {
        ReplaceSingle = 0,
        AppendSingle,
        PrependSingle,
        SwitchLetters,
        DeleteSingle,
        AddSingle,
        END
    };

    explicit bruteword(const std::string &chars);
    void nextStrategy();
    void setWord(const std::string &word);

    std::string next();

private:
    void resetIndex();

    std::string m_chars;
    std::string m_word;
    strategy m_currentStrategy;

    int iword = 0;
    int ichar = 0;
};

#endif //FEATHER_BRUTE_H
