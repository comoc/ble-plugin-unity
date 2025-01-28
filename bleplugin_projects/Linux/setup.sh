#!/bin/bash

# 必要なパッケージのインストール
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libbluetooth-dev \
    libdbus-1-dev \
    libglib2.0-dev

echo "Dependencies installed successfully!"
