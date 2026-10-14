# Third-party software

Project licensing does not replace dependency licenses.

| Dependency | Version | License | Distribution |
| --- | --- | --- | --- |
| [GLFW](https://github.com/glfw/glfw/tree/3.4) | 3.4 | [zlib/libpng](https://github.com/glfw/glfw/blob/3.4/LICENSE.md) | CMake FetchContent |
| [Dear ImGui](https://github.com/ocornut/imgui/tree/v1.91.5) | 1.91.5 | [MIT](https://github.com/ocornut/imgui/blob/v1.91.5/LICENSE.txt) | CMake FetchContent |
| [doctest](https://github.com/doctest/doctest/tree/v2.4.11) | 2.4.11 | [MIT](https://github.com/doctest/doctest/blob/v2.4.11/LICENSE.txt) | CMake FetchContent, tests |
| stb_image_write | Bundled header | Public domain or MIT, at your option | `third_party/stb_image_write.h`; complete notice at end of file |

Optional ffmpeg video export invokes a separately installed executable; ffmpeg is not bundled. Python verification/plotting tools use separately installed Pillow, NumPy, and Matplotlib.
