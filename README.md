\# NotePaint (记事本 + 画图)



一个使用 \*\*C++ / Qt6\*\* 开发的 Windows 桌面应用程序，将经典的“文本编辑器”与“画图工具”完美结合在同一个多标签页窗口中。本项目是从两个 Python (Tkinter) 脚本重构并合并而来的。



\## ✨ 功能特性



\### 📝 记事本模块

\- 多标签页支持，与画图模块无缝切换。

\- \*\*自定义保存对话框\*\*：支持选择保存类型（TXT/PY/HTML 等）和编码格式（UTF-8, UTF-8 BOM, GBK, GB18030 等）。

\- \*\*智能编码识别\*\*：打开文件时自动尝试 UTF-8 / GBK 等编码，防止中文乱码。

\- 支持快捷键：`Ctrl+O` (打开), `Ctrl+S` (保存), `Ctrl+Z` (撤销), `Ctrl+Y` (重做)。



\### 🎨 画图模块

\- 丰富的工具：铅笔、直线、矩形、椭圆、橡皮擦、文字输入。

\- 支持调整画笔粗细、前景色与填充色。

\- 按住 `Shift` 键可画直线、正方形、正圆。

\- 完整的撤销 (`Ctrl+Z`) / 重做 (`Ctrl+Y`) 栈。

\- 一键将画布保存为 PNG / JPEG 图片。



\## 🛠️ 构建与运行环境

\- \*\*语言\*\*：C++17

\- \*\*框架\*\*：Qt 6 (Core, Gui, Widgets)

\- \*\*构建工具\*\*：CMake (>= 3.16)

\- \*\*编译器\*\*：MSVC 2022 (Visual Studio) / MinGW



\## 🚀 如何编译
修改 CMakeLists.txt 中的 Qt 路径为你电脑上的实际路径：

cmake
set(CMAKE_PREFIX_PATH "你的Qt安装路径/msvc2022_64" ${CMAKE_PREFIX_PATH})
使用 Qt Creator 或 Visual Studio 打开 CMakeLists.txt 进行编译。

编译成功后，使用 windeployqt 提取依赖库即可双击运行。


1\. 克隆本项目到本地：
```bash
git clone https://github.com/Sejzap/TextEditorAndDraw.git
```



