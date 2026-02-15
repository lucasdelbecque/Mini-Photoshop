# GPU Image Processing Engine

A high-performance, real-time image processing application built from scratch using **C++** and **Modern OpenGL (3.3+)**.

This project demonstrates the implementation of a programmable graphics pipeline to perform signal processing operations (convolutions, spatial filtering, color manipulation) directly on the GPU via custom GLSL Fragment Shaders. It features a responsive graphical user interface (GUI) based on **Dear ImGui** for dynamic parameter control.

## Project Overview

Unlike traditional CPU-based image editors that iterate over pixels sequentially, this engine leverages the parallel processing power of the GPU. Every pixel is processed simultaneously through a custom graphics pipeline, allowing for lag-free adjustments of complex filters like Gaussian Blur or Edge Detection, even on high-resolution images.

### Key Features

* **Real-Time GPU Filtering**: All image manipulations occur in the Fragment Shader.
    * **Gaussian Blur**: Dynamic convolution kernel with variable radius.
    * **Edge Detection**: Sobel-like kernel implementation for structural feature extraction.
    * **Mosaic/Pixelation**: UV coordinate manipulation for local spatial resolution reduction.
    * **Color Grading**: Weighted luminance grayscale conversion and negative inversion.
* **Non-Destructive Workflow**: Filters can be toggled and combined dynamically without altering the original source texture data.
* **Modern OpenGL Architecture**: Strict adherence to Core Profile (VAO, VBO, EBO) standards. No deprecated functions (`glBegin`/`glEnd`) were used.
* **High-DPI Support**: Native support for Retina/4K displays with correct viewport scaling and UI coordinate mapping.
* **Robust Texture Handling**: Implementation of `GL_CLAMP_TO_EDGE` wrapping and linear filtering to prevent convolution artifacts at image borders.

## Technical Stack

* **Language**: C++17
* **Graphics API**: OpenGL 3.3 Core Profile
* **Shader Language**: GLSL 330
* **Windowing system**: GLFW
* **GUI Library**: Dear ImGui
* **Image Loading**: stb_image
* **Build System**: CMake

## Architecture

The application follows a strict separation between the host application (CPU) and the rendering pipeline (GPU):

1.  **Initialization**: The generic `input.jpg` is loaded into VRAM as a 2D Texture.
2.  **Application Loop**:
    * **Input**: `GLFW` handles window events and inputs.
    * **GUI**: `Dear ImGui` renders the sidebar and captures filter parameters (intensity, radius, toggles).
    * **Uniforms**: The C++ host sends these parameters to the GPU via Uniform variables.
3.  **Rendering Pipeline**:
    * **Vertex Shader**: Passes geometry and texture coordinates.
    * **Fragment Shader**: Executes the convolution loops and color mixing logic per pixel.
4.  **Output**: The result is rendered to a full-screen quad.

## Prerequisites

* **C++ Compiler**: Clang (macOS), GCC (Linux), or MSVC (Windows).
* **CMake**: Version 3.10 or higher.
* **GLFW**: Must be installed on the system.

### Installing Dependencies (macOS)

    brew install glfw

## Build Instructions

1.  **Clone the repository**:
    
    git clone <repository_url>
    cd <repository_folder>

2.  **Prepare the build directory**:

    mkdir build
    cd build

3.  **Configure and Compile**:

    cmake ..
    make

4.  **Run the application**:
    Make sure an image named `input.jpg` is present in the executable's directory.

    ./MiniPhotoshop

## Usage

1.  **Launch**: The application opens with the default image loaded.
2.  **Sidebar Controls**: Use the inspector panel on the right to toggle filters.
3.  **Intensity Sliders**: Adjust the sliders to change the kernel size (for Blur) or the mixing factor (for other effects).
4.  **Combination**: Multiple filters can be active simultaneously. The shader applies them in a logical pass order (Mosaic -> Blur -> Color adjustments).

## Author

**Lucas Delbecque**

Developed as a technical demonstration of hardware acceleration for image processing and low-level graphics programming.
