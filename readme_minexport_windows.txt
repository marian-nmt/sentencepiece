To recreate these bits:

- git clone https://github.com/marian-nmt/sentencepiece.git
- cd sentencepiece
- git checkout anthonyaue/min_export_dll (or wait for this to go into master
branch and just pull that).
- run test.bat. This will build the statically-linked sentencepiece binaries.
- copy build\src\Release\*.exe \usr\tmp\sentencepiece_pack\bin
- copy build\src\Release\*.lib \usr\tmp\sentencepiece_pack\lib\amd64
- copy src\*.h \usr\tmp\sentencepiece_pack\include
- rmdir /s build
- rename CMakeLists.txt CMakeLists.txt.stock
- rename src\CMakeLists.txt src\CMakeLists.txt.stock
- copy CMakeLists_minexport.txt CMakeLists.txt
- copy src\CMakeLists_minexport.txt src\CMakeLists.txt
- run test_minexport.bat. This will build the dynamically-linked minimum
  export library (needed for the managed wrapper). It fails to build a bunch
  of other targets, but will build the parts you need. I spent way too much
  time trying to get this to work, but I suck at CMake and CMake sucks at
  Visual Studio / Windows so we're left with this inelegant hack. Sorry.
  When prompted about unzipping protobuf, choose 'N' because you've already
  done this.
- copy build\src\Release\sentencepiece_minexport.dll to
  \usr\tmp\sentencepiece_pack\bin
- copy build\src\Release\sentencepiece_minexport.lib to 
  \usr\tmp\sentencepiece_pack\lib
- copy protobuf-3.6.1\cmake\Release\libprotobuf.dll to
  \usr\tmp\sentencepiece_pack\bin
- Create your package (cd \usr\tmp; packagecreate SentencePiece
  \usr\tmp\sentencepiece_pack VER)
- Update corext.config as appropriate.

