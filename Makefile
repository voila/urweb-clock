all: clock.exe

uw-process/uw_process.o: 
	cd uw-process && make CFLAGS="-I/usr/local/Cellar/urweb/20151122/include/urweb -I."

clock.exe: clock.ur clock.urs clock.urp uw-process/uw_process.o
	urweb clock

clean:
	rm uw-process/uw_process.o
	rm clock.exe
