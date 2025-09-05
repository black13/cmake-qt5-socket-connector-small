#!/bin/bash

# NodeGraph Dependencies Installer for WSL Ubuntu
echo "Installing NodeGraph dependencies..."

# Update package list
echo "Updating package list..."
sudo apt update

# Install required packages
echo "Installing development packages..."
sudo apt install -y \
    libxml2-dev \
    build-essential \
    cmake \
    pkg-config \
    libgl1-mesa-dev \
    libxkbcommon-x11-0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xinput0 \
    libxcb-xfixes0

echo "Dependencies installed successfully!"
echo ""
echo "Now you can run: ./build.sh"