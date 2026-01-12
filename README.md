# r36s-gamebootstrap

`r36s-gamebootstrap` is a small starter project for cross-platform game development on **Windows** and **R36S**.  
It provides a simple **device abstraction** for context creation, frame timing & management, and input.
It includes a working demo that renders a simple animated triangle.

## Features

* **Cross-platform support**

  * Windows: Uses **GLFW** for window and context creation.
  * R36S: Uses **KMS/DRM** for display and **GBM** for buffer allocation and context surfaces.
* **OpenGL support via GLAD**
  * Windows: OpenGL **4.6**
  * R36S: OpenGL ES **3.2**

## Dependencies

* [GLFW](https://www.glfw.org/) (Windows only)
* [GLAD](https://glad.dav1d.de/) for OpenGL function loading
* KMS/DRM & GBM (R36S only)

## Build and Usage

### Using CMake

Clone the repository:

```bash
git clone --recursive https://github.com/Belfer/r36s-gamebootstrap.git
```

#### Visual Studio (Windows):

* The project includes a `CMakePresets.json` that detects the platform automatically and shows only relevant targets.
* You can select or add remote machines via Visual Studio settings for cross-platform development.
* Open the folder with `CMakeLists.txt` in Visual Studio, select the local/remote machine, select the preset for that machine, and build/run the demo.

#### Non-Visual Studio (Linux / R36S):

```bash
cd r36s-gamebootstrap
mkdir build
cd build
cmake ..
make
./r36s-gamebootstrap
```
