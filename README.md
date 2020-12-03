# CUDA BSSRDF Raytracer

This project researches ways to do subsurface scattering for point-cloud models on the GPU in real-time.

## Dependencies
1. SDL
2. Happly (header included)
3. JSONcpp
4. CUDA

## Books Used
1. Physically Based Rendering
2. Fundamentals of Computer Graphics
3. Computer Graphics: Principles and Practice

## Papers Implemented
1. A Reflectance Model For Computer Graphics [Cook92]
2. Microfacet Models For Refraction Through Rough Surfaces [Walter07]
3. The Rendering Equation [Kajiya86]
4. Stackless KD-Tree Traversal For High-Performance GPU Ray-Tracing [Popov07]
5. Realtime Ray-Tracing and Interactive Global Illumination [Wald04]
6. Understanding The Efficiency of KD-Tree Ray-Traversal Techniques over a GPGPU Architecture [Santos11]
7. Robust BVH Ray Traversal [Ize13]
8. A Practical Model For Subsurface Light Transport [Jensen01]
9. Multiple Scattering Microfacet BSDFs With The Smith Model [Heitz16]

## Papers In Progress
1. [Interactive Ray Tracing of Point-based Models](http://www.sci.utah.edu/~wald/Publications/2005/pointbased/pointbased.pdf)
2. [Raytracing Point Set Surfaces](http://hyperfun.org/FHF_Log/29_adamsona_ray.pdf)
3. [Approximating and Intersecting Surfaces From Points](https://www.cs.jhu.edu/~misha/Fall13b/Papers/Adamson03.pdf)

## Bugs
1. Manually modified imgui_impl_opengl3.h to remove glew loader define
2. KDTree has several bugs
3. Microfacet BSDF (Smith) currently non-functional
4. Dipole not interfacing with UI
6. UI needs to be split into render func