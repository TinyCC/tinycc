# Use a specific version of Debian to avoid potential compatibility issues
FROM debian:bullseye
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /tinycc
RUN apt-get update && \
apt-get install -y gcc make git && \
git clone https://github.com/TinyCC/tinycc.git . && \
git checkout release_0_9_27 && \
apt-get install -y make libnsl2 libc-devtools gcc liberror-perl libnsl-dev && \
./configure && \
make && \
make test && \
make install
CMD ["/bin/bash"]
