# CMakeLists.txt
# Build configuration for filesystem component

idf_component_register(
    SRCS "filesystem.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_littlefs
)
