#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <GL/freeglut.h>
#include "dcpu16.h"
#include "sped3.h"

#ifdef BUILD_MODULE
#include "hardware_module.h"
#else
#include "hardware_host.h"
#endif

#define HW_ID (0x42babf3c)
#define HW_VER (0x0003)
#define HW_MFG (0x1eb37e91)

struct vertex {
	uint16_t first_word;
	uint16_t second_word;
};
#define VX(V) ((V).first_word & 0x00FF)
#define VY(V) (((V).first_word & 0xFF00) >> 8)
#define VZ(V) ((V).second_word & 0x00FF)
#define VC(V) (((V).second_word & 0x0300) >> 8)
#define VI(V) (((V).second_word & 0x0400) >> 10)
#define SET_VERTEX(V, X, Y, Z, C, I) do { \
	(V).first_word = ((X) & 0x00FF) | (((Y) << 8) & 0xFF00); \
	(V).second_word = ((Z) & 0x00FF) | (((C) << 8) & 0x0300) | \
			(((I) << 10) & 0x0400); \
} while (0);

static struct vertex *vertex_data;
static uint16_t vertex_count;
static uint16_t current_rotation, target_rotation;
static float view_rot_x, view_rot_y;

static uint16_t state;
static uint16_t error;

static void display(void);
static void reshape(int w, int h);
static void keyboard(unsigned char key, int x, int y);

static void sped_init(void);
static int sped_interrupt(void);
static void sped_step(void);

#ifndef BUILD_MODULE
struct hw_builtin sped_builtin = {
	HW_ID,
	HW_VER,
	HW_MFG,
	&sped_init,
	&sped_interrupt,
	&sped_step
};

struct hw_builtin *sped3(void)
{
	return &sped_builtin;
}
#endif

#ifdef BUILD_MODULE
int main(int argc, char *argv[])
{
	init_module();
	sped_init();

	while (1) {
		sped_step();
		if (int_requested) {
			fprintf(stderr, "got hwi, A=0x%hx\n", registers->A);
			sped_interrupt();
			int_requested = 0;
		}
		if (hwq_requested) {
			fprintf(stderr, "got hwq\n");
			hwq(HW_ID, HW_VER, HW_MFG);
			hwq_requested = 0;
		}
	}

	shutdown_module();
	return 0;
}
#endif

void sped_init()
{
	int argc = 1;
	char *argv[] = {"sped-3"};

	glutInit(&argc, argv);
	glutInitWindowSize(500, 500);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("sped-3");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	vertex_data = (struct vertex *)&memory[0];
	vertex_count = 0;
	current_rotation = 0;
	target_rotation = 0;

	view_rot_x = view_rot_y = 0;

	state = SPED_STATE_NO_DATA;
	error = SPED_ERROR_NONE;
}

int sped_interrupt(void)
{
	switch (registers->A) {
	case 0:
		registers->B = state;
		registers->C = error; // TODO clear error once it's been polled?
		break;
	case 1:
		vertex_data = (struct vertex *)&memory[registers->X];
		vertex_count = registers->Y;
		break;
	case 2:
		target_rotation = (registers->X % 360);
		break;
	default:
		break;
	}

	glutPostRedisplay();

	return 0;
}

void sped_step(void)
{
	glutMainLoopEvent();

	if (current_rotation != target_rotation) {
		//TODO rotate toward target @5 deg/s
	}

	if (current_rotation != target_rotation) {
		state = SPED_STATE_TURNING;
	} else if (vertex_count == 0) {
		state = SPED_STATE_NO_DATA;
	} else {
		state = SPED_STATE_RUNNING;
	}
}

void display(void)
{
	int i;

	fprintf(stderr, "sped3 displaying 0x%hX lines\n", vertex_count);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glRotatef(current_rotation, 0, 1, 0);
	glRotatef(view_rot_x, 0, 1, 0);
	glRotatef(view_rot_y, 1, 0, 0);

	glBegin(GL_LINE_LOOP);
		for (i = 0; (i < vertex_count) && (i < 128); i++) {
			//TODO use CC and I bits
			fprintf(stderr, "vertex 0x%hX, 0x%hX, 0x%hX\n",
					VY(vertex_data[i]),
					VY(vertex_data[i]),
					VZ(vertex_data[i]));
			glVertex3f((float)VX(vertex_data[i]) / 128.0 - 1.0,
					(float)VY(vertex_data[i]) / 128.0 - 1.0,
					(float)VZ(vertex_data[i]) / 128.0 - 1.0);
		}
	glEnd();

	glutSwapBuffers();
}

void reshape(int w, int h)
{
	int dim;

	if (w > h) {
		dim = h;
	} else {
		dim = w;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, dim, dim);
	glOrtho(-2, 2, -2, 2, -2, 2);
	glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y)
{
	(void)x; (void)y;

	if (key == 'a') {
		view_rot_x--;
	} else if (key == 'd') {
		view_rot_x++;
	} else if (key == 'w') {
		view_rot_y--;
	} else if (key == 's') {
		view_rot_y++;
	}

	glutPostRedisplay();
}
