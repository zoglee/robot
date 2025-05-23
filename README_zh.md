# 项目：用于 Protobuf 服务的 C++ 客户端模拟器

## 项目概述

本项目是一个 C++ 网络客户端模拟器，专为测试基于 Protobuf 的服务而设计。它允许用户定义复杂的请求序列，并模拟多个客户端并发执行，以对服务器端应用程序进行压力测试和功能验证。该模拟器能基于运行时配置动态处理 Protobuf 消息。

原始项目描述为："一个通用的网络客户端模拟程序框架，用于测试各种基于protobuf的网络服务。"

## 功能特性

*   **动态 Protobuf 消息处理**：在运行时加载和使用 Protobuf 定义，支持灵活的请求和响应类型，无需为新的 `.proto` 文件重新编译。
*   **可配置的请求序列**：通过配置文件定义客户端行为，指定操作序列、请求消息和预期响应。
*   **并发客户端模拟**：使用线程池模拟多个客户端并行操作。
*   **灵活的配置**：客户端组、操作和消息体通过 Protobuf 文本格式文件进行配置。
*   **GoogleTest 集成**：包含一个用于验证功能的测试套件（正在进行中）。

## 依赖项

构建和运行本项目需要以下库和工具：

*   **CMake**：版本 2.6 或更高（注意：`CMakeLists.txt` 当前指定为 2.6，但用于 GoogleTest 的 `FetchContent` 可能需要更新的版本，如 3.11+）。
*   **C++ 编译器**：支持 C++11 或更高版本的 C++ 编译器（例如 g++）。
*   **GLib**：开发库（例如 `libglib2.0-dev`）。
*   **Protocol Buffers (Protobuf)**：
    *   `libprotobuf-dev`（Protobuf 库和头文件）
    *   `protobuf-compiler`（用于编译 `.proto` 文件）
    *   `libprotoc-dev`（Protoc 库，特别是 `importer.h` 所需）
*   **glog**：Google 日志库（例如 `libgoogle-glog-dev`）。
*   **gflags**：Google 命令行标志库（例如 `libgoogle-gflags-dev`）。
*   **OpenSSL**：用于 SSL/TLS 的开发库（例如 `libssl-dev`）。
*   **GoogleTest**：用于开发和运行测试。由 CMake 通过 `FetchContent` 自动获取（当前使用 `release-1.12.1` 版本）。
*   **Valgrind**：（可选，用于内存调试）
*   **apt-file**：（可选，用于查找提供特定文件的包，有助于解决依赖问题）

## 构建项目

1.  **安装依赖项**：
    确保系统已安装上述所有依赖项。对于 Debian/Ubuntu 系统，您可以使用 `apt-get`：
    ```bash
    sudo apt-get update
    sudo apt-get install -y cmake g++ make libglib2.0-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-glog-dev libgoogle-gflags-dev libssl-dev
    ```

2.  **编译脚本**：
    项目使用一个 shell 脚本来管理 CMake 配置和构建。

    *   **构建主应用程序 (`robot`)**：
        ```bash
        ./COMPILE
        ```
        这将创建一个 `cmake_build` 目录，运行 CMake，然后运行 `make`。最终的 `robot` 可执行文件将被移动到项目根目录。

    *   **构建测试 (`robot_tests`)**：
        测试需要 GoogleTest，它通过 `FetchContent` 获取。
        ```bash
        ./COMPILE robot_tests
        ```
        这将在 `cmake_build` 目录中构建 `robot_tests` 可执行文件。

    *   **构建所有目标 (包括主应用程序和测试)**：
        ```bash
        ./COMPILE all
        ```
        这将确保 `robot` 和 `robot_tests`（以及 GoogleTest 库）都被构建。`robot` 可执行文件将在项目根目录中，而 `robot_tests` 将在 `cmake_build/` 目录中。

## 运行应用程序

主应用程序根据配置文件模拟客户端。

*   **命令行用法**：
    ```bash
    ./robot --configfullpath=<配置文件路径>
    ```
    例如：
    ```bash
    ./robot --configfullpath=proto/robot.pbconf
    ```
    其他命令行标志（定义在 `flags.cc` 中）可以通过 `--help` 查看。

## 运行测试

构建测试后（例如，使用 `./COMPILE all` 或 `./COMPILE robot_tests`）：

*   **执行测试**：
    ```bash
    ./cmake_build/robot_tests
    ```
    这将运行链接到 `robot_tests` 可执行文件中的所有测试。

## 配置

机器人模拟器的主要配置通过 Protobuf 文本格式文件完成，通常是 `proto/robot.pbconf`。此文件是 `proto/robot.proto` 中定义的 `pbcfg.CfgRoot` 消息的一个实例。

关键配置部分包括：

*   **`proto_path`**：指定模拟器查找 `.proto` 文件以动态加载消息定义的目录。
*   **`body_config`**：定义可重用的消息模板。每个条目包含：
    *   `uniq_name`：此消息模板的唯一标识符。
    *   `type_name`：完全限定的 Protobuf 消息类型（例如 `MyNamespace.MyRequest`）。
    *   `body_str`：Protobuf 文本格式的消息内容。
