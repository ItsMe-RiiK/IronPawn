FROM ubuntu:22.04

# Prevent interactive prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary compilation tools and Stockfish engine
RUN apt-get update && apt-get install -y \
    stockfish \
    cmake \
    g++ \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /app

# Copy all project files into the container
COPY . .

# Compile the C++ bot
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Expose the Local Web Server port so Koyeb knows where to send web traffic
EXPOSE 8080

# When the container starts, run the bot!
CMD ["./build/IronPawn"]
