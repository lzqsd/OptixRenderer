#include "creator/createEnvmap.h"

cv::Mat loadEnvmap(Envmap env, unsigned width, unsigned height){
    
    int envWidth, envHeight;
    FILE* hdrRead = fopen(env.fileName.c_str(), "r");
    int rgbeInd = RGBE_ReadHeader( hdrRead, &envWidth, &envHeight, NULL);

    if( rgbeInd == -1 ){
        std::cout<<"Wrong: fail to load hdr image."<<std::endl;
        exit(0);
    }
    else{
        float* hdr = new float [envHeight * envWidth * 3];
        for(int n = 0; n < envHeight * envWidth * 3; n++)
            hdr[n] = 0;
        RGBE_ReadPixels_RLE(hdrRead, hdr, envWidth, envHeight );
        fclose(hdrRead );

        // Resize the  image
        cv::Mat envMat(envHeight, envWidth, CV_32FC3);
        for(int r = 0; r < envHeight; r++) {
            for(int c = 0; c < envWidth; c++) {
                int hdrInd = 3 *(r*envWidth + c );
                for(int ch = 0; ch < 3; ch++) {
                    float color = hdr[hdrInd + ch];
                    envMat.at<cv::Vec3f>(r, envWidth -1 - c)[2 - ch] = color * env.scale;
                } 
            }
        }
        cv::Mat envMatNew(height, width, CV_32FC3);
        cv::resize(envMat, envMatNew, cv::Size(width, height), cv::INTER_AREA); 
        
        delete [] hdr;

        return envMatNew;
    }
}


void createEnvmapBuffer(Context& context, 
        cv::Mat& envMat, cv::Mat& envMatBlured, 
        unsigned gridWidth, unsigned gridHeight )
{
    unsigned int width = envMat.cols;
    unsigned int height = envMat.rows;
    gridWidth = (gridWidth == 0) ? width : gridWidth;
    gridHeight = (gridHeight == 0) ? height : gridHeight;

    // Create tex sampler and populate with default values
    TextureSampler envmapSampler = createTextureSampler(context );
    loadImageToTextureSampler(context, envmapSampler, envMatBlured );
    context["envmap"] -> setTextureSampler(envmapSampler);
    
    TextureSampler envmapDirectSampler = createTextureSampler(context );
    loadImageToTextureSampler(context, envmapDirectSampler, envMat );
    context["envmapDirec"] -> setTextureSampler(envmapDirectSampler);
  
    Buffer envcdfV = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, 1, gridHeight);
    Buffer envcdfH = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, gridWidth, gridHeight);
    Buffer envpdf = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, gridWidth, gridHeight);
    context["envcdfV"] -> set(envcdfV);
    context["envcdfH"] -> set(envcdfH);
    context["envpdf"] -> set(envpdf);
}

void computeEnvmapDistribution(
            Context& context,
            cv::Mat envMat,
            unsigned width, unsigned height, 
            unsigned gridWidth, unsigned gridHeight
        )
{
    gridWidth = (gridWidth == 0) ? width : gridWidth;
    gridHeight = (gridHeight == 0) ? height : gridHeight;

    // Compute the weight map
    float* envWeight = new float[gridWidth * gridHeight];
    int winH = int(height / gridHeight);
    int winW = int(width  / gridWidth);
    for(int hid = 0; hid < gridHeight; hid++){
        float hidf = float(hid + 0.5) / float(gridHeight);
        float theta = hidf * PI;
        for(int wid = 0; wid < gridWidth; wid++){
            int hs = hid * winH;
            int he = (hid == gridHeight-1) ? height : (hid + 1) * winH;
            int ws = wid * winW;
            int we = (wid == gridWidth-1) ? width : (wid + 1) * winW;
            
            float N = (he - hs) * (we - ws);
            float W = 0;
            for(int i = hs; i < he; i++){
                for(int j = ws; j < we; j++){
                    for(int ch = 0; ch < 3; ch++)
                        W += envMat.at<cv::Vec3f>(height - 1 - i, j)[ch] / 3.0;
                }
            }
            envWeight[hid*gridWidth + wid] = W / N * sinf(theta) ;
        }
    }


    // Compute the cdf of envmap 
    optix::Buffer envcdfV = context["envcdfV"] -> getBuffer();
    float* envcdfV_p = reinterpret_cast<float*>(envcdfV -> map() );
    optix::Buffer envcdfH = context["envcdfH"] -> getBuffer();
    float* envcdfH_p = reinterpret_cast<float*>(envcdfH -> map() );  
    for(int vId = 0; vId < gridHeight; vId++){
        int offset = vId * gridWidth;
        envcdfH_p[offset] = envWeight[offset];
        for(int uId = 1; uId < gridWidth; uId++){
            envcdfH_p[offset + uId] = 
            envcdfH_p[offset + uId-1] + envWeight[offset + uId];
        }
        float rowSum = envcdfH_p[offset + gridWidth-1];
        if(vId == 0)
            envcdfV_p[vId] = rowSum;
        else
            envcdfV_p[vId] = envcdfV_p[vId-1] + rowSum;
        for(int uId = 0; uId < gridWidth; uId++){
            float S = (rowSum > 1e-6) ? rowSum : 1e-6;
            envcdfH_p[offset + uId] /= S;
        }
    }
    float colSum = envcdfV_p[gridHeight-1];
    for(int vId = 0; vId < gridHeight; vId++){
        float S = (colSum > 1e-6) ? colSum : 1e-6;
        envcdfV_p[vId] /= colSum;
    }

    // Compute the pdf for sampling
    int gridSize = gridHeight * gridWidth;
    for(int vId = 0; vId < gridHeight; vId++){
        float vIdf = float(vId + 0.5) / float(gridHeight);
        float sinTheta = sinf(vIdf * PI);
        for(int uId = 0; uId < gridWidth; uId++){
            envWeight[vId * gridWidth+ uId] /= 
                ((colSum * 2 * PI * PI * sinTheta) / gridSize );
        }
    } 

    optix::Buffer envpdf = context["envpdf"] -> getBuffer(); 
    float* envpdf_p = static_cast<float*> (envpdf -> map() );
    for(unsigned int i = 0; i < gridHeight; i++){
        for(unsigned int j = 0; j < gridWidth; j++){            
            int envId = i * gridWidth + j;
            envpdf_p[envId] = envWeight[envId];
        }
    }
 
    envpdf -> unmap();
    envcdfV -> unmap();
    envcdfH -> unmap();

    delete [] envWeight;
}


