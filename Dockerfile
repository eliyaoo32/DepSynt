# Preparing the image
FROM debian:latest
WORKDIR /usr/src/app
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libboost-all-dev \
    wget \
    gnupg2
# TODO: install libboost-program-options-dev instead of libboost-all-dev (Check where boost/range and boost/algorithm and requires)


# Install spot
RUN wget -q -O - https://www.lrde.epita.fr/repo/debian.gpg | apt-key add -
RUN echo 'deb http://www.lrde.epita.fr/repo/debian/ stable/' >> /etc/apt/sources.list
RUN apt-get update && apt-get install -y \
    spot=2.11.4.0-1 libspot-dev=2.11.4.0-1 \
    libspot0=2.11.4.0-1 libspotltsmin0=2.11.4.0-1 libspotgen0=2.11.4.0-1 \
    libbddx0=2.11.4.0-1 libbddx-dev=2.11.4.0-1

# Install BoostJson
RUN echo "deb http://deb.debian.org/debian sid main" >> /etc/apt/sources.list
RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-json-dev \
    && rm -rf /var/lib/apt/lists/*

# Build ABC
COPY ./libs/abc /usr/src/app/libs/abc
RUN cd ./libs/abc && make ABC_USE_NO_READLINE=1 ABC_USE_PIC=1 libabc.so
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/usr/src/app/libs/abc"

# Copy project
COPY . .


# Create a build directory
RUN cmake . && make synthesis

ENTRYPOINT ["./synthesis"]
