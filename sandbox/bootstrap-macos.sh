set -xe

export pwd=`pwd`
mkdir -p build-macosx-x86_64-release
cd build-macosx-x86_64-release
cmake -DCMAKE_BUILD_TYPE=Release -DARCH=macosx-x86_64 ..

cd $pwd
mkdir -p build-macosx-x86_64-debug
cd build-macosx-x86_64-debug
cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=macosx-x86_64 ..
