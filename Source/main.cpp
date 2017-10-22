#include <iostream>
#include "runClient.hpp"



// Application Entry point
int main(int argc, char const *argv[])
{
  if (argc != 2)
  {
    std::cout << "Incorrect arguments! Please include the target stat collection server. Note: All stat collection servers are assumed to run on port " + PORT;
    return 0;
  }

  RunClient(argv[0], argv[1]);

  return 0;
}

