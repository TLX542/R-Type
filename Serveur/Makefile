##
## EPITECH PROJECT, 2024
## R-Type Server
## File description:
## Makefile
##

# Compiler and flags
CXX			=	g++
CXXFLAGS	=	-std=c++17 -Wall -Wextra -I./include

# Libraries
LDFLAGS		=	-lpthread

# Check for ASIO standalone or Boost.Asio
ASIO_STANDALONE := $(shell echo '\#include <asio.hpp>' | $(CXX) -E -x c++ - >/dev/null 2>&1 && echo yes)

ifeq ($(ASIO_STANDALONE),yes)
    $(info Using standalone ASIO)
    CXXFLAGS += -DASIO_STANDALONE
else
    $(info Trying Boost.Asio)
    BOOST_CHECK := $(shell echo '\#include <boost/asio.hpp>' | $(CXX) -E -x c++ - >/dev/null 2>&1 && echo yes)
    ifeq ($(BOOST_CHECK),yes)
        $(info Using Boost.Asio)
        LDFLAGS += -lboost_system
    else
        $(error Neither ASIO standalone nor Boost.Asio found. Install with: sudo apt-get install libasio-dev)
    endif
endif

# Directories
SRC_DIR		=	src
OBJ_DIR		=	obj
INC_DIR		=	include

# Server files
SERVER_SRC	=	$(SRC_DIR)/Server.cpp \
				$(SRC_DIR)/Protocol.cpp \
				$(SRC_DIR)/main.cpp

SERVER_OBJ	=	$(SERVER_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
SERVER_BIN	=	r-type_server

# Client test files (old simple client)
CLIENT_SRC	=	$(SRC_DIR)/client_test.cpp
CLIENT_OBJ	=	$(CLIENT_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
CLIENT_BIN	=	client_test

# Protocol test client (new)
TEST_CLIENT_SRC	=	$(SRC_DIR)/test_client.cpp \
					$(SRC_DIR)/Protocol.cpp
TEST_CLIENT_OBJ	=	$(TEST_CLIENT_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TEST_CLIENT_BIN	=	protocol_test

# Interactive client (game client)
INTERACTIVE_SRC	=	$(SRC_DIR)/interactive_client.cpp \
					$(SRC_DIR)/Protocol.cpp
INTERACTIVE_OBJ	=	$(INTERACTIVE_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
INTERACTIVE_BIN	=	game_client

# Colors
GREEN		=	\033[0;32m
RED			=	\033[0;31m
BLUE		=	\033[0;34m
YELLOW		=	\033[0;33m
NC			=	\033[0m

# Rules
all: $(SERVER_BIN) $(CLIENT_BIN) $(TEST_CLIENT_BIN) $(INTERACTIVE_BIN)
	@echo "$(GREEN)✓ Build complete!$(NC)"
	@echo "$(BLUE)Server: ./$(SERVER_BIN) <tcp_port> <udp_port>$(NC)"
	@echo "$(BLUE)Game Client: ./$(INTERACTIVE_BIN) <host> <tcp_port>$(NC)"
	@echo "$(BLUE)Protocol Test: ./$(TEST_CLIENT_BIN) <host> <tcp_port>$(NC)"

$(SERVER_BIN): $(SERVER_OBJ)
	@echo "$(YELLOW)Linking $(SERVER_BIN)...$(NC)"
	@$(CXX) $(SERVER_OBJ) -o $(SERVER_BIN) $(LDFLAGS)
	@echo "$(GREEN)✓ $(SERVER_BIN) created$(NC)"

$(CLIENT_BIN): $(CLIENT_OBJ)
	@echo "$(YELLOW)Linking $(CLIENT_BIN)...$(NC)"
	@$(CXX) $(CLIENT_OBJ) -o $(CLIENT_BIN) $(LDFLAGS)
	@echo "$(GREEN)✓ $(CLIENT_BIN) created$(NC)"

$(TEST_CLIENT_BIN): $(TEST_CLIENT_OBJ)
	@echo "$(YELLOW)Linking $(TEST_CLIENT_BIN)...$(NC)"
	@$(CXX) $(TEST_CLIENT_OBJ) -o $(TEST_CLIENT_BIN) $(LDFLAGS)
	@echo "$(GREEN)✓ $(TEST_CLIENT_BIN) created$(NC)"

$(INTERACTIVE_BIN): $(INTERACTIVE_OBJ)
	@echo "$(YELLOW)Linking $(INTERACTIVE_BIN)...$(NC)"
	@$(CXX) $(INTERACTIVE_OBJ) -o $(INTERACTIVE_BIN) $(LDFLAGS)
	@echo "$(GREEN)✓ $(INTERACTIVE_BIN) created$(NC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "$(BLUE)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files...$(NC)"
	@rm -rf $(OBJ_DIR)
	@echo "$(GREEN)✓ Clean complete$(NC)"

fclean: clean
	@echo "$(RED)Cleaning binaries...$(NC)"
	@rm -f $(SERVER_BIN) $(CLIENT_BIN) $(TEST_CLIENT_BIN) $(INTERACTIVE_BIN)
	@echo "$(GREEN)✓ Full clean complete$(NC)"

re: fclean all

# Test targets
test_server: $(SERVER_BIN)
	@echo "$(YELLOW)Starting server on ports TCP:4242 UDP:4243...$(NC)"
	./$(SERVER_BIN) 4242 4243

test_client: $(CLIENT_BIN)
	@echo "$(YELLOW)Connecting to localhost:4242...$(NC)"
	./$(CLIENT_BIN) localhost 4242

test_protocol: $(TEST_CLIENT_BIN)
	@echo "$(YELLOW)Testing protocol with localhost:4242...$(NC)"
	./$(TEST_CLIENT_BIN) localhost 4242

# Help
help:
	@echo "$(BLUE)R-Type Server Makefile$(NC)"
	@echo ""
	@echo "$(YELLOW)Available targets:$(NC)"
	@echo "  make           - Build server and clients"
	@echo "  make clean     - Remove object files"
	@echo "  make fclean    - Remove object files and binaries"
	@echo "  make re        - Rebuild everything"
	@echo "  make test_server   - Build and run server on ports 4242/4243"
	@echo "  make test_client   - Build and run old test client"
	@echo "  make test_protocol - Build and run protocol test client"
	@echo "  make help          - Display this help"
	@echo ""
	@echo "$(YELLOW)Manual usage:$(NC)"
	@echo "  ./$(SERVER_BIN) <tcp_port> <udp_port>"
	@echo "  ./$(TEST_CLIENT_BIN) <host> <tcp_port>"

.PHONY: all clean fclean re test_server test_client test_protocol help
