# optixRendererForSUNCG
An optix path tracer current designed for rendering SUNCG dataset

## Dependencies and compiling the code
The requirements are almost the same as [optix_advanced_samples](https://github.com/nvpro-samples/optix_advanced_samples). 
We also need opencv 2 for image processing. 
ccmake is needed to compile the code. Please check [INTALL-LINUX.txt](./INSTALL-LINUX.txt) and [INSTALL-WIN.txt](./INSTALL-WIN.txt) for details. After compiling, the executable file will be ./src/bin/optixSUNCG

## Running the code
To run the code you can use the following command
```
./src/bin/optixSUNCG -f [shape_file] -o [output_file] -c [camera_file] -m [mode] --gpuIds [gpu_ids]
```
* `shape_file`: The xml file used to define the scene.  See [Shape file](https://github.com/lzqsd/optixRendererForSUNCG/edit/master/README.md#shape-file)
* `output_file`: The name of the rendered image. For ldr image, we support formats jpg, bmp, png. For hdr image, currently we support .rgbe image only. 
* `camera_file`: The file containing all the camera positions. We support rendering multiple images for a single scene. The format camera file will be 
  ```
  N           # Number of view points
  a_0 a_1 a_2 # The origin of the camera 1
  b_0 b_1 b_2 # The lookAt position of the camera 1
  c_0 c_1 c_2 # The up vector of camera 1
  .....
  ```
* `mode`: 
* `gpuIds`: 

## Shape file


## To be finished
* Add support of relative path
* Use opencv3 to load and save hdr image. 
* Add more materials, especially transparent materials.
* Bidirectional path tracing should be added. 
