# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set (LINK_DIR /Users/bloit/LIBS/link)

include(${LINK_DIR}/AbletonLinkConfig.cmake)
include_directories(src/main/cpp/android-ifaddrs)
include_directories(${LINK_DIR}/include)
include_directories(${LINK_DIR}/modules/asio-standalone/asio/include)

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             src/main/cpp/native-lib.cpp
             src/main/cpp/AudioEngine.cpp
             src/main/cpp/SineGenerator.cpp

             # for Link
             src/main/cpp/android-ifaddrs/ifaddrs.cpp
             )

target_compile_definitions(native-lib PRIVATE
  ANDROID=1 LINK_PLATFORM_LINUX=1 PD=1 ABL_LINK_OFFSET_MS=23)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       oboe
                       Ableton::Link)

# Set the path to the Oboe directory.
set (OBOE_DIR /Users/bloit/LIBS/oboe)

# Add the Oboe library as a subdirectory in your project.
add_subdirectory (${OBOE_DIR} ./oboe)

# Specify the path to the Oboe header files.
include_directories (${OBOE_DIR}/include)