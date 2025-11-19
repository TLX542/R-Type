##
## EPITECH PROJECT, 2023
## Makefile
## File description:
## Optimized Makefile
##

# Compiler and flags
CXX = g++
CXXFLAGS += -std=c++17 -W -Wall -Wextra -Wundef -pedantic -Wformat=2 -Wunused-function -O2 -march=native
LDFLAGS += -lsfml-graphics -lsfml-window -lsfml-system

# Directories
SRCDIR = $(shell find . -type d -name 'src')
OBJDIR = obj
BINDIR = .

# Include paths
INCLUDES = -I include

# Source and object files
SRC = $(shell find $(SRCDIR) -name '*.cpp')
OBJ = $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRC))

# Binary name
NAME = bs-rtype

# Number of cores for parallel compilation
NPROCS = $(shell nproc)

# Main target
all: $(BINDIR)/$(NAME)

# Linking rule
$(BINDIR)/$(NAME): $(OBJ)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# Compilation rule with automatic dependencies
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files
-include $(OBJ:.o=.d)

# Clean targets
clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(BINDIR)/$(NAME)

re:
	+$(MAKE) fclean
	+$(MAKE) all

.PHONY: all clean fclean re