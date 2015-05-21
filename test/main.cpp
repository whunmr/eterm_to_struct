#include <iostream>
#include <gtest/gtest.h>
using namespace std;

#include "erl_interface.h"
#include "ei.h"

int main(int argc, char** argv) {
  erl_init(NULL, 0);
  ::testing::InitGoogleTest(&argc, argv);  
  return RUN_ALL_TESTS();
}

