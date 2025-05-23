# プロジェクト：Protobufサービス向け C++クライアントシミュレータ

## プロジェクト概要

このプロジェクトは、Protobufベースのサービスのテスト用に設計されたC++ネットワーククライアントシミュレータです。ユーザーは複雑なリクエストシーケンスを定義し、複数のクライアントを同時にシミュレートして、サーバーサイドアプリケーションのストレステストや検証を行うことができます。シミュレータは、実行時の設定に基づいてProtobufメッセージを動的に処理します。

元のプロジェクト説明：「一個通用的網絡客户端模擬程序框架，用於測試各種基於protobuf的網絡服務。(様々なProtobufベースのネットワークサービスをテストするための汎用ネットワーククライアントシミュレーションプログラムフレームワーク。)」

## 機能

*   **動的Protobufメッセージ処理**：実行時にProtobuf定義をロードして使用し、新しい`.proto`ファイルのために再コンパイルすることなく、柔軟なリクエストおよびレスポンスタイプを可能にします。
*   **設定可能なリクエストシーケンス**：設定ファイルを通じてクライアントの動作を定義し、アクションのシーケンス、リクエストメッセージ、および期待されるレスポンスを指定します。
*   **並行クライアントシミュレーション**：スレッドプールを使用して並行して動作する複数のクライアントをシミュレートします。
*   **柔軟な設定**：クライアントグループ、アクション、およびメッセージボディは、Protobufテキストフォーマットファイルを介して設定されます。
*   **GoogleTest連携**：機能検証のためのテストスイートが含まれています（作業中）。

## 依存関係

プロジェクトのビルドと実行には、以下のライブラリとツールが必要です：

*   **CMake**：バージョン2.6以上（注意：`CMakeLists.txt`は現在2.6を指定していますが、GoogleTestの`FetchContent`は3.11+のようなより最近のバージョンが望ましい場合があります）。
*   **C++コンパイラ**：C++11以降をサポートするC++コンパイラ（例：g++）。
*   **GLib**：開発ライブラリ（例：`libglib2.0-dev`）。
*   **Protocol Buffers (Protobuf)**：
    *   `libprotobuf-dev`（Protobufライブラリとヘッダー）
    *   `protobuf-compiler`（`.proto`ファイルのコンパイル用）
    *   `libprotoc-dev`（Protocライブラリ、特に`importer.h`用）
*   **glog**：Googleロギングライブラリ（例：`libgoogle-glog-dev`）。
*   **gflags**：Googleコマンドラインフラグライブラリ（例：`libgoogle-gflags-dev`）。
*   **OpenSSL**：SSL/TLS用開発ライブラリ（例：`libssl-dev`）。
*   **GoogleTest**：テストの開発と実行用。CMakeによって`FetchContent`経由で自動的に取得されます（現在`release-1.12.1`を使用）。
*   **Valgrind**：（オプション、メモリデバッグ用）
*   **apt-file**：（オプション、特定のファイルを提供するパッケージを見つけるため、依存関係解決中に役立ちます）

## プロジェクトのビルド

1.  **依存関係のインストール**：
    上記のすべての依存関係がシステムにインストールされていることを確認してください。Debian/Ubuntuベースのシステムでは、`apt-get`を使用できます：
    ```bash
    sudo apt-get update
    sudo apt-get install -y cmake g++ make libglib2.0-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-glog-dev libgoogle-gflags-dev libssl-dev
    ```

2.  **コンパイルスクリプト**：
    プロジェクトは、CMakeの設定とビルドを管理するためにシェルスクリプトを使用します。

    *   **メインアプリケーションのビルド (`robot`)**：
        ```bash
        ./COMPILE
        ```
        これにより`cmake_build`ディレクトリが作成され、CMakeが実行され、次に`make`が実行されます。最終的な`robot`実行可能ファイルはプロジェクトのルートディレクトリに移動されます。

    *   **テストのビルド (`robot_tests`)**：
        テストにはGoogleTestが必要で、これは`FetchContent`経由で取得されます。
        ```bash
        ./COMPILE robot_tests
        ```
        これにより`robot_tests`実行可能ファイルが`cmake_build`ディレクトリ内にビルドされます。

    *   **すべてのターゲットのビルド (メインアプリケーションとテストを含む)**：
        ```bash
        ./COMPILE all
        ```
        これにより`robot`と`robot_tests`（およびGoogleTestライブラリ）の両方がビルドされます。`robot`実行可能ファイルはプロジェクトルートに、`robot_tests`は`cmake_build/`に配置されます。

## アプリケーションの実行

メインアプリケーションは設定ファイルに基づいてクライアントをシミュレートします。

*   **コマンドラインでの使用法**：
    ```bash
    ./robot --configfullpath=<設定ファイルへのパス>
    ```
    例：
    ```bash
    ./robot --configfullpath=proto/robot.pbconf
    ```
    その他のコマンドラインフラグ（`flags.cc`で定義）は`--help`で一覧表示できます。

## テストの実行

テストをビルドした後（例：`./COMPILE all`または`./COMPILE robot_tests`を使用）：

*   **テストの実行**：
    ```bash
    ./cmake_build/robot_tests
    ```
    これにより`robot_tests`実行可能ファイルにリンクされているすべてのテストが実行されます。

## 設定

ロボットシミュレータの主要な設定は、Protobufテキストフォーマットファイル（通常は`proto/robot.pbconf`）を介して行われます。このファイルは、`proto/robot.proto`で定義された`pbcfg.CfgRoot`メッセージのインスタンスです。

主要な設定セクションは次のとおりです：

