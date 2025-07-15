FROM ubuntu:latest

RUN apt-get update

RUN apt-get install -y cmake

RUN apt-get install -y g++

RUN apt-get install -y libgtest-dev && apt-get install -y libgmock-dev

RUN apt-get install -y libfmt-dev && apt-get install -y libyaml-cpp-dev

ARG work_dir=/usr/src/echo-web-server

RUN mkdir -p $work_dir

WORKDIR $work_dir

COPY . .

RUN mkdir build

WORKDIR $work_dir/build

RUN cmake -DECHO_WEB_SERVER_BUILD_TESTS=ON ..

RUN cmake --build .

WORKDIR $work_dir/build/bin

CMD ["./echo-web-server"]