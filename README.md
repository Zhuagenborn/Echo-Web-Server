# Echo Web Server

![C++](docs/badges/C++.svg)
[![CMake](docs/badges/Made-with-CMake.svg)](https://cmake.org)
[![Docker](docs/badges/Made-with-Docker.svg)](https://www.docker.com)
[![GitHub Actions](docs/badges/Made-with-GitHub-Actions.svg)](https://github.com/features/actions)
![Linux](docs/badges/Linux.svg)
![License](docs/badges/License-MIT.svg)

## Introduction

A *C++20* echo web server running on *Linux*.

If a user inputs a name `<user>` and a message `<msg>`, the server will reply with `<user> said "<msg>"`.

- Using a *YAML*-based configuration system, supporting the notification of value changes and parsing containers and custom types.
- Using a customizable logging system, supporting synchronous and asynchronous modes.
- Using auto-expandable buffers to store data.
- Using a state machine to parse *HTTP* requests.
- Using a thread pool, an epoll and non-blocking sockets to process *HTTP* requests.
- Using a timer system based on a min-heap to close timed-out connections.
- Unit tests using *GoogleTest*.

## Getting Started

### Prerequisites

- Install and start [*Docker*](https://www.docker.com).

### Building

Set the location to the project folder and run:

```bash
docker image build . -t <image>
```

`<image>` should be replaced with a custom *Docker* image name.

### Running Tests

```bash
docker container run <image> ctest --test-dir .. -VV
```

## Usage

### Configuration

`config.yaml` is the default configuration. It will be copied to *Docker* during the building.

```yaml
server:
  # The TCP listening port.
  port: 10000
  # The website folder.
  asset_folder: "assets"
  # The maximum alive time for client timers (in seconds).
  # When a client's timer reaches zero and it has no activity, it will disconnect.
  alive_time: 60
loggers:
  - name: root
    level: info
    appenders:
      - type: stdout
```

Here are two ways to make a new configuration effective.

- Change `config.yaml` and build a new *Docker* image.
- Enter an existing *Docker* container and change `build/bin/config.yaml`.

See `src/log/README.md` for more details if you want to change logger configuration.

### Running the Server

```bash
docker container run -p <container-port>:<host-port> <image>
```

`<container-port>` should be equal to `server.port` in `config.yaml`. `<host-port>` can be any available port of the host machine. `-p <container-port>:<host-port>` binds `<container-port>` of the container to `<host-port>` of the host machine.

After the server is running, open a browser and access `http://localhost:<host-port>` on the host machine to use it.

You can run the following command and access `http://localhost:10000` if you are using the default configuration.

```bash
docker container run -p 10000:10000 <image>
```

## Unit Tests

The unit tests perform using the [*GoogleTest*](http://google.github.io/googletest) framework, consisting of public and private tests.

- Public tests are in the `tests` folder.
- Private tests are in the same folder as the corresponding modules.

The name of an unit test file ends with `_test`.

## Documents

The code comment style follows the [*Doxygen*](http://www.doxygen.nl) specification.

The class diagram follows the [*Mermaid*](https://mermaid-js.github.io/mermaid/#) specification.

## Structure

```
.
├── CITATION.cff
├── CMakeLists.txt
├── Dockerfile
├── LICENSE
├── README.md
├── .github
│   └── workflows
│       └── cmake-gtest.yaml
├── assets
│   ├── favicon.ico
│   ├── http-status.html
│   └── index.html
├── config.yaml
├── docs
│   └── badges
│       ├── C++.svg
│       ├── License-MIT.svg
│       ├── Linux.svg
│       ├── Made-with-CMake.svg
│       ├── Made-with-GitHub-Actions.svg
│       └── Made-with-Docker.svg
├── include
│   ├── config.h
│   ├── containers
│   │   ├── block_deque.h
│   │   ├── buffer.h
│   │   ├── epoller.h
│   │   ├── heap_timer.h
│   │   └── thread_pool.h
│   ├── http.h
│   ├── io.h
│   ├── ip.h
│   ├── log.h
│   ├── test_util.h
│   ├── util.h
│   └── web_server.h
├── src
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   └── config.cpp
│   ├── containers
│   │   ├── CMakeLists.txt
│   │   ├── buffer
│   │   │   ├── CMakeLists.txt
│   │   │   ├── README.md
│   │   │   └── buffer.cpp
│   │   ├── epoller
│   │   │   ├── CMakeLists.txt
│   │   │   └── epoller.cpp
│   │   └── thread_pool
│   │       ├── CMakeLists.txt
│   │       └── thread_pool.cpp
│   ├── http
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   ├── http.cpp
│   │   ├── request.cpp
│   │   ├── request.h
│   │   ├── request_test.cpp
│   │   ├── response.cpp
│   │   ├── response.h
│   │   └── response_test.cpp
│   ├── io
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   └── io.cpp
│   ├── ip
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   └── ip.cpp
│   ├── log
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   ├── appender.cpp
│   │   ├── config_init.cpp
│   │   ├── config_init.h
│   │   ├── config_init_test.cpp
│   │   ├── field.cpp
│   │   ├── field.h
│   │   ├── field_test.cpp
│   │   └── log.cpp
│   ├── main.cpp
│   ├── test_util
│   │   ├── CMakeLists.txt
│   │   └── test_util.cpp
│   └── util
│       ├── CMakeLists.txt
│       └── util.cpp
└── tests
    ├── CMakeLists.txt
    ├── config_test.cpp
    ├── containers
    │   ├── block_deque_test.cpp
    │   ├── buffer_test.cpp
    │   ├── heap_timer_test.cpp
    │   └── thread_pool_test.cpp
    ├── http_test.cpp
    ├── io_test.cpp
    ├── ip_test.cpp
    ├── log_test.cpp
    └── util_test.cpp
```

## Dependencies

- [*yaml-cpp*](https://github.com/jbeder/yaml-cpp)
- [*{fmt}*](https://github.com/fmtlib/fmt)

## References

- [*WebServer*](https://github.com/markparticle/WebServer)
- [*sylar*](https://github.com/sylar-yin/sylar)

## License

Distributed under the *MIT License*. See `LICENSE` for more information.