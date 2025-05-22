---
**Available Languages:** [English](README.md) | [中文 (Chinese)](README_zh.md) | [日本語 (Japanese)](README_ja.md)
---

# Project: C++ Client Simulator for Protobuf Services

## Project Overview

This project is a C++ network client simulator designed for testing protobuf-based services. It allows users to define complex request sequences and simulate multiple clients concurrently to stress-test and verify server-side applications. The simulator dynamically handles Protobuf messages based on runtime configuration.

The original project description was: "一个通用的网络客户端模拟程序框架，用于测试各种基于protobuf的网络服务。(A general network client simulation program framework for testing various protobuf-based network services.)"

## Features

*   **Dynamic Protobuf Message Handling**: Loads and uses Protobuf definitions at runtime, allowing for flexible request and response types without recompilation for new `.proto` files.
*   **Configurable Request Sequences**: Define client behavior through configuration files, specifying sequences of actions, request messages, and expected responses.
*   **Concurrent Client Simulation**: Simulates multiple clients operating in parallel using a thread pool.
*   **Flexible Configuration**: Client groups, actions, and message bodies are configured via Protobuf Text Format files.
*   **GoogleTest Integration**: Includes a test suite for verifying functionality (work in progress).

## Dependencies

The following libraries and tools are required to build and run the project:

*   **CMake**: Version 2.6 or higher (Note: `CMakeLists.txt` currently specifies 2.6, but `FetchContent` for GoogleTest might benefit from a more recent version like 3.11+).
*   **C++ Compiler**: A C++ compiler supporting C++11 or later (e.g., g++).
*   **GLib**: Development libraries (e.g., `libglib2.0-dev`).
*   **Protocol Buffers (Protobuf)**:
    *   `libprotobuf-dev` (Protobuf library and headers)
    *   `protobuf-compiler` (for compiling `.proto` files)
    *   `libprotoc-dev` (Protoc library, specifically for `importer.h`)
*   **glog**: Google Logging Library (e.g., `libgoogle-glog-dev`).
*   **gflags**: Google Commandline Flags Library (e.g., `libgoogle-gflags-dev`).
*   **OpenSSL**: Development libraries for SSL/TLS (e.g., `libssl-dev`).
*   **GoogleTest**: For development and running tests. Fetched automatically by CMake via `FetchContent` (currently using `release-1.12.1`).
*   **Valgrind**: (Optional, for memory debugging)
*   **apt-file**: (Optional, for finding which packages provide specific files, useful during dependency resolution)

## Building the Project

1.  **Install Dependencies**:
    Ensure all dependencies listed above are installed on your system. For Debian/Ubuntu-based systems, you can use `apt-get`:
    ```bash
    sudo apt-get update
    sudo apt-get install -y cmake g++ make libglib2.0-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-glog-dev libgoogle-gflags-dev libssl-dev
    ```

2.  **Compile Script**:
    The project uses a shell script to manage CMake configuration and building.

    *   **To build the main application (`robot`)**:
        ```bash
        ./COMPILE
        ```
        This will create a `cmake_build` directory, run CMake, and then run `make`. The final `robot` executable will be moved to the project root directory.

    *   **To build the tests (`robot_tests`)**:
        The tests require GoogleTest, which is fetched via `FetchContent`.
        ```bash
        ./COMPILE robot_tests
        ```
        This will build the `robot_tests` executable inside the `cmake_build` directory.

    *   **To build all targets (including the main application and tests)**:
        ```bash
        ./COMPILE all
        ```
        This ensures both `robot` and `robot_tests` (and GoogleTest libraries) are built. The `robot` executable will be in the project root, and `robot_tests` will be in `cmake_build/`.

## Running the Application

The main application simulates clients based on a configuration file.

*   **Command-line usage**:
    ```bash
    ./robot --configfullpath=<path_to_config_file>
    ```
    For example:
    ```bash
    ./robot --configfullpath=proto/robot.pbconf
    ```
    Other command-line flags (defined in `flags.cc`) can be listed with `--help`.

## Running Tests

After building the tests (e.g., using `./COMPILE all` or `./COMPILE robot_tests`):

*   **Execute tests**:
    ```bash
    ./cmake_build/robot_tests
    ```
    This will run all tests linked into the `robot_tests` executable.

## Configuration

The primary configuration for the robot simulator is done via a Protobuf Text Format file, typically `proto/robot.pbconf`. This file is an instance of the `pbcfg.CfgRoot` message defined in `proto/robot.proto`.

Key configuration sections include:

*   **`proto_path`**: Specifies directories where the simulator should look for `.proto` files to dynamically load message definitions.
*   **`body_config`**: Defines reusable message templates. Each entry has:
    *   `uniq_name`: A unique identifier for this message template.
    *   `type_name`: The fully qualified Protobuf message type (e.g., `MyNamespace.MyRequest`).
    *   `body_str`: The message content in Protobuf Text Format.
*   **`group_config`**: Defines groups of simulated clients. Each group can have:
    *   `name`: Name of the client group.
    *   `peer_addr`: Server address (e.g., "localhost:50051").
    *   `client_count`: Number of concurrent clients in this group.
    *   `loop_count`: How many times each client should execute its action sequence.
    *   `action`: A list of actions to be performed by each client in the group. Each action specifies:
        *   `request_uniq_name`: References a `uniq_name` from `body_config` to be used as the request.
        *   `response`: A list of expected response message types for validation.
        *   `timeout`: Timeout for waiting for a response.
    *   Other parameters like `max_pkg_len`, `has_checksum`, etc.

## Project Structure

*   **`/` (Root Directory)**: Contains main source files (`.cc`, `.h`), `CMakeLists.txt`, and build/utility scripts.
*   **`proto/`**:
    *   Contains Protobuf definition files (`.proto`).
    *   `robot.proto`: Defines core configuration messages like `CfgRoot`, `Group`, `Action`, `Client`, `Body`.
    *   `robot.pbconf`: Example configuration file in Protobuf Text Format.
    *   `proto/out/`: Default output directory for C++ files generated by `protoc` from `.proto` files (e.g., `robot.pb.cc`, `robot.pb.h`). This directory is created during the build.
*   **`tests/`**: Contains test source files.
    *   `mock_client.h`: Mock client implementation for testing.
    *   `test_robot_run_group_once.cc`: Example test case using GoogleTest.
*   **`cmake/`**: Contains CMake helper scripts, such as `Findglib.cmake` and `Findprotobuf.cmake`.
*   **`cmake_build/`**: Build directory created by the `./COMPILE` script. Contains CMake cache, Makefiles, object files, and built executables (like `robot_tests`). The main `robot` executable is moved to the root directory after a successful build.

---

This README provides a comprehensive guide to understanding, building, and running the project.
