#include "Manager.hpp"
#include "FileManagerImpl.hpp"

#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

using namespace Mira::Rpc;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;


        
Manager::Manager()
{
    std::string s_ServerAddress("0.0.0.0:9999");
    FileManagerImpl s_Service;
    ServerBuilder s_Builder;

    s_Builder.AddListeningPort(s_ServerAddress, grpc::InsecureServerCredentials());

    s_Builder.RegisterService(&s_Service);

    std::unique_ptr<Server> s_Server(s_Builder.BuildAndStart());
    std::cout << "Server listening on " << s_ServerAddress << std::endl;

    s_Server->Wait();
}

Manager::~Manager()
{

}
