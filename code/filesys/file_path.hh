
#ifndef NACHOS_FILESYS_FILEPATH__HH
#define NACHOS_FILESYS_FILEPATH__HH

#include <list>
#include <cstring>

class FilePath{
public:
    // constructor
    FilePath(const char *);
    // change directory
    void chdir(const char *);
    // next directory in chain
    char* next();
    // conversion to char
    operator char*() const;
private:
    // directory chain
    std::list<std::string> path;
};

#endif