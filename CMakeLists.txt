cmake_minimum_required(VERSION 3.23)
project(xshm_cap C)

set(CMAKE_C_STANDARD 99)

add_library(xshm_cap SHARED xhelpers.h xhelpers.c xshm-cap.h xshm-cap.c)


target_link_libraries(xshm_cap
        libX11.so
        libXtst.so
        libxcb.so
        libxcb-xfixes.so
        libxcb-randr.so
        libxcb-shm.so
        libxcb-xinerama.so
        )