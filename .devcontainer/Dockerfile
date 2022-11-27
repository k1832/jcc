FROM --platform=linux/amd64 ubuntu:focal-20220415
RUN apt update

RUN DEBIAN_FRONTEND=noninteractive apt install -y gcc make git binutils libc6-dev gdb sudo python3-pip clangd-9 bear
RUN sudo update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-9 100

# Install cpplint
RUN pip install cpplint
RUN echo "export PATH=$PATH:/home/user/.local/bin" >> ~/.bashrc

RUN adduser --disabled-password --gecos '' user
RUN echo 'user ALL=(root) NOPASSWD:ALL' > /etc/sudoers.d/user
USER user
WORKDIR /home/user
