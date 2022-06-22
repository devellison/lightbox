cmake_minimum_required(VERSION 3.20)

if(MSVC)
  add_compile_options(/W4 /WX /Zc:preprocessor)
  set(EXTRA_LIBS Strmiids.lib Mfplat.lib Mf.lib delayimp.lib)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-c11-extensions)
  set(EXTRA_LIBS fmt::fmt)
endif()

if(OpenMP_CXX_FOUND)
  set(EXTRA_LIBS OpenMP::OpenMP_CXX)
endif()

# Basic unit tests
add_executable(zebral_test zebral_test.cpp)
target_link_libraries(zebral_test gtest_main zebral::zebral ${EXTRA_LIBS})
gtest_add_tests(TARGET zebral_test)
