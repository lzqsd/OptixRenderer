#include "utils/ptxPath.h"
#include <iostream>

std::string ptxPath( const std::string& cuda_file )
{
    std::string file = std::string(sutil::samplesPTXDir()) +
        "/" + std::string(SAMPLE_NAME) + "_generated_" +
        cuda_file +
        ".ptx";
    return file;
}
