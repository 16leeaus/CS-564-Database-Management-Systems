##############################################################
#               CMake Project Wrapper Makefile               #
############################################################## 
CC = g++
CFLAGS = -std=c++14 -g -Wall

all:
	cd src;\
	$(CC) $(CFLAGS) *.cpp exceptions/*.cpp -I. -o badgerdb_main
clean:
	@cd src;\
	rm -f badgerdb_main test.?

test:
	@echo "BUILDING..."
	@cd src;\
	rm -f test.?
	@cd src;\
	$(CC) $(CFLAGS) *.cpp exceptions/*.cpp -I. -o badgerdb_main
	@echo "BUILD COMPLETE\n\nTESTING...\n----------------------------------\n"
	@./src/badgerdb_main
	@echo "\n----------------------------------\nTEST COMPLETE\n"
	@cd src;\
	rm -f badgerdb_main test.?

format:
	find . \( -iname '*.h' -o -iname '*.cpp' \) -exec clang-format -style=Google -i {} \;

docs:
	doxygen Doxyfile
