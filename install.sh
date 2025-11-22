#!/bin/bash
USER_INCLUDES=$(findpaths -e B_FIND_PATH_HEADERS_DIRECTORY | grep /boot/home/config)

echo Installing SEN development headers into $USER_INCLUDES

mkdir -p $USER_INCLUDES/sen && \
cp ./src/cpp/include/* $USER_INCLUDES/sen/ && \
echo Successfully installed C++ headers to user includes path $USER_INCLUDES/sen. ||
echo Error installing SEN includes to path $USER_INCLUDES/sen: $?
