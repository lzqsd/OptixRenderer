#ifndef RELATIVEPATH_HEADER
#define RELATIVEPATH_HEADER

#include <assert.h>
#include <vector>
#include <string>
#include <iostream>

void splitPath(std::vector<std::string>& strs, std::string inputPath );


std::string relativePath(std::string root, std::string inputPath);

#endif
