#!/bin/bash

# This script removes all files except .h files from a specified directory

# Check if a directory is provided as an argument
if [ -z "$1" ]; then
  echo "Usage: $0 <directory>"
  exit 1
fi

# Change to the specified directory
cd "$1" || exit

# Find and remove all files except .h files
find . -type f ! -name '*.h' -exec rm -f {} +

# Remove test/ directories if they exist
find . -type d -name 'test' -exec rm -rf {} +

echo "All files except .h files have been removed from $1"