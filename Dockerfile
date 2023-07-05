FROM ubuntu:latest

RUN apt update -y && apt install -y \
    build-essential autotools-dev   \
    libboost-all-dev cmake \
    netcat

WORKDIR /home

COPY server.cc /home

RUN g++ -std=c++17 /home/server.cc -o server

CMD ["/home/server"]