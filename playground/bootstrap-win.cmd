set CWD=%cd%

md build-Win64-Debug 2> NUL
cd build-Win64-Debug
cmake -DCMAKE_BUILD_TYPE=Debug -A x64 -DARCH=Win64 ..

cd "%CWD%
md build-Win64-Release 2> NUL
cd build-Win64-Release
cmake -DCMAKE_BUILD_TYPE=Release -A x64 -DARCH=Win64 ..

cd "%CWD%
pause