#!/bin/bash

##
## EPITECH PROJECT, 2024
## R-Type Server
## File description:
## Script to launch multiple clients
##

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
HOST="localhost"
PORT="4242"
NUM_CLIENTS=3

# Help function
show_help() {
    echo -e "${BLUE}Usage: $0 [OPTIONS]${NC}"
    echo ""
    echo -e "${YELLOW}Options:${NC}"
    echo "  -h, --help              Show this help message"
    echo "  -n, --num <number>      Number of clients to launch (default: 3)"
    echo "  -H, --host <hostname>   Server hostname (default: localhost)"
    echo "  -p, --port <port>       Server port (default: 4242)"
    echo ""
    echo -e "${YELLOW}Examples:${NC}"
    echo "  $0                      # Launch 3 clients on localhost:4242"
    echo "  $0 -n 5                 # Launch 5 clients"
    echo "  $0 -n 10 -p 8080        # Launch 10 clients on port 8080"
    echo ""
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -n|--num)
            NUM_CLIENTS="$2"
            shift 2
            ;;
        -H|--host)
            HOST="$2"
            shift 2
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Check if client_test exists
if [ ! -f "./client_test" ]; then
    echo -e "${RED}Error: client_test executable not found!${NC}"
    echo -e "${YELLOW}Please compile first with: make${NC}"
    exit 1
fi

# Validate number of clients
if ! [[ "$NUM_CLIENTS" =~ ^[0-9]+$ ]] || [ "$NUM_CLIENTS" -lt 1 ]; then
    echo -e "${RED}Error: Number of clients must be a positive integer${NC}"
    exit 1
fi

if [ "$NUM_CLIENTS" -gt 100 ]; then
    echo -e "${YELLOW}Warning: Launching more than 100 clients may be resource intensive${NC}"
    read -p "Continue? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
fi

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     R-Type Multiple Clients Launcher   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}Configuration:${NC}"
echo -e "  Host:         ${GREEN}$HOST${NC}"
echo -e "  Port:         ${GREEN}$PORT${NC}"
echo -e "  Clients:      ${GREEN}$NUM_CLIENTS${NC}"
echo ""

# Array to store PIDs
declare -a PIDS=()

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}Stopping all clients...${NC}"
    for pid in "${PIDS[@]}"; do
        if ps -p $pid > /dev/null 2>&1; then
            kill $pid 2>/dev/null
        fi
    done
    echo -e "${GREEN}All clients stopped${NC}"
    exit 0
}

# Trap Ctrl+C
trap cleanup SIGINT SIGTERM

# Function to send messages from a client
run_client() {
    local client_id=$1
    local color=$2

    {
        # Wait a bit for connection to establish
        sleep 0.5

        # Send initial message
        echo "Hello from client #$client_id"
        sleep 1

        # Send periodic messages
        for i in {1..5}; do
            echo "Client #$client_id - Message $i"
            sleep $((2 + RANDOM % 3))  # Random delay between 2-4 seconds
        done

        echo "Client #$client_id - Goodbye!"
        sleep 1
    } | ./client_test $HOST $PORT 2>&1 | while IFS= read -r line; do
        echo -e "${color}[Client #$client_id]${NC} $line"
    done
}

# Launch clients
echo -e "${YELLOW}Launching $NUM_CLIENTS clients...${NC}"
echo ""

# Array of colors for different clients
COLORS=("$RED" "$GREEN" "$YELLOW" "$BLUE" "$MAGENTA" "$CYAN")

for ((i=1; i<=NUM_CLIENTS; i++)); do
    color_index=$((($i - 1) % ${#COLORS[@]}))
    color="${COLORS[$color_index]}"

    run_client $i "$color" &
    PIDS+=($!)

    echo -e "${GREEN}✓${NC} Client #$i launched (PID: ${PIDS[$((i-1))]})"

    # Small delay between launches to avoid overwhelming the server
    sleep 0.2
done

echo ""
echo -e "${GREEN}All clients launched successfully!${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop all clients${NC}"
echo ""
echo -e "${CYAN}Active clients: $NUM_CLIENTS${NC}"
echo ""

# Wait for all clients to finish
for pid in "${PIDS[@]}"; do
    wait $pid 2>/dev/null
done

echo ""
echo -e "${GREEN}All clients have finished${NC}"
