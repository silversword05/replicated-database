# replicated-database
CS739 Project 2

### CMake Installation
```
mkdir -p $HOME/.local/
wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.19.6/cmake-3.19.6-Linux-x86_64.sh
sh cmake-linux.sh -- --skip-license --prefix=$HOME/.local/
export PATH="$PATH:$HOME/.local/bin"
cmake --version
rm cmake-linux.sh
```
**Install Cmake from source.** apt-get install is old version.

### GRPC Installation
```
export PATH="$PATH:$HOME/.local/bin"
mkdir -p $HOME/.local/
sudo apt install -y build-essential autoconf libtool pkg-config
git clone --recurse-submodules -b v1.50.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc; mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$HOME/.local/ ../..
make -j 4; make install
popd
```

### Run project
```
cd replicated-database
mkdir build; cd build;
cmake ..
make -j4
...
```

### Formatting requirements
- Download C/C++ extension from [here](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
- Go File -> Preferences -> Settings
- Search for C_Cpp.clang_format_fallbackStyle
- Change from "Visual Studio" to "LLVM"
