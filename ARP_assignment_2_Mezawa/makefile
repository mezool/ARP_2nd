all: masterp dronep keyboard watchdog serverp object target
	
masterp:
	gcc master.c -o build/masterp
dronep:
	gcc drone.c -o build/dronep
keyboard:
	gcc keyboard.c -o build/keyboard
watchdog:
	gcc watchdog.c -o build/watchdog
serverp:
	gcc server.c -o build/serverp -lncurses -lm
object:
	gcc object.c -o build/object
target:
	gcc target.c -o build/target