#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{    
    std::cout << "Testing 123" << std::endl;
    for (auto i = 0; i < argc; ++i)
    {
        std::cout << i << ": " << argv[i] << std::endl;
    }
    return 0;
}