*   **`group_config`**：定义模拟客户端组。每个组可以包含：
    *   `name`：客户端组的名称。
    *   `peer_addr`：服务器地址（例如 "localhost:50051"）。
    *   `client_count`：此组中的并发客户端数量。
    *   `loop_count`：每个客户端应执行其操作序列的次数。
    *   `action`：组中每个客户端要执行的操作列表。每个操作指定：
        *   `request_uniq_name`：引用 `body_config` 中的 `uniq_name` 作为请求。
        *   `response`：用于验证的预期响应消息类型列表。
        *   `timeout`：等待响应的超时时间。
    *   其他参数，如 `max_pkg_len`、`has_checksum` 等。

### 帧头配置 (通过 YAML)

二进制帧头（位于 `CsMsgHead` Protobuf 消息之前的字节序列）的结构现在可以使用 YAML 文件进行自定义。这允许为不同的服务器协议定义自定义的数据包结构。

*   **命令行标志**：使用 `--frameheadconfig=<YAML文件路径>` 标志指定 YAML 文件。
    *   示例：`./robot --configfullpath=proto/robot.pbconf --frameheadconfig=my_custom_header.yaml`
    *   默认值：`"frame_header.yaml"` （如果此文件存在于工作目录中，则默认加载）。如果标志为空或文件未找到，模拟器将回退到其原始的硬编码帧结构。

*   **YAML 结构**：
    YAML 文件应定义一个 `frame_header` 映射，其中包含一个 `fields` 序列。

    ```yaml
    frame_header:
      fields:
        - name: descriptive_field_name
          size: <integer_bytes>
          type: <string_data_type>
          value: <literal_value_or_calculation_rule_string>
        # ... 更多字段 ...
    ```

*   **字段属性**：
    *   `name`：字段的描述性名称（主要为了 YAML 的可读性）。
    *   `size`：字段的大小（字节）（例如, 1, 2, 4, 8）。
    *   `type`：字段的数据类型和字节序。支持的类型：
        *   `uint8`, `int8`
        *   `uint16_be`, `uint16_le`, `int16_be`, `int16_le` (大端, 小端)
        *   `uint32_be`, `uint32_le`, `int32_be`, `int32_le`
        *   `uint64_be`, `uint64_le`, `int64_be`, `int64_le`
        *   `fixed_string` (`value` 应为字符串，将用空字符填充或截断以适应 `size`)。
    *   `value`：字段值的确定方式：
        *   **字面量值**：可以是整数（例如 `123`, `0xCAFE`）或 `fixed_string` 类型的字符串（例如 `"HELLO"`）。
        *   `"CALC_TOTAL_PACKET_LENGTH"`：该值将是此 `frame_header` YAML 结构中定义的所有字段的总长度，再加上 `CsMsgHead` 和 `CsMsgBody` Protobuf 消息的长度。
        *   `"CALC_PROTOBUF_HEAD_LENGTH"`：该值将是此字段本身的大小（由 `size` 定义）加上序列化的 `CsMsgHead` Protobuf 消息的长度。

*   **示例 (`frame_header.yaml` 模拟默认行为)**：
    此配置复制了先前在客户端编码逻辑中硬编码的两个长度字段。
    ```yaml
    # 示例: frame_header.yaml (模拟默认行为)
    frame_header:
      fields:
        - name: total_packet_length
          size: 4
          type: uint32_be
          value: "CALC_TOTAL_PACKET_LENGTH"
        - name: proto_header_length
          size: 4
          type: uint32_be
          value: "CALC_PROTOBUF_HEAD_LENGTH"
    ```

## 项目结构

*   **`/` (根目录)**：包含主要源文件（`.cc`、`.h`）、`CMakeLists.txt` 以及构建/实用工具脚本。
*   **`proto/`**：
    *   包含 Protobuf 定义文件（`.proto`）。
    *   `robot.proto`：定义核心配置消息，如 `CfgRoot`、`Group`、`Action`、`Client`、`Body`。
    *   `robot.pbconf`：Protobuf 文本格式的示例配置文件。
    *   `proto/out/`：由 `protoc` 从 `.proto` 文件生成的 C++ 文件（例如 `robot.pb.cc`、`robot.pb.h`）的默认输出目录。此目录在构建过程中创建。
*   **`tests/`**：包含测试源文件。
    *   `mock_client.h`：用于测试的 mock 客户端实现。
    *   `test_robot_run_group_once.cc`：使用 GoogleTest 的示例测试用例。
*   **`cmake/`**：包含 CMake 辅助脚本，如 `Findglib.cmake` 和 `Findprotobuf.cmake`。
*   **`cmake_build/`**：由 `./COMPILE` 脚本创建的构建目录。包含 CMake 缓存、Makefiles、目标文件和构建的可执行文件（如 `robot_tests`）。主 `robot` 可执行文件在成功构建后会移至根目录。

---

本 README 提供了理解、构建和运行项目的全面指南。
