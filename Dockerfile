FROM ubuntu:latest

RUN apt update

RUN apt install -y cmake

RUN apt install -y clang

RUN apt install -y libgtest-dev \
    && apt install -y libgmock-dev

RUN apt install -y libfmt-dev \
    && apt install -y libyaml-cpp-dev

ARG work_dir=/usr/src/echo-web-server

RUN mkdir -p $work_dir

WORKDIR $work_dir

COPY . .

RUN mkdir build

WORKDIR $work_dir/build

RUN cmake ..

RUN cmake --build .

WORKDIR $work_dir/build/bin

CMD ["./echo-web-server"]