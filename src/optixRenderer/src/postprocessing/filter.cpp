#include "postprocessing/filter.h"

void medianFilter(float* img, int width, int height, int hwsize){

    float* newImg = new float[width * height * 3];

    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            int minR = std::max(0, r - hwsize);
            int maxR = std::min(height-1, r+hwsize);
            int minC = std::max(0, c - hwsize);
            int maxC = std::min(width-1, c+hwsize);
            
            for(int ch = 0; ch < 3; ch++){
                std::vector<float> wArr( (maxR-minR+1) * (maxC-minC+1) );
                int cnt = 0;
                for(int rr = minR; rr <= maxR; rr++){
                    for(int cc = minC; cc <= maxC; cc++){
                        wArr[cnt++] = img[3*(rr*width + cc) + ch];
                    }
                }
                std::sort(wArr.begin(), wArr.end() );
                int middleId = int(round(cnt * 0.5) );

                bool isChange = false;
                float origValue = img[3*(r * width + c) + ch];
                if(origValue == wArr[0] ){
                    if( (wArr[2] / std::max(origValue, float(1e-6) ) ) > 2){
                        isChange = true;
                    }
                }
                
                if(origValue == wArr[cnt-1] ){
                    if( origValue / std::max(wArr[cnt-3], float(1e-6) ) > 2){
                        isChange = true;
                    }
                }
                
                if(isChange )
                    newImg[3*(r*width + c) + ch] = wArr[middleId ];
                else
                    newImg[3*(r*width + c) + ch] = origValue;
            }
        }
    }   

    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            for(int ch = 0; ch < 3; ch++){
                img[3 * (r*width + c) + ch] = newImg[3 * (r*width + c) + ch];
            }
        }
    }

    delete [] newImg;
}
