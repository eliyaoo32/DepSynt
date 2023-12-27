# Preparing the image
FROM ubuntu:latest
WORKDIR /usr/src/app
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libboost-all-dev

COPY . .

# Build ABC
RUN cd ./libs/abc && make ABC_USE_NO_READLINE=1 ABC_USE_PIC=1 libabc.so
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/usr/src/app/libs/abc"


# Create a build directory
RUN mkdir build && cd build
RUN cd build && cmake .. && make

CMD ["./build/synthesis"]