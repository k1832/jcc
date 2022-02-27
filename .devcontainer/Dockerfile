FROM --platform=linux/amd64 ubuntu:latest
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive apt install -y gcc make git binutils libc6-dev gdb sudo python3-pip wget
RUN wget -O ~/vsls-reqs https://aka.ms/vsls-linux-prereq-script && chmod +x ~/vsls-reqs && ~/vsls-reqs

# Install cpplint
RUN pip install cpplint
RUN echo "export PATH=$PATH:/home/user/.local/bin" >> ~/.bashrc

RUN adduser --disabled-password --gecos '' user
RUN echo 'user ALL=(root) NOPASSWD:ALL' > /etc/sudoers.d/user
USER user
WORKDIR /home/user
