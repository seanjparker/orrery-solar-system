# Orrery
## A simulation of the inner planets written in C and OpenGL

![The spaceship view from the simulation]()

## Compilation
Use the following scripts to compile the `orrery.c` file depending on your OS:

### Mac OS
```bash
gcc -DMACOS -framework OpenGL -framework GLUT -w -o orrery orrery.c
```

### Linux
```bash
gcc -framework OpenGL -framework GLUT -w -o orrery orrery.c
```

## Controls

* Right click (Opens menu)
* W A S D (Move around in spaceship view)
* Arrow keys (Turn around)



