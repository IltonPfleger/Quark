#pragma once
#include <GL/glut.h>
#include <stdio.h>
#include <time.h>

class LidarViewer
{
    struct Point {
        double x;
        double y;
        double z;
        double i;
    };

    static constexpr const char* WINDOW = "LidarViewer";
    static constexpr size_t WIDTH       = 800;
    static constexpr size_t HEIGHT      = 800;
    static constexpr size_t N_POINTS    = 30000;
    static constexpr double SENSITIVITY = 0.5;
    static struct Point POINTS[N_POINTS];
    static size_t COUNTER;
    static double ANGLE_X;
    static double ANGLE_Y;
    static bool RUNNING;

   public:
    static void init(int argc, char* argv[])
    {
        if (!RUNNING) {
            srand(time(NULL));
            glutInit(&argc, argv);
            glutInitWindowSize(LidarViewer::WIDTH, LidarViewer::HEIGHT);
            glutCreateWindow(LidarViewer::WINDOW);
            glutDisplayFunc(LidarViewer::display);
            glutMotionFunc(LidarViewer::motion);
            glutIdleFunc(LidarViewer::update);
            glutMainLoop();
            RUNNING = true;
        }
    }

    static void setColorBasedOnIntensity(unsigned char intensity)
    {
        if (intensity <= 10) {
            glColor3f(0, 0, 1);
        } else if (intensity <= 50) {
            glColor3f(0, 1, 0);
        } else if (intensity <= 150) {
            glColor3f(1, 1, 0);
        } else {
            glColor3f(1, 0, 0);
        }
    }

    static void display()
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glTranslatef(0, 0, 0);
        glRotatef(ANGLE_X, 0, 1, 0);
        glRotatef(ANGLE_Y, 1, 0, 0);
        renderPoints();
        glutSwapBuffers();
    }

    static void motion(int x, int y)
    {
        static int last_x = 0;
        static int last_y = 0;
        int dx            = last_x - x;
        int dy            = last_y - y;

        ANGLE_X += dx * SENSITIVITY;
        ANGLE_Y += dy * SENSITIVITY;

        last_x = x;
        last_y = y;
        glutPostRedisplay();
    }

    static void renderPoints()
    {
        glBegin(GL_POINTS);
        for (size_t i = 0; i < N_POINTS; i++) {
            setColorBasedOnIntensity(POINTS[i].i);
            glVertex3f(POINTS[i].x, POINTS[i].y, POINTS[i].z);
        }
        glEnd();
    }

    static void add(double x, double y, double z, unsigned char i)
    {
        if (COUNTER >= N_POINTS) COUNTER = 0;
        POINTS[COUNTER].x = x / 10;
        POINTS[COUNTER].y = y / 10;
        POINTS[COUNTER].z = z / 10;
        POINTS[COUNTER].i = i;
        COUNTER++;
    }

    static void update() { glutPostRedisplay(); }
};

double LidarViewer::ANGLE_X = 0.0f;
double LidarViewer::ANGLE_Y = 0.0f;
size_t LidarViewer::COUNTER = 0;
bool LidarViewer::RUNNING   = false;
struct LidarViewer::Point LidarViewer::POINTS[LidarViewer::N_POINTS];
