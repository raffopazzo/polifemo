bin_PROGRAMS=test

test_SOURCES=src/main.cpp src/mongoose.c

test_CXXFLAGS=
test_CXXFLAGS+=--std=c++11
test_CXXFLAGS+=$(opencv_CFLAGS)

test_LDADD=
test_LDADD+=-lpthread
test_LDADD+=$(opencv_LIBS)

