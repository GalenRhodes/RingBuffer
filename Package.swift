// swift-tools-version:5.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(name: "RingBuffer", products: [ .library(name: "RingBuffer", targets: [ "RingBuffer" ]) ], targets: [ .target(name: "RingBuffer", dependencies: []) ])
