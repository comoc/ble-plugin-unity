#!/bin/bash

# ビルドディレクトリの作成
mkdir -p build
cd build

# CMakeの実行
cmake ..

# ビルドの実行
make

# プラグインのコピー
# mkdir -p ../../../Assets/Plugins/x86_64
cp libbleplugin.so ../../../Assets/Plugins/x86_64/

echo "Build completed successfully!"
