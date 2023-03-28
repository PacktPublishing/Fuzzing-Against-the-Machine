all: sysc_fuzz

CC=xcrun -sdk iphoneos clang -arch arm64
#CC=xcrun --sdk iphoneos clang -arch arm64 -isysroot $(xcrun --sdk iphoneos --show-sdk-path)
OBJS=sysc.o sysc_fuzz.o parse.o argfd.o fuzz.o

sysc_fuzz: $(OBJS)
	$(CC) -o $@ $(OBJS)
	codesign -f -s - --entitlements ent.xml sysc_fuzz

#sysc_fuzz: sysc_fuzz.c aflCall.c sysc.c parse.c argfd.c fuzz.S fuzz.h drv.h sysc.h
#	xcrun -sdk iphoneos clang -arch arm64 -o sysc_fuzz aflCall.c sysc.c parse.c argfd.c fuzz.S 
#	codesign -f -s - --entitlements ent.xml sysc_fuzz

sock_fuzz: sock_fuzz.c fuzz.S fuzz.h
	xcrun -sdk iphoneos -sdk clang -arch arm64 -o sock_fuzz sock_fuzz.c fuzz.S
	codesign -f -s - --entitlements ent.xml sock_fuzz
