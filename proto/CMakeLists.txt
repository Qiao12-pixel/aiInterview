# Qt 客户端和独立服务端共用的协议构建入口。
if(TARGET interview_protocol)
    return()
endif()
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED}")
add_custom_command(
    OUTPUT "${GENERATED}/interview.pb.cc" "${GENERATED}/interview.pb.h"
           "${GENERATED}/interview.grpc.pb.cc" "${GENERATED}/interview.grpc.pb.h"
    COMMAND protobuf::protoc --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${GENERATED} --grpc_out=${GENERATED}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            "${CMAKE_CURRENT_SOURCE_DIR}/interview.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/interview.proto"
    VERBATIM)
add_library(interview_protocol "${GENERATED}/interview.pb.cc" "${GENERATED}/interview.grpc.pb.cc")
set_target_properties(interview_protocol PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(interview_protocol PUBLIC "${GENERATED}")
target_link_libraries(interview_protocol PUBLIC gRPC::grpc++ protobuf::libprotobuf)
