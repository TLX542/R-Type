#!/bin/bash
# Integration test: Start server, connect client, verify communication

set -e

echo "=== R-Type Server-Client Integration Test ==="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Change to repo root
cd "$(dirname "$0")/.."

# Check if binaries exist
if [ ! -f "./r-type-server" ]; then
    echo -e "${RED}ERROR: r-type-server not found. Please build first.${NC}"
    exit 1
fi

if [ ! -f "./r-type-client" ]; then
    echo -e "${RED}ERROR: r-type-client not found. Please build first.${NC}"
    exit 1
fi

# Start server in background
echo "Starting server on ports TCP:4242 UDP:4243..."
./r-type-server 4242 4243 > /tmp/server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Wait for server to start
sleep 2

# Check if server is running
if ! ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}ERROR: Server failed to start${NC}"
    cat /tmp/server.log
    exit 1
fi

echo -e "${GREEN}✓ Server started successfully${NC}"

# Create a simple test client script
cat > /tmp/test_client.cpp << 'EOF'
#include <iostream>
#include <asio.hpp>
#include <cstring>
#include <thread>
#include <chrono>

struct PacketHeader {
    uint16_t magic{0xABCD};
    uint8_t version{0x01};
    uint8_t payloadSize{0};
    uint8_t type{0};
    uint32_t sessionToken{0};
};

struct PlayerInputPayload {
    uint32_t timestamp;
    uint8_t playerId;
    uint8_t buttons;
    int8_t moveX;
    int8_t moveY;
};

int main() {
    try {
        asio::io_context io_context;
        
        // Connect TCP
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("localhost", "4242");
        asio::ip::tcp::socket tcp_socket(io_context);
        asio::connect(tcp_socket, endpoints);
        
        std::cout << "[Test] TCP connected" << std::endl;
        
        // Send CONNECT
        std::string msg = "CONNECT username=IntegrationTest version=1.0\n";
        asio::write(tcp_socket, asio::buffer(msg));
        
        // Read response
        asio::streambuf response;
        asio::read_until(tcp_socket, response, "\n");
        std::istream response_stream(&response);
        std::string line;
        std::getline(response_stream, line);
        
        std::cout << "[Test] Response: " << line << std::endl;
        
        if (line.find("CONNECT_OK") == 0) {
            std::cout << "[Test] ✓ Connection successful" << std::endl;
            return 0;
        }
        return 1;
    } catch (std::exception& e) {
        std::cerr << "[Test] Error: " << e.what() << std::endl;
        return 1;
    }
}
EOF

# Compile test client
echo "Compiling test client..."
g++ -std=c++17 /tmp/test_client.cpp -o /tmp/test_client -lpthread
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Failed to compile test client${NC}"
    kill $SERVER_PID
    exit 1
fi

# Run test client
echo "Running test client..."
/tmp/test_client
TEST_RESULT=$?

# Check result
if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ Client connection test PASSED${NC}"
else
    echo -e "${RED}✗ Client connection test FAILED${NC}"
fi

# Cleanup
echo ""
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true

# Show server log summary
echo ""
echo "Server log summary (last 20 lines):"
tail -20 /tmp/server.log

echo ""
if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}=== ALL TESTS PASSED ===${NC}"
    exit 0
else
    echo -e "${RED}=== TESTS FAILED ===${NC}"
    exit 1
fi
