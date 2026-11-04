##
## EPITECH PROJECT, 2026
## G-PDG-300-LIL-3-1-PDGD04-51
## File description:
## Makefile
##

# FILES
SRC_LOGIC = src/Server.cpp \
            src/Client.cpp

SRC_MAIN  = src/main.cpp

TEST_SRC  = tests/test_ftp.cpp

SRC       = $(SRC_LOGIC) $(SRC_MAIN)

# COLORS
RED=\033[31m
GREEN=\033[32m
WHITE=\033[37m
BOLD=\033[1m
RESET=\033[0m

# VARIABLES
OBJ	=	$(SRC:.cpp=.o)
OBJ_DIR = tests/obj/
NAME = myftp
TEST_NAME = unit_tests

# COMPILER
CC       = g++
CPPFLAGS = -I./src 
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror
TFLAGS   = -lcriterion --coverage

# PRINT COMPILATION
%.o: %.cpp
	@echo "$(GREEN)$(BOLD)[COMPILING]:\t$(RESET)$(GREEN)$<$(RESET)"
	@$(CC) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# PRINT AND THEN COMPILE
all: pre-compile $(NAME)

# PRINT INFO
pre-compile:
	@echo "$(WHITE)$(BOLD)\t[COMPILATION]$(RESET)"

# LINKING
$(NAME): $(OBJ)
	@$(CC) $(OBJ) -o $(NAME)
	@echo "$(WHITE)$(BOLD)[READY]:$(RESET)$(WHITE)\t$(NAME)$(RESET)"

# TESTS
# tests_run: fclean
# 	@$(CC) -o $(TEST_NAME) $(SRC_LOGIC) $(TEST_SRC) $(CXXFLAGS) $(CPPFLAGS) $(TFLAGS)
# 	@./$(TEST_NAME) --color=always
# 	@rm -f -- *.gcno *.gcda $(TEST_NAME)

# CLEAN OBJECT FILES
clean:
	@rm -f -- *.gcno *.gcda $(TEST_NAME)
	@found=0; \
	for file in $(OBJ); do \
		if [ -f "$$file" ]; then \
			found=1; \
			break; \
		fi; \
	done; \
	if [ $$found -eq 1 ]; then \
		echo "$(WHITE)$(BOLD)\t[CLEANING OBJ FILES]$(RESET)"; \
		for file in $(OBJ); do \
			if [ -f "$$file" ]; then \
				echo "$(RED)$(BOLD)[REMOVED]:\t$(RESET)$(RED)$$file$(RESET)"; \
				rm -f "$$file"; \
			fi; \
		done; \
	fi


# CLEAN BINARY AND OBJECT FILES
fclean:
	@if [ -f "$(NAME)" ]; then \
		echo "$(WHITE)$(BOLD)\t[CLEANING BINARY]$(RESET)"; \
		echo "$(RED)$(BOLD)[REMOVED]:\t$(RESET)$(RED)$(NAME)$(RESET)"; \
		rm -f $(NAME); \
	fi
	@$(MAKE) clean -s


# CLEAN EVERYTHING AND RE-COMPILE
re: fclean all
