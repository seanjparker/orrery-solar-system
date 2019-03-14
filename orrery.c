/*
==========================================================================
File:        orrery.c
Author:     Sean Parker
==========================================================================
*/

/* The following ratios are not to scale: */
/* Moon orbit : planet orbit */
/* Orbit radius : body radius */
/* Sun radius : planet radius */

#ifdef MACOS
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOLARSYS_SIZE 600000000
#define MAX_STARS 5000
#define MAX_BODIES 25
#define TOP_VIEW 1
#define ECLIPTIC_VIEW 2
#define SHIP_VIEW 3
#define EARTH_VIEW 4
#define PI 3.141593
#define DEG_TO_RAD 0.01745329
#define ORBIT_POLY_SIDES 50
#define TIME_STEP 0.25 /* days per frame */
#define EARTH 3
#define TURN_ANGLE 4.0
#define RUN_SPEED  10000000

typedef struct
{
  char name[25];          /* name */
  GLfloat r, g, b;        /* colour */
  GLfloat orbital_radius; /* distance to parent body (km) */
  GLfloat orbital_tilt;   /* angle of orbit wrt ecliptic (deg) */
  GLfloat orbital_period; /* time taken to orbit (days) */
  GLfloat radius;         /* radius of body (km) */
  GLfloat axis_tilt;      /* tilt of axis wrt body's orbital plane (deg) */
  GLfloat rot_period;     /* body's period of rotation (days) */
  GLint orbits_body;      /* identifier of parent body */
  GLfloat spin;           /* current spin value (deg) */
  GLfloat orbit;          /* current orbit value (deg) */
} body;

typedef struct
{
  float x[MAX_STARS], y[MAX_STARS], z[MAX_STARS];
} starfield;

GLdouble lat,     lon;              /* View angles (degrees)    */
GLfloat  eyex,    eyey,    eyez;    /* Eye point                */
GLfloat  centerx, centery, centerz; /* Look point               */
GLfloat  upx,     upy,     upz;     /* View up vector           */

body bodies[MAX_BODIES];
int numBodies, current_view, draw_orbits, draw_labels, draw_starfield, draw_axes;
float date;

starfield *mystarfield;

/*****************************/

float myRand(void)
{
  /* return a random float in the range [0,1] */
  return (float)rand() / RAND_MAX;
}

float myRandStarfield(void)
{
  /* return a random float in the range [-1,1] */
  return -1 + 2 * ((float)rand()) / RAND_MAX;
}

//////////////////////////////////////////////
/* Given an eyepoint and latitude and longitude angles, will
 compute a look point one unit away */
void calculate_lookpoint() {
  // Intermediate values for the distance moved in each direction
  GLfloat dir_x, dir_y, dir_z;

  // Calculate the new latitude and bound
  if (lat >= 89.5) lat = 89.5;
  else if (lat <= -89.5) lat = -89.5;

  // Convert the latitude and longitude to radians
  float lat_Rads = lat * DEG_TO_RAD;
  float lon_Rads = lon * DEG_TO_RAD;

  // Calculate the distance moved
  dir_x = cos(lat_Rads) * sin(lon_Rads);
  dir_y = sin(lat_Rads);
  dir_z = cos(lat_Rads) * cos(lon_Rads);

  // Update centre in each axis
  centerx = eyex + (dir_x * SOLARSYS_SIZE);
  centery = eyey + (dir_y * SOLARSYS_SIZE);
  centerz = eyez + (dir_z * SOLARSYS_SIZE);
}

/*****************************/

void drawStarfield()
{
  glColor3f(1.0, 1.0, 1.0);
  glBegin(GL_POINTS);
  int i;
  for (i = 0; i < 1000; ++i)
    glVertex3f(
      mystarfield->x[i],
      mystarfield->y[i],
      mystarfield->z[i]);
  glEnd();
}

/*****************************/

void readSystem(void)
{
  /* reads in the description of the solar system */

  FILE *f;
  int i;

  f = fopen("sys", "r");
  if (f == NULL)
  {
    printf("ex2.c: Couldn't open 'sys'\n");
    printf("To get this file, use the following command:\n");
    printf("  cp /opt/info/courses/COMP27112/ex2/sys .\n");
    exit(0);
  }
  fscanf(f, "%d", &numBodies);
  for (i = 0; i < numBodies; i++)
  {
    fscanf(f, "%s %f %f %f %f %f %f %f %f %f %d",
      bodies[i].name,
      &bodies[i].r, &bodies[i].g, &bodies[i].b,
      &bodies[i].orbital_radius,
      &bodies[i].orbital_tilt,
      &bodies[i].orbital_period,
      &bodies[i].radius,
      &bodies[i].axis_tilt,
      &bodies[i].rot_period,
      &bodies[i].orbits_body);

    /* Initialise the body's state */
    bodies[i].spin = 0.0;
    bodies[i].orbit = myRand() * 360.0; /* Start each body's orbit at a
                                          random angle */
    bodies[i].radius *= 1000.0;         /* Magnify the radii to make them visible */
  }
  fclose(f);
}

