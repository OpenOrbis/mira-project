protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` FileManager.proto
protoc -I . --cpp_out=. FileManager.proto