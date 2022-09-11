#include"Reactor.hpp"
#include"Crud.hpp"
Crud crud("localhost","root","123456","NetDisk");


void test(){
    ReactorServer server(8888,"172.24.40.165",4,5);
    server.start();
}

int main()
{
    test();
    return 0;
}