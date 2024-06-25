# Use a lightweight base image
FROM alpine:3.14 as builder

# Install build dependencies
RUN apk add --no-cache \
    g++ \
    make \
    cmake

# Set the working directory in the container
WORKDIR /app

# Copy the source code
COPY . .

# Compile the server
RUN g++ -std=c++11 -pthread main.cpp -o server

# Use a smaller runtime image
FROM alpine:3.14

# Install runtime dependencies
RUN apk add --no-cache libstdc++

# Set the working directory in the container
WORKDIR /app

# Copy the compiled server from the builder stage
COPY --from=builder /app/server .

# Expose the port the server runs on
EXPOSE 42069

# Run the server
CMD ["./server"]