// swift-tools-version:5.4
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

//@f:0
let package = Package(
    name: "RingBuffer",
    platforms: [ .macOS(.v10_15), .tvOS(.v13), .iOS(.v13), .watchOS(.v6), ],
    products: [ .library(name: "RingBuffer", type: .static, targets: [ "RingBuffer" ]) ],
    targets: [ .target(name: "RingBuffer") ]
)
