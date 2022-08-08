cmake_minimum_required(VERSION 3.20)

project(
  lightbox_app
  VERSION ${CMAKE_PROJECT_VERSION}
  LANGUAGES CXX
)

if(MSVC)
  add_compile_options(/W4 /WX /Zc:preprocessor)
  set(EXTRA_LIBS Strmiids.lib Mfplat.lib Mf.lib delayimp.lib)
  # add_link_options(/DELAYLOAD:Mf.lib)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-c11-extensions)
endif()

if(OpenMP_CXX_FOUND)
  set(EXTRA_LIBS ${EXTRA_LIBS} OpenMP::OpenMP_CXX)
endif()

# Add lightbox app (move these to separate library)
set(SRC src/lightbox.cpp src/lightbox_app.cpp)
set(INC inc/lightbox.hpp)
add_executable(${PROJECT_NAME} ${SRC} ${INC})
target_include_directories(${PROJECT_NAME} PUBLIC 
  inc 
  ${NanoGUI_SOURCE_DIR}/include 
  ${NANOGUI_EXTRA_INCS} 
  ${OpenCV_INCLUDE_DIRS}   
  zebralcam::zebralcam 
  zebralserial::zebralserial
  zebralcommon::zebralcommon
  zebralnetwork::zebralnetwork
)
target_compile_definitions(
  ${PROJECT_NAME} PUBLIC LIGHTBOX_VERSION="${PROJECT_VERSION}" ${NANOGUI_EXTRA_DEFS}
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries( ${PROJECT_NAME} 
  nanogui 
  ${NANOGUI_EXTRA_LIBS} 
  ${OpenCV_LIBS}
  zebralcam::zebralcam 
  zebralserial::zebralserial
  zebralcommon::zebralcommon
  zebralnetwork::zebralnetwork
)

# Add lightbox camera test
add_executable(lbcamtest src/lbcamtest.cpp)
target_include_directories(lbcamtest PUBLIC 
  inc  
  ${OpenCV_INCLUDE_DIRS}
  zebralcam::zebralcam 
  zebralserial::zebralserial
  zebralcommon::zebralcommon
  zebralnetwork::zebralnetwork
)

target_compile_definitions(lbcamtest PUBLIC LIGHTBOX_VERSION="${PROJECT_VERSION}")
target_compile_features(lbcamtest PUBLIC cxx_std_20)
target_link_libraries(lbcamtest  
  ${OpenCV_LIBS}
  zebralcam::zebralcam 
  zebralserial::zebralserial
  zebralcommon::zebralcommon
  zebralnetwork::zebralnetwork
)


install(TARGETS ${PROJECT_NAME} lbcamtest DESTINATION ${CMAKE_INSTALL_BINDIR})
