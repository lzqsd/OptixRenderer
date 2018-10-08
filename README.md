# optixRendererForSUNCG
An optix path tracer current designed for rendering SUNCG dataset

## Dependencies and compiling the code
The requirements are almost the same as [optix_advanced_samples](https://github.com/nvpro-samples/optix_advanced_samples). 
We also need opencv 2 for image processing. 
ccmake is needed to compile the code. Please check [INTALL-LINUX.txt](./INSTALL-LINUX.txt) and [INSTALL-WIN.txt](./INSTALL-WIN.txt) for details. After compiling, the executable file will be ./src/bin/optixSUNCG

## Running the code
To run the code, use the following command
```
./src/bin/optixSUNCG -f [shape_file] -o [output_file] -c [camera_file] -m [mode] --gpuIds [gpu_ids]
```
* `shape_file`: The xml file used to define the scene.  See [Shape file](https://github.com/lzqsd/optixRendererForSUNCG/edit/master/README.md#shape-file)
* `output_file`: The name of the rendered image. For ldr image, we support formats jpg, bmp, png. For hdr image, currently we support .rgbe image only. 
* `camera_file`: The file containing all the camera positions. We support rendering multiple images from different view points for a single scene. The format camera file will be 
  ```
  N           # Number of view points
  a_0 a_1 a_2 # The origin of the camera 1
  b_0 b_1 b_2 # The lookAt of the camera 1
  c_0 c_1 c_2 # The up vector of camera 1
  .....
  ```
  The camera position can be also defined in shape file (.xml). But if camera file is given, the camera position in the shape file will be neglected. 
* `mode`: The type of output of renderer
  - 0: Image (ldr or hdr)
  - 1: baseColor (.png)
  - 2: normal (.png)
  - 3: roughness (.png)
  - 4: mask (.png, If a ray from the pixel can hit an object, then the value of the pixel will be 255. If the ray hit an area light source or nothing, then the value will be 0)
  - 5: depth (.dat)
  - 6: metallic (.png)
* `gpuIds`: The ids of devices used for rendering. For example --gpuIds 0 1 2, then 3 gpus will be used for rendering. 

## Shape file
We use xml file to define the scene. The format will be very similar to mitsuba. The main elements of the shape file include integrator, sensor, material, shape and emitter. We will introduce how to define the 5 elements in details in the following. An example rendering file can be find in ./src/optixSUNCG/cbox. A typical shape file will look like:
```
<?xml version="1.0" encoding="utf-8"?>

<scene version="0.5.0">
  <integrator .../>
  ...
  <sensor .../>
  ...
  <bsdf .../>
  ...
  <emitter ../>
  ...
  <shape ../>
  ...
</scene>
```
### Integrator
Currently we only support path tracer. So the only acceptable way to define the integrator will be
```
<integrator type="path"/>
```

### Sensor
We support three types of sensors, the panorama camera (`envmap`), the hemisphere camera (`hemisphere`) and the perspective camera (`perspective`). The sub-elements belong to sensor include
* `fovAxis`: The axis of field of view. The value can be `x` or `y`.
* `fov`: Field of view, the measurement is degree.
* `transform`: The extrinsic parameter of the camera
* `sampler`: Currently we support two types of sampler. `independent` will actually do stratified sampling. `adaptive` sampling will render two images with different random seeds, scale the two images so that the mean value of pixel will be 0.5 and then compute the variance. If the variance is larger than a threshold, we will average the two images and render a new images by doubling the number of samples. We repeat this process until the variance is smaller than the threshold or the number of samples is too large. Adaptive sampling is highly recommended when rendering dataset.
* `film`: We currently support two types of film. `ldrfilm` will render ldr image with gamma correction (2.2). `hdrfilm` will render hdr image (.rgbe). The `height` and `width` of the images are set here. 

Following is an example of the xml file of a perspective sensor. 
```
<sensor type="perspective">
  <string name="fovAxis" value="x"/>
  <float name="fov" value="60.0"/>
  <transform name="toWorld">
    <lookAt origin="278, 273, -800" target="278, 273, -799" up="0, 1, 0"/>
  </transform>
  <sampler type="independent">
    <integer name="sampleCount" value="10000"/>
  </sampler>

  <film type="ldrfilm">
    <integer name="width" value="512"/>
    <integer name="height" value="512"/>
  </film>
</sensor>
```
### Material
The renderer currently supports three kinds of materials: `diffuse`, `phong` and `microfacet`
* `diffuse`: The parameters include `reflectance` and `normal`. The type of `reflectance` can be `rgb` or `texture`. The type of `normal` can only be `texture`. The normal map should be a bitmap. 
* `phong`: The parameters include `diffuseReflectance`, `specularReflectance`, `alpha` and `normal`. The type of `diffuseReflectance` and `specularReflectance` can be `rgb` or `texture`. The type of `alpha` can be `float` or `texture`. 
* `microfacet`: The parameters include `albedo`, `normal`, `roughness`, `metallic` and `fresnel`. The type of `albedo` can be `rgb` or `texture`. The type of `roughness`, `metallic` and `fresnel` can be `texture` and `float`. 

### Shape

### Emitter

## To be finished
* Add support of relative path. All paths of images and shapes have to be absolute path currently. This may cause inconveniences.
* Use OpenCV 3 to load and save hdr image. Installing OpenCV 3 might be non-trivial on Adobe Cluster, so I use OpenCV 2 instead, which does not support hdr image. 
* Add more materials, especially transparent materials.
* Bidirectional path tracing should be added, if we have time in the future. 
