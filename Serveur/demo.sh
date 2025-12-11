#!/bin/bash
# Demo script for R-Type Server-Client Architecture
# This script builds and runs the server and multiple clients for demonstration

set -e  # Exit on error

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Configuration
TCP_PORT=4242
UDP_PORT=4243
HOST=localhost

echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     R-Type Server-Client Demo Script              ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    echo -e "${RED}Error: Must be run from Serveur directory${NC}"
    exit 1
fi

# Step 1: Build
echo -e "${YELLOW}[1/4] Building server and clients...${NC}"
make clean > /dev/null 2>&1 || true
if make > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    echo -e "${YELLOW}Trying to show errors:${NC}"
    make
    exit 1
fi
echo ""

# Check if binaries exist
if [ ! -f "./r-type_server" ]; then
    echo -e "${RED}✗ Server binary not found${NC}"
    exit 1
fi

if [ ! -f "./render_client" ]; then
    echo -e "${YELLOW}⚠ Render client not found (SFML may not be installed)${NC}"
    echo -e "${YELLOW}  Continuing with server only...${NC}"
    HAS_RENDER_CLIENT=false
else
    HAS_RENDER_CLIENT=true
fi

# Step 2: Start Server
echo -e "${YELLOW}[2/4] Starting server on ports ${TCP_PORT}/${UDP_PORT}...${NC}"
./r-type_server $TCP_PORT $UDP_PORT > /tmp/rtype_server.log 2>&1 &
SERVER_PID=$!
sleep 2

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}✗ Server failed to start${NC}"
    echo -e "${YELLOW}Server log:${NC}"
    cat /tmp/rtype_server.log
    exit 1
fi

echo -e "${GREEN}✓ Server started (PID: $SERVER_PID)${NC}"
echo ""

# Step 3: Show server info
echo -e "${YELLOW}[3/4] Server Information${NC}"
echo -e "  TCP Port: ${GREEN}$TCP_PORT${NC} (for connections)"
echo -e "  UDP Port: ${GREEN}$UDP_PORT${NC} (for gameplay)"
echo -e "  Log file: ${GREEN}/tmp/rtype_server.log${NC}"
echo ""

# Step 4: Instructions
echo -e "${YELLOW}[4/4] Demo Instructions${NC}"
echo ""

if [ "$HAS_RENDER_CLIENT" = true ]; then
    echo -e "${GREEN}Graphical Client Available!${NC}"
    echo -e "To start a client in a new terminal:"
    echo -e "  ${BLUE}cd $(pwd)${NC}"
    echo -e "  ${BLUE}./render_client $HOST $TCP_PORT${NC}"
    echo ""
    echo -e "Controls:"
    echo -e "  ${GREEN}Arrow Keys${NC} - Move your square"
    echo -e "  ${GREEN}Space${NC}      - Shoot (not implemented yet)"
    echo -e "  ${GREEN}ESC${NC}        - Quit"
    echo ""
else
    echo -e "${YELLOW}Graphical client not available (SFML not installed)${NC}"
    echo -e "You can still test with the terminal client:"
    echo -e "  ${BLUE}cd $(pwd)${NC}"
    echo -e "  ${BLUE}./game_client $HOST $TCP_PORT${NC}"
    echo ""
fi

echo -e "To test multiple players:"
echo -e "  1. Open ${GREEN}4 different terminals${NC}"
echo -e "  2. Run the client command in each"
echo -e "  3. Each player gets a different color"
echo -e "  4. All players see each other move in real-time!"
echo ""

echo -e "${BLUE}════════════════════════════════════════════════════${NC}"
echo -e "Server is running. Press ${RED}Ctrl+C${NC} to stop."
echo -e "${BLUE}════════════════════════════════════════════════════${NC}"
echo ""

# Function to cleanup on exit
cleanup() {
    echo ""
    echo -e "${YELLOW}Shutting down...${NC}"
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    echo -e "${GREEN}✓ Server stopped${NC}"
    echo -e "${BLUE}Demo complete. Thank you!${NC}"
}

trap cleanup EXIT INT TERM

# Show live server logs
echo -e "${GREEN}Server Output:${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────${NC}"
tail -f /tmp/rtype_server.log
