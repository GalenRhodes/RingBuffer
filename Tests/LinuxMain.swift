import XCTest

import RingBufferTests

var tests = [XCTestCaseEntry]()
tests += RingBufferTests.allTests()
XCTMain(tests)
