import XCTest
@testable import RingBuffer

final class RingBufferTests: XCTestCase {
    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct
        // results.
        XCTAssertEqual(RingBuffer().text, "Hello, World!")
    }

    static var allTests = [
        ("testExample", testExample),
    ]
}
