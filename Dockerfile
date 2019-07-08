FROM ubuntu:19.04

RUN apt-get update
RUN apt-get install -y gcc g++ git cmake make ninja-build python
#RUN wget http://releases.llvm.org/8.0.0/llvm-8.0.0.src.tar.xz
RUN git clone -b 'llvmorg-8.0.0' --single-branch --depth 1  https://github.com/llvm/llvm-project.git
RUN mkdir -p /root/llvm8 
WORKDIR /llvm-project/clang
RUN cmake -G Ninja -DLLVM_ENABLE_PROJECTS="llvm" -DCMAKE_INSTALL_PREFIX="/root/llvm8"  -DCMAKE_BUILD_TYPE="MinSizeRel"  -DLLVM_TARGETS_TO_BUILD="X86;WebAssembly"  -DLLVM_BUILD_TOOLS="NO" -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_BINDINGS=OFF ../llvm
RUN ninja -j4
RUN ninja install
