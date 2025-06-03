#!/bin/bash
USER_INCLUDES=$HOME/config/non-packaged/include
mkdir -p $USER_INCLUDES/sen && \
cp src/cpp/include/* $USER_INCLUDES/sen/ && \
echo Successfully installed CPP headers to user includes path $USER_INCLUDES/sen.
