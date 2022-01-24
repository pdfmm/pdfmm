set -xe

export pwd=`pwd`
mkdir -p build-linux-x86_64-release
cd build-linux-x86_64-release
cmake -DCMAKE_BUILD_TYPE=Release -DARCH=linux-x86_64 ..

cd $pwd
mkdir -p build-linux-x86_64-debug
cd build-linux-x86_64-debug
cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=linux-x86_64 ..