*   **`proto_path`**：シミュレータがメッセージ定義を動的にロードするために`.proto`ファイルを検索するディレクトリを指定します。
*   **`body_config`**：再利用可能なメッセージテンプレートを定義します。各エントリには以下が含まれます：
    *   `uniq_name`：このメッセージテンプレートの一意の識別子。
    *   `type_name`：完全修飾されたProtobufメッセージタイプ（例：`MyNamespace.MyRequest`）。
    *   `body_str`：Protobufテキストフォーマットのメッセージ内容。
*   **`group_config`**：シミュレートされたクライアントのグループを定義します。各グループは以下を持つことができます：
    *   `name`：クライアントグループの名前。
    *   `peer_addr`：サーバーアドレス（例："localhost:50051"）。
    *   `client_count`：このグループ内の同時クライアント数。
    *   `loop_count`：各クライアントがアクションシーケンスを実行する回数。
    *   `action`：グループ内の各クライアントが実行するアクションのリスト。各アクションは以下を指定します：
        *   `request_uniq_name`：リクエストとして使用する`body_config`の`uniq_name`を参照します。
        *   `response`：検証用の期待されるレスポンスメッセージタイプのリスト。
        *   `timeout`：レスポンスを待つためのタイムアウト。
    *   `max_pkg_len`、`has_checksum`などのその他のパラメータ。

### フレームヘッダー設定 (YAML経由)

バイナリフレームヘッダー（`CsMsgHead` Protobufメッセージの前に置かれるバイト列）の構造をYAMLファイルを使用してカスタマイズできるようになりました。これにより、さまざまなサーバープロトコルに対応するカスタムパケット構造を定義できます。

*   **コマンドラインフラグ**: `--frameheadconfig=<YAMLファイルへのパス>` フラグを使用してYAMLファイルを指定します。
    *   例: `./robot --configfullpath=proto/robot.pbconf --frameheadconfig=my_custom_header.yaml`
    *   デフォルト値: `"frame_header.yaml"` （このファイルが作業ディレクトリに存在する場合、デフォルトでロードされます）。フラグが空の場合やファイルが見つからない場合、シミュレータは元のハードコードされたフレーム構造にフォールバックします。

*   **YAML構造**:
    YAMLファイルは `frame_header` マッピングを定義し、その中に `fields` シーケンスを含める必要があります。

    ```yaml
    frame_header:
      fields:
        - name: descriptive_field_name
          size: <integer_bytes>
          type: <string_data_type>
          value: <literal_value_or_calculation_rule_string>
        # ... 他のフィールド ...
    ```

*   **フィールド属性**:
    *   `name`: フィールドの記述名（主にYAMLの可読性のため）。
    *   `size`: フィールドのサイズ（バイト単位）（例: 1, 2, 4, 8）。
    *   `type`: フィールドのデータ型とエンディアン。サポートされている型：
        *   `uint8`, `int8`
        *   `uint16_be`, `uint16_le`, `int16_be`, `int16_le` （ビッグエンディアン, リトルエンディアン）
        *   `uint32_be`, `uint32_le`, `int32_be`, `int32_le`
        *   `uint64_be`, `uint64_le`, `int64_be`, `int64_le`
        *   `fixed_string` (`value`は文字列となり、指定された`size`に合わせてNULL文字でパディングされるか切り詰められます)。
    *   `value`: フィールドの値の決定方法：
        *   **リテラル値**: 整数（例: `123`, `0xCAFE`）または`fixed_string`型の場合は文字列（例: `"HELLO"`）。
        *   `"CALC_TOTAL_PACKET_LENGTH"`: この`frame_header` YAML構造で定義された全フィールドの合計長に、`CsMsgHead`および`CsMsgBody` Protobufメッセージの長さを加えたもの。
        *   `"CALC_PROTOBUF_HEAD_LENGTH"`: このフィールド自体のサイズ（`size`で定義）に、シリアライズされた`CsMsgHead` Protobufメッセージの長さを加えたもの。

*   **例 (`frame_header.yaml` デフォルト動作を模倣)**:
    この設定は、以前クライアントのエンコーディングロジックでハードコードされていた2つの長さフィールドを再現します。
    ```yaml
    # 例: frame_header.yaml (デフォルト動作を模倣)
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

## プロジェクト構造

*   **`/` (ルートディレクトリ)**：メインソースファイル（`.cc`、`.h`）、`CMakeLists.txt`、およびビルド/ユーティリティスクリプトが含まれています。
*   **`proto/`**：
    *   Protobuf定義ファイル（`.proto`）が含まれています。
    *   `robot.proto`：`CfgRoot`、`Group`、`Action`、`Client`、`Body`などのコア設定メッセージを定義します。
    *   `robot.pbconf`：Protobufテキストフォーマットのサンプル設定ファイル。
    *   `proto/out/`：`.proto`ファイルから`protoc`によって生成されたC++ファイル（例：`robot.pb.cc`、`robot.pb.h`）のデフォルト出力ディレクトリ。このディレクトリはビルド中に作成されます。
*   **`tests/`**：テストソースファイルが含まれています。
    *   `mock_client.h`：テスト用のモッククライアント実装。
    *   `test_robot_run_group_once.cc`：GoogleTestを使用したサンプルテストケース。
*   **`cmake/`**：CMakeヘルパースクリプト（`Findglib.cmake`や`Findprotobuf.cmake`など）が含まれています。
*   **`cmake_build/`**：`./COMPILE`スクリプトによって作成されるビルドディレクトリ。CMakeキャッシュ、Makefile、オブジェクトファイル、およびビルドされた実行可能ファイル（`robot_tests`など）が含まれています。メインの`robot`実行可能ファイルは、ビルド成功後にルートディレクトリに移動されます。

---

このREADMEは、プロジェクトの理解、ビルド、実行に関する包括的なガイドを提供します。
