// $Id: t_partfile.cc,v 1.4 2005-12-02 14:14:36 bjoo Exp $

#include <iostream>
#include <cstdio>

#include "qdp.h"
#include "unittest.h"

// The Unit Tests Themselves
#include "testOpenPartFile.h"
#include "testIONode.h"

using namespace QDP;

int main(int argc, char **argv)
{
  // Initialize QDP++ with argc, and argv. Set Lattice Dimensions
  const int latdims[] = {4,4,8,8};

  // Initialize UnitTest jig
  TestRunner  testjig(&argc, &argv, latdims);
  
  // Add a test -- to open a partfile
  testjig.addTest(new TestOpenPartFile(), string("TestOpenPartFile"));

#if 0   
  // These tests are disabled pending a decision about what we do 
  // with the partfile.

  testjig.addTest(new TestSingleFileIONode(), string("TestSingleFileIONode"));
  testjig.addTest(new TestMultiFileIONode(), string("TestMultiFileIONode"));
  testjig.addTest(new TestPartFileIONode1(), string("TestPartFileIONode1"));
  testjig.addTest(new TestPartFileIONode2(), string("TestPartFileIONode2"));
  testjig.addTest(new TestPartFileIONode3(), string("TestPartFileIONode3"));
#endif

  // Run all tests
  testjig.run();

  // Testjig is destroyed
  testjig.summary();

}

