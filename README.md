# OptixRenderer
An optix GPU based path tracer. This is the renderer built and used in the following 2 projects:
* Li, Z., Shafiei, M., Ramamoorthi, R., Sunkavalli, K., & Chandraker, M. (2020). Inverse rendering for complex indoor scenes: Shape, spatially-varying lighting and svbrdf from a single image. In Proceedings of the IEEE/CVF Conference on Computer Vision and Pattern Recognition (pp. 2475-2484).
* Li, Z.\*, Yeh, Y. Y.\*, & Chandraker, M. (2020). Through the Looking Glass: Neural 3D Reconstruction of Transparent Shapes. In Proceedings of the IEEE/CVF Conference on Computer Vision and Pattern Recognition (pp. 1262-1271).

Please contact [Zhengqin Li](https://sites.google.com/a/eng.ucsd.edu/zhengqinli) about any questions with regard to the renderer. Please consider citing the 2 papers if you find the renderer useful in your own project. 

## Dependencies and compiling the code
The requirements are almost the same as [optix_advanced_samples](https://github.com/nvpro-samples/optix_advanced_samples). 
We also need opencv 2 for image processing. 
ccmake is needed to compile the code. Please check [INTALL-LINUX.txt](./INSTALL-LINUX.txt) and [INSTALL-WIN.txt](./INSTALL-WIN.txt) for details. After compiling, the executable file will be ./src/bin/optixRenderer

## Running the code
To run the code, use the following command
```
./src/bin/optixRenderer -f [shape_file] -o [output_file] -c [camera_file] -m [mode] --gpuIds [gpu_ids]
```
* `shape_file`: The xml file used to define the scene.  See [Shape file](https://github.com/lzqsd/optixRenderer/edit/master/README.md#shape-file)
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

# Other flags
There are some other flags to control the program. 
* `--noiseLimit`: If the variance of image is larger than this threshold in the first iteration of adaptive samplilng. This image will be abandoned directly. 
* `--vertexLimit`: If the .obj or .ply file contains more vertices than the threshold, it will be abandoned.
* `--intensityLimit`: If the intensity of the rendered image is smaller than the threshold, it will be abandoned.
* `--camStart`: The start camera Id in the camera file.
* `--camEnd`: The end camera Id in the camera file. 
* `--forceOutput`: Overwrite the existing file. 
* `--rrBeginLength`: Start Roulette Roulette for light path with length longer than this threshold.  
* `--maxPathLength`: The maximum length of the light path. 


## Shape file
We use xml file to define the scene. The format will be very similar to mitsuba. The main elements of the shape file include integrator, sensor, material, shape and emitter. We will introduce how to define the 5 elements in details in the following. An example rendering file can be find in ./src/optixRenderer/cbox. A typical shape file will look like:
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

Following is an example of the xml file of a perspective sensor. The xml for `envmap` and `hemisphere` camera should be almost the same, except that `fov` and `fovAxis` is no longer useful. 
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
* `dielectric`: The parameters include `specularReflectance`, `specularTransmittance`, `normal`, `intIOR` and `extIOR`. The type of `specularReflectance` and `specularTransmittance` can only be `rgb`. The type of  `intIOR` and `extIOR` can only be `float`. 
* `conductor`: The parameters include `specularReflectance`. The type of `specularReflectance` can only be `rgb`.

Following is an example of `phong` material. Notice that the path to the texture should be absolute path. 
```
<bsdf id="Model#317-1_material_28" type="phong">
  <texture name="diffuseReflectance" type="bitmap">
    <string name="filename" value="/home/exx/Zhengqin/SceneMaterial/Dataset/texture/wood_5.jpg"/>
  </texture>
  <rgb name="specularReflectance" value="0.43922 0.43922 0.43922"/>
  <float name="alpha" value="40.00000"/>
</bsdf>
```
### Shape
Currently we support `.obj` file and `.ply` file formats. `.mtl` file is not supported so the material can only be defined in `.xml` file. However, we support using `usemtl ...` in `.obj` file so that you can have different materials for a single `.obj` file. Following is an example of a shape with two different materials. In `.obj` file, the material is defined using 
```
...
usemtl Model#317-1_material_28
...
usemtl Model#317-1_material_29
...
```
The corresponding `.xml` file should be
```
<shape id="Model#317-1_object" type="obj">
  <string name="filename" value="/home/exx/Zhengqin/SceneMaterial/Dataset/house_obj_render/train/Shape__0004d52d1aeeb8ae6de39d6bd993e992/Model#317-1_object.obj"/>
  <bsdf id="Model#317-1_material_27" type="phong">
    <rgb name="diffuseReflectance" value="0.27843 0.27843 0.27843"/>
    <rgb name="specularReflectance" value="1.00000 1.00000 1.00000"/>
    <float name="alpha" value="300.00000"/>
  </bsdf>
  <bsdf id="Model#317-1_material_28" type="phong">
    <texture name="diffuseReflectance" type="bitmap">
      <string name="filename" value="/home/exx/Zhengqin/SceneMaterial/Dataset/texture/wood_5.jpg"/>
    </texture>
    <rgb name="specularReflectance" value="0.43922 0.43922 0.43922"/>
    <float name="alpha" value="40.00000"/>
  </bsdf>
</shape>
```
Notice that the `id` of brdf should be consistent with the material defined by `usemtl` commmand. We also support scale, translate and rotate an object. The format is exactly the same as mitsuba. 

### Emitter
We support three kinds of emitter, area light (`area`), point light (`point`), flash light (`flash`) and environment map (`envmap`). 
* `area`: The area light can be considered as an sub-elements of shape. It has one parameter `radiance`. The type of `radiance` is `rgb`
* `point` and `flash`: The `point` light has two parameters. The `radiance` controls the light intensity while `position` controls its position in the scene. The type of `position` is `point`. The `flash` light only has parameter `radiance`. The `position` will be the same as origin of camera. That will be useful when rendering with camera file. 
* `envmap`: The image used for environmental map must be of format `.hdr`.  The `scale` parameter can be used to control the intensity of the environmental map. 

Followings are examples of `area` light and `envmap` respectively. 
```
<shape type="obj">
  <string name="filename" value="/home/exx/Zhengqin/SceneMaterial/Code/optix_advanced_samples/src/optixSUNCG/cbox/meshes/cbox_luminaire.obj"/>
  <emitter type="area">
    <rgb name="radiance" value="20.0 20.0 20.0"/>
  </emitter>
</shape>

<emitter type = "envmap">
  <string name="filename" value="/home/exx/Zhengqin/SceneMaterial/Code/optix_advanced_samples/src/optixSUNCG/cbox/envmap.hdr"/>
</emitter>
```
## Examples
There are three `.xml` files in `src/optixRenderer/cbox` directory, which can be directly used for testing.  

## To be finished
* Bidirectional path tracing should be added, if we have time in the future. 