void createEnvmap(
        Context& context,
        std::vector<Envmap>& envmaps, 
        unsigned width, unsigned height, 
        unsigned gridWidth, unsigned gridHeight
        )
{
    if(gridWidth == 0 || gridHeight == 0){
        gridWidth = width;
        gridHeight = height;
    }

    if(envmaps.size() == 0){
        context["isEnvmap"] -> setInt(0);
        // Create the texture sampler 
        cv::Mat emptyMat = cv::Mat::zeros(1, 1, CV_32FC3);
        createEnvmapBuffer(context, emptyMat, emptyMat, 1, 1);
    }
    else{
        // Load the environmental map
        Envmap env = envmaps[0];
        cv::Mat envMat = loadEnvmap(env, width, height);
        
        unsigned kernelSize = std::max(5, int(height / 100) );
        if(kernelSize % 2 == 0) kernelSize += 1;
        cv::Mat envMatBlured(height, width, CV_32FC3);
        cv::GaussianBlur(envMat, envMatBlured, cv::Size(kernelSize, kernelSize), 0, 0); 

        context["isEnvmap"] -> setInt(1); 
        createEnvmapBuffer(context, envMat, envMatBlured, gridWidth, gridHeight);

        computeEnvmapDistribution(context, envMatBlured, 
                width, height, gridWidth, gridHeight);
    }
}

void rotateUpdateEnvmap(Context& context, Envmap& env, float phiDelta, 
        unsigned width, unsigned height,
        unsigned gridWidth, unsigned gridHeight
        )
{
    cv::Mat envMat = loadEnvmap(env, width, height);
    cv::Mat envMatNew(height, width, CV_32FC3);
    
    // Rotate the envmap  
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            float phi = (2 * float(c) / width - 1) * PI;
            float phiNew = phi + phiDelta;
            while(phiNew >= PI) phiNew -= 2 * PI;
            while(phiNew < -PI) phiNew += 2 * PI;

            float cNew = (float(width) -1) * (phiNew + PI) / (2 * PI);
            int c1 = int(ceil(cNew ) );
            int c2 = int(floor(cNew ) );
            float w1 = cNew - c2;
            float w2 = (c1 == c2) ? 0 : c1 - cNew;
            
            for(int ch = 0; ch < 3; ch++){
                envMatNew.at<cv::Vec3f>(r, c)[ch] = 
                    w1 * envMat.at<cv::Vec3f>(r, c1)[ch]
                    + w2 * envMat.at<cv::Vec3f>(r, c2)[ch];
            }
        }
    }
    
    unsigned kernelSize = std::max(3, int(height / 200) );
    if(kernelSize % 2 == 0) kernelSize += 1;
    cv::Mat envMatNewBlured(height, width, CV_32FC3);
    cv::GaussianBlur(envMatNew, envMatNewBlured, cv::Size(kernelSize, kernelSize), 0, 0); 

    // Update the texture 
    TextureSampler Sampler = context["envmap"] -> getTextureSampler();
    updateImageToTextureSampler(Sampler, envMatNewBlured);
    computeEnvmapDistribution(context, envMatNewBlured, width, height, gridWidth, gridHeight); 
     
    TextureSampler direcSampler = context["envmapDirect"] -> getTextureSampler();
    updateImageToTextureSampler(direcSampler, envMatNew);
}
