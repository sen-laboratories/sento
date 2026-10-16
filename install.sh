#!/bin/bash
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
#
# Installs the SEN development headers to the writable (non-packaged) headers folder of the user.
USER_INCLUDES=$(findpaths -e B_FIND_PATH_HEADERS_DIRECTORY | grep /config/non-packaged | head -1)
# (the folder is not there on a system that never had non-packaged headers: it is made)
[ -n "$USER_INCLUDES" ] || USER_INCLUDES="$(finddir B_USER_NONPACKAGED_DEVELOP_DIRECTORY)/headers"

echo Installing SEN development headers into $USER_INCLUDES

mkdir -p "$USER_INCLUDES/sen" && \
cp ./src/cpp/include/* "$USER_INCLUDES/sen/" && \
echo "Successfully installed C++ headers to $USER_INCLUDES/sen." ||
echo "Error installing SEN includes to path $USER_INCLUDES/sen: $?"
