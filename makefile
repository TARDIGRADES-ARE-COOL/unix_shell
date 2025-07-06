CC = gcc
SRC_DIR = ./source/system_programs
BIN_DIR = ./bin
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%)
MAIN_SRC = ./source/shell.c # add more source files here
MAIN_EXEC = cseshell

# Special rule for main executable
all: $(OBJECTS) $(MAIN_EXEC)

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $< -o $@

# $< refers to the first dependency, here it is ./source/shell.c
# if you have more dependencies, use $^ instead 
# $@: This variable represents the target of the rule
# It is the filename of the file that is being generated or updated by the rule, e.g: MAIN_EXEC (cseshell)
$(MAIN_EXEC): $(MAIN_SRC)
	$(CC) $< -o $@

clean:
	rm -f $(OBJECTS) $(MAIN_EXEC)