/*****************************/

void drawString(void *font, float x, float y, char *str)
{ /* Displays the string "str" at (x,y,0), using font "font" */
  char *ch;
  glRasterPos3f(x, y, 0.0);
  for (ch = str; *ch; ch++)
  {
    glutBitmapCharacter(font, (int)*ch);
  }
}

void drawAxes(void)
{
  // Draws X Y and Z axis lines, of length LEN
  float LEN = 20000000.0;

  glLineWidth(5.0);
  glBegin(GL_LINES);
  {
    glColor3f(1.0, 0.0, 0.0); // red
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(LEN, 0.0, 0.0);

    glColor3f(0.0, 1.0, 0.0); // green
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, LEN, 0.0);

    glColor3f(0.0, 0.0, 1.0); // blue
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, LEN);
  }
  glEnd();

  glLineWidth(1.0);
}

/*****************************/

void setView(void)
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  switch (current_view)
  {
  case TOP_VIEW:
    gluLookAt(0.0, 400000000.0, 0.0,
              0.0, 0.0, 0.0,
              0.0, 0.0, 1.0);
    break;
  case ECLIPTIC_VIEW:
    gluLookAt(0.0, 0.0, 400000000.0,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);
    break;
  case SHIP_VIEW:
    calculate_lookpoint();
    gluLookAt(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
    break;
  case EARTH_VIEW:
    {
      float earthPlanetOrbit = bodies[EARTH].orbit * DEG_TO_RAD;
      gluLookAt(bodies[EARTH].orbital_radius * 1.25 * sin(earthPlanetOrbit), // eyeX
                bodies[EARTH].radius * 1.25,                                 // eyeY
                bodies[EARTH].orbital_radius * 1.25 * cos(earthPlanetOrbit), // eyeZ
                0, 0, 0, 0, 1.0, 0);
      break;
    }
  }
}

/*****************************/

void menu(int menuentry)
{
  switch (menuentry)
  {
  case 1:
    current_view = TOP_VIEW;
    break;
  case 2:
    current_view = ECLIPTIC_VIEW;
    break;
  case 3:
    current_view = SHIP_VIEW;
    break;
  case 4:
    current_view = EARTH_VIEW;
    break;
  case 5:
    draw_labels = !draw_labels;
    break;
  case 6:
    draw_orbits = !draw_orbits;
    break;
  case 7:
    draw_starfield = !draw_starfield;
    break;
  case 8:
    exit(0);
  }
}

/*****************************/

void init(void)
{
  /* Define background colour */
  glClearColor(0.0, 0.0, 0.0, 0.0);

  glutCreateMenu(menu);
  glutAddMenuEntry("Top view", 1);
  glutAddMenuEntry("Ecliptic view", 2);
  glutAddMenuEntry("Spaceship view", 3);
  glutAddMenuEntry("Earth view", 4);
  glutAddMenuEntry("", 999);
  glutAddMenuEntry("Toggle labels", 5);
  glutAddMenuEntry("Toggle orbits", 6);
  glutAddMenuEntry("Toggle starfield", 7);
  glutAddMenuEntry("", 999);
  glutAddMenuEntry("Quit", 8);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  current_view = TOP_VIEW;
  draw_labels = 1;
  draw_orbits = 1;
  draw_starfield = 1;
  draw_axes = 1;

  eyex = 100000000.0;
  eyey = 200000000.0;
  eyez = -400000000.0;

  centerx = 0.0;
  centery = 0.0;
  centerz = 0.0;

  upx = 0.0;
  upy = 1.0;
  upz = 0.0;

  lat = -20.0;
  lon = -20.0;

  // Create the starfield array
  mystarfield = (starfield *)malloc(sizeof(starfield));
  int i;
  for (i = 0; i < 1000; ++i)
  {
    mystarfield->x[i] = myRandStarfield() * SOLARSYS_SIZE;
    mystarfield->y[i] = myRandStarfield() * SOLARSYS_SIZE;
    mystarfield->z[i] = myRandStarfield() * SOLARSYS_SIZE;
  }
}

/*****************************/

void animate(void)
{
  int i;

  date += TIME_STEP;

  for (i = 0; i < numBodies; i++)
  {
    bodies[i].spin += 360.0 * TIME_STEP / bodies[i].rot_period;
    bodies[i].orbit += 360.0 * TIME_STEP / bodies[i].orbital_period;
    glutPostRedisplay();
  }
}

/*****************************/

void drawOrbit(int n)
{ /* Draws a polygon to approximate the circular
     orbit of body "n" */
  body planet = bodies[n];
  float theta = 0.0;
  float moveAngle = 2 * PI / (float) ORBIT_POLY_SIDES;
  float x, z;
  glColor3f(bodies[n].r, bodies[n].g, bodies[n].b);
  glBegin(GL_LINE_LOOP);
  int i;
  for (i = 0; i < ORBIT_POLY_SIDES; i++)
  {
    x = cos(theta) * planet.orbital_radius;
    z = sin(theta) * planet.orbital_radius;
    glVertex3f(x, 0.0, z);

    theta += moveAngle;
  }
  glEnd();
  glColor3f(1.0, 1.0, 1.0);
}

