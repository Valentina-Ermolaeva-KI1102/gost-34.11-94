#include <iostream>
#include "gost.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout<<"Use: ./GOST file_in\n"<<std::endl;
        return -1;
    }
    else
    {
		if(gost(argv[1], argv[2]))
			std::cout<<"Hesh complite!\n"<<std::endl;
        else
			std::cout<<"Hesh error!\n"<<std::endl;
    }
    return 0;
}
