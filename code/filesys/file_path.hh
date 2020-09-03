
#ifndef NACHOS_FILESYS_FILEPATH__HH
#define NACHOS_FILESYS_FILEPATH__HH

#include <list>
#include <string>

class FilePath{
public:
    // constructor
    FilePath(const char *);
    // change directory
    void chdir(const char *);
    // next directory in chain
    std::string next();
    //char* next();
    // conversion to char
    //operator char*() const;
    // conversion to string
    operator std::string () const;
    // chain length
    size_t size() const;
private:
    // directory chain
    std::list<std::string> path;
};

#endif