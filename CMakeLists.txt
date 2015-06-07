# Copyright (C) 2015  Vianney le Clément de Saint-Marcq <vleclement@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

project(bootimgtool)
cmake_minimum_required(VERSION 2.8)

find_package(OpenSSL REQUIRED)

set(SRCS
    bootimgtool.c
    bootimgtool.h
    bootimg.h
    io.c
    io.h
    sha.c
    sha.h
    variant_standard.c
    variant_qcom.c
    variant_fsl.c
)

set(CMAKE_C_FLAGS "-Wall")

include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_CRYPTO_LIBRARY})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
