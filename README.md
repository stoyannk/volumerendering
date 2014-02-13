*Volume rendering sample* - this is a sample for the **Voxels** library that shows realtime volume rendering of large meshes
The **Voxels** library is available for free at http://github.com/stoyannk/voxels

## Features
 - Voxel grid loading saving
 - Realtime scalable polygonization
 - Materials
 - Simple surface editing
 - Procedural surface creation

## Control

 - Arrows to move around
 - Left shift for faster movement
 - Left mouse buton - Modify grid based on current active modification
 - Right mouse button for freelook
 - F2 - Save voxel grid
 - S - Toggle solid draw
 - W - Toggle wireframe
 - T - Show/hide transition meshes
 - A - Draw/hide surface
 - L - Toggle dynamic LOD culling
 - U - Disable/enable updating LOD & culling (will freeze LOD so that you can look around)
 - R - Recalculate grid
 - + - Add material blend
 - - - Subtract material blend
 - Z - LOD level +1 (only if dynamic LOD & culling update is disabled)
 - X - LOD level -1 (only if dynamic LOD & culling update is disabled)
 - M - Circle modification types - modify surface or material
 - 0 - Set current material to 0
 - 1 - Set current material to 1 or "Add" to surface (depending on current mode)
 - 2 - Set current material to 2 or "SubtractAddInner" to surface (depending on current mode)
 - 3 - Set current material to 3 or "Subtract" from surface (depending on current mode)
 - 4 - Set current material to 4
 - 5 - Set current material to 5
 - F5-F8 - Change modification brush size
 
## Command line parameters

 - grid - load a specified voxel grid
 - gridsize - grid size in voxels
 - surface - seed the grid with a surface type
 - materials - materials definition file
 - heightmap - heightmap file
 - hscale - heightmap scale factor
 - xscale - grid scale factor on X coordinate
 - yscale - grid scale factor on Y coordinate
 - zscale - grid scale factor on Z coordinate
 - msaa - msaa samples

## Examples

"VolumeRendering --surface proc" - will create a perlin-noise generated surface of default size

"VolumeRendering --surface sphere --gridsize 128" - will create a 128x128x128 procedural voxel sphere

"VolumeRendering --surface plane --gridsize 128 --xscale 2 --yscale 2 --zscale 2" - will create a flat plane in a 128x128x128 grid scaled by a factor of 2 in all directions

"VolumeRendering --heightmap ..\media\testHmap2.png --gridsize 128 --xscale 4 --zscale 4" - will load a heightmap from a file, create a voxel grid from it and polygonize it, stretching it by a factor of 4 in the XZ plane

## Interesting classes

The purpose of this sample is to showcase the **Voxels** Library. The *VoxelPlane*, *VoxelBall*, *VoxelProc* and *VoxelBox* 
classes show procedural generation of grids. 

The *VoxelLodOctree* shows a class that dynamically culls surface blocks and computes the LOD levels to use.

The *MaterialTable* class shows how to implement the *Voxels::MaterialMap* interface and loads it's material definitions from a JSON file.

## System requirements
 - Windows Vista+
 - DirectX 11 level video card
 - Visual Studio 2012 toolset for building
 
## Building

The application is also available pre-built for Windows.

The sample depends on the  (http://github.com/stoyannk/dx11-framework), the **Voxels** Library (http://github.com/stoyannk/voxels) 
and Boost (http://boost.org) and makes heavy use of the DirectXMath library.

To build the sample checkout the dx11-framework, the **Voxels** Library and the sample in one root folder. The result is having them 
laid out like this:

 - {ROOT}\dx11-framework
 - {ROOT}\voxels
 - {ROOT}\volumerendering

Setup the dx11-framework by following it's instructions (you'll need to build Boost). 

**NOTE:** You must also modify the paths in *ApplicationPaths.props* and *Framework.props*.

If you follow this default directory structure you should now be able to build *volumerendering* via the VolumeRendering.sln solution.

The *MakeLinks.py* script can be run from an Administrator console with Python and will created symlinks to the *Voxels* library for use by the 
application on runtime. You should respect the above given directory structure for this to work.

## License

Code for the sample is licensed under the "New BSD License". The **Voxels** Library is free to use but with some limitations 
desribed in it's repo.

*Feedback welcome!*
