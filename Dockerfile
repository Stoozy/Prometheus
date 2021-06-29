FROM ubuntu
LABEL org.opencontainers.image.authors="Stoozy"
ENV TZ=America/New_York
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

ENV PREFIX="/src/opt/cross"
ENV TARGET="i386-elf"
ENV PATH="$PREFIX/bin:$PATH"

# Install deps

RUN yes | apt-get update 
RUN yes | apt-get install build-essential
RUN yes | apt-get install     bison
RUN yes | apt-get install     flex
RUN yes | apt-get install     libgmp3-dev
RUN yes | apt-get install     libmpc-dev
RUN yes | apt-get install     libmpfr-dev
RUN yes | apt-get install     texinfo
RUN yes | apt-get install     vim
RUN yes | apt-get install     git
RUN yes | apt-get install     wget
RUN yes | apt-get install     nasm
RUN yes | apt-get install     xorriso
RUN yes | apt-get install     grub2


WORKDIR /

RUN mkdir src 

WORKDIR /src

# Download and Extract binutils
RUN wget https://ftp.gnu.org/gnu/binutils/binutils-2.31.tar.gz && tar -xvzf  binutils-2.31.tar.gz && rm -rf binutils-2.31.tar.gz

WORKDIR /src
# Configure and Install binutils
RUN mkdir build-binutils && cd build-binutils && linux32 ../binutils-2.31/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror && linux32 make && linux32 make install


WORKDIR /src/

# Download and Extract gcc
RUN  wget https://ftp.gnu.org/gnu/gcc/gcc-11.1.0/gcc-11.1.0.tar.gz && tar -xvzf gcc-11.1.0.tar.gz && rm -rf gcc-11.1.0.tar.gz

WORKDIR /src/

# Configure and Install gcc
RUN mkdir build-gcc && cd build-gcc && linux32 ../gcc-11.1.0/configure  --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers && linux32 make all-gcc && linux32 make all-target-libgcc && linux32 make install-gcc && linux32 make install-target-libgcc

# Clone actual project
RUN rm -rf /src/Dead-OS
RUN git clone https://github.com/Stoozy/Dead-OS.git
RUN chmod +x /src/Dead-OS/build.sh
RUN chmod +x /src/Dead-OS/iso.sh

CMD cd /src/Dead-OS/ && ./build.sh && cd /src/Dead-OS/  && ./iso.sh 