/*****************************/

void drawLabel(int n)
{ /* Draws the name of body "n" */
  drawString(GLUT_BITMAP_HELVETICA_18, 0.0, bodies[n].radius * 2, bodies[n].name);
}

/*****************************/

void drawBody(int n)
{
  body *planet = &bodies[n];
  float planetOrbit = planet->orbit * DEG_TO_RAD;
  float parentPlanetOrbit = bodies[planet->orbits_body].orbit * DEG_TO_RAD;

  // Draws body "n"
  GLint sl = 17;
  GLint st = 13;

  if (planet->orbits_body == 0) {
    glRotatef(planet->orbital_tilt, 0.0, 0.0, 1.0); //T_otilt
  } else {
    glRotatef(bodies[planet->orbits_body].orbital_tilt, 0.0, 0.0, 1.0); //T_otilt
  }

  // If a planet's parent is not the sun, then translate to the parent body
  if (planet->orbits_body != 0)
  {
    glTranslatef(
      bodies[planet->orbits_body].orbital_radius * sin(parentPlanetOrbit),
      0.0,
      bodies[planet->orbits_body].orbital_radius * cos(parentPlanetOrbit));
  }

  if (draw_orbits)
    drawOrbit(n);

  glTranslatef(planet->orbital_radius * sin(planetOrbit), 0.0, planet->orbital_radius * cos(planetOrbit));  //T_orb

  if (draw_labels)
    drawLabel(n);

  glRotatef(planet->axis_tilt, 0.0, 0.0, -1.0);  //T_atilt

  glRotatef(planet->spin, 0.0, 1.0, 0.0);       //T_spin

  glRotatef(90, 1.0, 0.0, 0.0);                 //T_vert

  // Draw the body
  glColor3f(planet->r, planet->g, planet->b);
  glutWireSphere(planet->radius, sl, st);

  // Draw the axis
  float axisLength = planet->radius;
  glLineWidth(2.5);
  glBegin(GL_LINES);
  {
    glColor3f(1.0, 1.0, 1.0); // white
    glVertex3f(0.0, 0.0, -planet->radius - axisLength);
    glVertex3f(0.0, 0.0, planet->radius + axisLength);
  }
  glEnd();
  glLineWidth(1.0);
}

/*****************************/

void display(void)
{
  int i;

  glClear(GL_COLOR_BUFFER_BIT);

  if (draw_axes)
    drawAxes();

  /* set the camera */
  setView();

  if (draw_starfield)
    drawStarfield();

  for (i = 0; i < numBodies; i++)
  {
    glPushMatrix();
    drawBody(i);
    glPopMatrix();
  }

  glutSwapBuffers();
}

/*****************************/

void reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(48.0, (GLfloat)w / (GLfloat)h, 10000.0, 800000000.0);
}

/*****************************/

void keyboard(unsigned char key, int x, int y)
{
  switch (key)
  {
  case 114: // r, move up
    eyey += RUN_SPEED;
    break;
  case 102: // f, move down
    eyey -= RUN_SPEED;
    break;
  case 119: // w, move forward
    // Moved forward in the direction we are pointing
    eyex += RUN_SPEED * sin(lon * DEG_TO_RAD);
    eyez += RUN_SPEED * cos(lon * DEG_TO_RAD);
    break;
  case 115: // s, move backward
    // Moved forward in the opposite direction we are pointing
    eyex -= RUN_SPEED * sin(lon * DEG_TO_RAD);
    eyez -= RUN_SPEED * cos(lon * DEG_TO_RAD);
    break;
  case 97: // letter a, toggle draw_axes
    draw_axes ^= 1;
    break;
  case 27: /* Escape key */
    free(mystarfield);
    exit(0);
  }
}

void cursor_keys(int key, int x, int y) {
  // Set up the keys for the turning
  switch (key) {
    case GLUT_KEY_LEFT:
      // Spin left
      lon += TURN_ANGLE;
      break;
    case GLUT_KEY_RIGHT:
      // Spin right
      lon -= TURN_ANGLE;
      break;
    case GLUT_KEY_UP:
      // Manual turning, look up
      lat += TURN_ANGLE;
      break;
    case GLUT_KEY_DOWN:
      // Manual turning, look down
      lat -= TURN_ANGLE;
      break;
    case GLUT_KEY_HOME:
      // Reset the latitude and longitude
      lat = 0.0;
      lon = 0.0;
      break;
  }

  // Bound the latitude to between 90 and -90
  if (lat >=  89.5) lat = 89.5;
  else if (lat <= -89.5) lat = -89.5;
}

/*****************************/

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("COMP27112 Exercise 2");
  glutFullScreen();
  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc (cursor_keys);
  glutIdleFunc(animate);
  readSystem();
  glutMainLoop();
  return 0;
}
