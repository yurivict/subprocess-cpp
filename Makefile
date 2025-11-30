# Makefile for subprocess-cpp

CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDFLAGS ?=

PREFIX ?= /usr/local
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib

LIB_NAME = subprocess-cpp
STATIC_LIB = lib$(LIB_NAME).a
SHARED_LIB = lib$(LIB_NAME).so

SRCS = subprocess-cpp.cpp
OBJS = subprocess-cpp.o
OBJS_PIC = subprocess-cpp.pic.o

HEADERS = subprocess-cpp.h deps/subprocess.h/subprocess.h

# Test configuration
TEST_DIR = tests
TEST_SRC = $(TEST_DIR)/test_subprocess.cpp
TEST_BIN = $(TEST_DIR)/test_subprocess
GTEST_CFLAGS != pkg-config --cflags gtest 2>/dev/null || true
GTEST_LIBS != pkg-config --libs gtest gtest_main 2>/dev/null || echo "-lgtest -lgtest_main -pthread"

.PHONY: all clean install uninstall test

all: $(STATIC_LIB) $(SHARED_LIB)

# Static library
$(STATIC_LIB): $(OBJS)
	$(AR) rcs $@ $^

# Shared library
$(SHARED_LIB): $(OBJS_PIC)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

# Object files for static library
subprocess-cpp.o: subprocess-cpp.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. -Ideps/subprocess.h -c subprocess-cpp.cpp -o subprocess-cpp.o

# PIC object files for shared library
subprocess-cpp.pic.o: subprocess-cpp.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -fPIC -I. -Ideps/subprocess.h -c subprocess-cpp.cpp -o subprocess-cpp.pic.o

# Install targets
install: $(STATIC_LIB) $(SHARED_LIB)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -m 644 subprocess-cpp.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 deps/subprocess.h/subprocess.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/
	install -m 755 $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/

uninstall:
	rm -f $(DESTDIR)$(INCLUDEDIR)/subprocess-cpp.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/subprocess.h
	rm -f $(DESTDIR)$(LIBDIR)/$(STATIC_LIB)
	rm -f $(DESTDIR)$(LIBDIR)/$(SHARED_LIB)

# Test target
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -I. -Ideps/subprocess.h -o $@ $(TEST_SRC) $(STATIC_LIB) $(GTEST_LIBS) -lpthread

clean:
	rm -f $(OBJS) $(OBJS_PIC) $(STATIC_LIB) $(SHARED_LIB) $(TEST_BIN)
