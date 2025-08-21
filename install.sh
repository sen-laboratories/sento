#!/bin/bash
USER_INCLUDES=$HOME/config/non-packaged/include

echo Installing SEN development headers into user includes at USER_INCLUDES

mkdir -p $USER_INCLUDES/sen && \
cp ./src/cpp/include/* $USER_INCLUDES/sen/ && \
echo Successfully installed C++ headers to user includes path $USER_INCLUDES/sen. ||
echo Error installing SEN includes to path $USER_INCLUDES/sen: $?
