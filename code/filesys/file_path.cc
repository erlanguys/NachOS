#include <list>
#include <algorithm>
#include <sstream>
#include "file_path.hh"

/*
FilePath::operator char*() const {
    std::string buffer;
    for(auto &dir : path) buffer += "/" + dir;
    return std::strcpy(new char[buffer.length() + 1], buffer.c_str());
}*/

FilePath::operator std::string() const {
    std::string buffer;
    for(auto &dir : path) buffer += "/" + dir;
    return buffer;
}

FilePath::FilePath(const char *filepath_c) {
    chdir(filepath_c);
}

void FilePath::chdir(const char *filepath_c){
    std::stringstream filepath {filepath_c};
    std::string directory;
    if( filepath_c[0] == '/' ) path.clear();
    while( std::getline(filepath, directory, '/'))
        if( directory == "." or directory.empty() ) continue;
        else if( directory == "..")
            if( path.empty() ) continue;
            else path.pop_back();
        else path.emplace_back(directory);
}

size_t FilePath::size() const {
    return path.size();
}

std::string FilePath::next(){
    if( path.empty() ) return "";
    auto dir = path.front(); path.pop_front();
    return dir;
}

std::string FilePath::last(){
    if( path.empty() ) return "";
    auto dir = path.back(); path.pop_back();
    return dir;
}

/*
char* FilePath::next() {
    if( path.empty() ) return nullptr;
    auto *dir = std::strcpy(new char[path.front().length() + 1], path.front().c_str());
    path.pop_front();
    return dir;
}*/
