cmake_minimum_required(VERSION 3.16)
project(TextEditorAndDraw LANGUAGES CXX)

# ★ 1. 你的 Qt MSVC 路径
set(CMAKE_PREFIX_PATH "D:/Qt/6.12.0/msvc2022_64" ${CMAKE_PREFIX_PATH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ★ 2. 寻找 Qt 组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

# ★ 3. 源文件列表（加入了 app.rc）
set(PROJECT_SOURCES 
    main.cpp
    app.rc
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# ★ 4. 告诉 CMake 不需要对 main.cpp 扫描 MOC（因为我们把 Q_OBJECT 删了）
set_property(SOURCE main.cpp PROPERTY SKIP_AUTOMOC ON)

# ★ 5. 隐藏控制台黑框
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

# ★ 6. 针对 MSVC 强制使用 UTF-8 防乱码
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# ★ 7. 编译完成后，自动把 icon.ico 复制到 exe 所在目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/icon.ico"
)

# ★ 8. 链接 Qt 库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
)