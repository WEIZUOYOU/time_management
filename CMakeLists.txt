cmake_minimum_required(VERSION 3.15)
project(time_management)

# 明确设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 添加包含目录
include_directories(include)

# 查找Raylib
find_package(raylib REQUIRED)

# 添加可执行文件 - 包含所有必要的源文件
add_executable(time_management 
    src/main.c
    src/trash.c
    src/data.c
    src/achievement.c
)

# 链接Raylib
target_link_libraries(time_management raylib)

# 复制资源文件
add_custom_command(TARGET time_management POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:time_management>/assets)