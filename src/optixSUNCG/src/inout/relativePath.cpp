#include "inout/relativePath.h"

void splitPath(std::vector<std::string>& strs, std::string inputPath ){

    strs.clear();
    std::size_t sp = 0, ep = 0;
    std::string spliter = std::string("/");

    while(sp < inputPath.size() && inputPath[sp] == '/') sp++;
    ep = inputPath.find(spliter, sp);
    while(ep != std::string::npos){
        strs.push_back(inputPath.substr(sp, ep - sp) );

        sp = ep + 1;
        while(sp < inputPath.size() && inputPath[sp] == '/' ) sp++;
        ep = inputPath.find(spliter, sp);
    }
    if (sp < inputPath.size() )
        strs.push_back(inputPath.substr(sp ) );
}


std::string relativePath(std::string root, std::string inputPath){

    if(inputPath[0] == '/'){
        return inputPath;
    }
    else{
        
        std::vector<std::string > rootParts, inputParts;
        splitPath(rootParts, root );
        splitPath(inputParts, inputPath );
        
        assert(rootParts.size() != 0);
        std::size_t ep = rootParts.size() - 1;
        std::size_t sp = 0;
        for(std::size_t i = 0; i < inputParts.size(); i++){
            if(inputParts[i] == std::string("..") ){
                sp = sp + 1;
                ep = ep - 1;
            }
            else if(inputParts[i] == std::string(".") ){
                sp = sp + 1;
                break;
            }
            else{
                break;
            }
        }

        if(ep <= 0){
            std::cout<<"Wrong: error in computing relative path, too  any .."<<std::endl;
            assert(false );
        }
        
        std::string filePath = "/";

        for(int i = 0; i < ep; i++){
            filePath += rootParts[i];
            filePath += "/";
        }
        
        for(int i = sp; i < inputParts.size(); i++){
            filePath += inputParts[i];
            if(i != inputParts.size()-1 )
                filePath += "/";
        }

        return filePath;
    }    
}
