# CUDA BSSRDF Raytracer

This project researches ways to do subsurface scattering for point-cloud models on the GPU in real-time.
## Demo
Please see the Demo/ folder for a video demonstration of the public version's capabilities

## Dependencies
1. SDL2 (not included please use nuget through Visual Studio to include SDL2)
2. Happly (header included)
3. JSONcpp (included)
4. CUDA (not included)

## Books Used
1. Physically Based Rendering
2. Fundamentals of Computer Graphics
3. Computer Graphics: Principles and Practice

## Papers Implemented
1. A Reflectance Model For Computer Graphics [Cook92](https://www.researchgate.net/profile/Robert_Cook10/publication/220184024_A_Reflectance_Model_for_Computer_Graphics/links/56e62d5708ae98445c21739b/A-Reflectance-Model-for-Computer-Graphics.pdf)
2. Microfacet Models For Refraction Through Rough Surfaces [Walter07](https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html)
3. The Rendering Equation [Kajiya86](http://www.cse.chalmers.se/edu/year/2011/course/TDA361/2007/rend_eq.pdf)
4. Stackless KD-Tree Traversal For High-Performance GPU Ray-Tracing [Popov07](http://www-sop.inria.fr/members/Stefan.Popov/media/StacklessKDTreeTraversal_EG07.pdf)
5. Realtime Ray-Tracing and Interactive Global Illumination [Wald04](https://www.researchgate.net/profile/Ingo_Wald/publication/4169853_Interactive_ray_tracing_of_point-based_models/links/02e7e51fc1fb6be599000000/Interactive-ray-tracing-of-point-based-models.pdf)
6. Understanding The Efficiency of KD-Tree Ray-Traversal Techniques over a GPGPU Architecture [Santos11](https://www.cin.ufpe.br/~als3/saap/ArturLiraDosSantos-ArtigoIJPP.pdf)
7. Robust BVH Ray Traversal [Ize13](http://jcgt.org/published/0002/02/02/paper.pdf)
8. A Practical Model For Subsurface Light Transport [Jensen01](https://graphics.stanford.edu/papers/bssrdf/)
9. Multiple Scattering Microfacet BSDFs With The Smith Model [Heitz16](https://jo.dreggn.org/home/2015_microfacets.pdf)
10. [Interactive Ray Tracing of Point-based Models](http://www.sci.utah.edu/~wald/Publications/2005/pointbased/pointbased.pdf)
11. [Raytracing Point Set Surfaces](http://hyperfun.org/FHF_Log/29_adamsona_ray.pdf)
12. [Approximating and Intersecting Surfaces From Points](https://www.cs.jhu.edu/~misha/Fall13b/Papers/Adamson03.pdf)
## Papers In Progress
Public Version is mostly finalized

## Bugs
1. Some portability bugs. May not build correctly on machines with an older card than GeForce GTX 1080, or newer than RTX 2070