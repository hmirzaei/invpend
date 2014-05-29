#define K_N_1_1 (0)
#define K_N_1_2 (7.078622e-02)
#define K_N_1_3 (-7.134932e-02)
#define K_N_1_4 (1.030036e-02)
#define K_N_1_5 (1.154275e-03)
#define K_N_1_6 (9.447385e-08)
#define K_D_1_1 (-1)
#define K_D_1_2 (1.876222e+00)
#define K_D_1_3 (-1.150317e+00)
#define K_D_1_4 (2.573144e-01)
#define K_D_1_5 (-2.021720e-02)
#define K_D_1_6 (0)
double x1_1, x1_2, x1_3, x1_4, x1_5;
double y1_1, y1_2, y1_3, y1_4, y1_5;

#define K_N_2_1 (0)
#define K_N_2_2 (1.582848e+00)
#define K_N_2_3 (-2.681523e+00)
#define K_N_2_4 (1.336611e+00)
#define K_N_2_5 (-1.643441e-01)
#define K_N_2_6 (3.722562e-04)
#define K_D_2_1 (-1)
#define K_D_2_2 (1.876222e+00)
#define K_D_2_3 (-1.150317e+00)
#define K_D_2_4 (2.573144e-01)
#define K_D_2_5 (-2.021720e-02)
#define K_D_2_6 (0)
double x2_1, x2_2, x2_3, x2_4, x2_5;
double y2_1, y2_2, y2_3, y2_4, y2_5;

#define K_N_3_1 (0)
#define K_N_3_2 (-1.081442e+01)
#define K_N_3_3 (1.170944e+01)
#define K_N_3_4 (-3.230695e+00)
#define K_N_3_5 (2.596232e-01)
#define K_N_3_6 (-9.407636e-07)
#define K_D_3_1 (-1)
#define K_D_3_2 (1.876222e+00)
#define K_D_3_3 (-1.150317e+00)
#define K_D_3_4 (2.573144e-01)
#define K_D_3_5 (-2.021720e-02)
#define K_D_3_6 (0)
double x3_1, x3_2, x3_3, x3_4, x3_5;
double y3_1, y3_2, y3_3, y3_4, y3_5;

#define K_N_4_1 (0)
#define K_N_4_2 (-5.602640e+00)
#define K_N_4_3 (9.390406e+00)
#define K_N_4_4 (-4.631012e+00)
#define K_N_4_5 (5.595815e-01)
#define K_N_4_6 (1.025392e-04)
#define K_D_4_1 (-1)
#define K_D_4_2 (1.876222e+00)
#define K_D_4_3 (-1.150317e+00)
#define K_D_4_4 (2.573144e-01)
#define K_D_4_5 (-2.021720e-02)
#define K_D_4_6 (0)
double x4_1, x4_2, x4_3, x4_4, x4_5;
double y4_1, y4_2, y4_3, y4_4, y4_5;

double calcControllerOutput(double x1, double x2, double x3, double x4) {
  double y1, y2, y3, y4;
  y1 = K_N_1_1 * x1 + K_N_1_2 * x1_1 + K_N_1_3 * x1_2 + K_N_1_4 * x1_3 + K_N_1_5 * x1_4 + K_N_1_6 * x1_5 + K_D_1_2 * y1_1 + K_D_1_3 * y1_2 + K_D_1_4 * y1_3 + K_D_1_5 * y1_4 + K_D_1_6 * y1_5 + 0;
  y2 = K_N_2_1 * x2 + K_N_2_2 * x2_1 + K_N_2_3 * x2_2 + K_N_2_4 * x2_3 + K_N_2_5 * x2_4 + K_N_2_6 * x2_5 + K_D_2_2 * y2_1 + K_D_2_3 * y2_2 + K_D_2_4 * y2_3 + K_D_2_5 * y2_4 + K_D_2_6 * y2_5 + 0;
  y3 = K_N_3_1 * x3 + K_N_3_2 * x3_1 + K_N_3_3 * x3_2 + K_N_3_4 * x3_3 + K_N_3_5 * x3_4 + K_N_3_6 * x3_5 + K_D_3_2 * y3_1 + K_D_3_3 * y3_2 + K_D_3_4 * y3_3 + K_D_3_5 * y3_4 + K_D_3_6 * y3_5 + 0;
  y4 = K_N_4_1 * x4 + K_N_4_2 * x4_1 + K_N_4_3 * x4_2 + K_N_4_4 * x4_3 + K_N_4_5 * x4_4 + K_N_4_6 * x4_5 + K_D_4_2 * y4_1 + K_D_4_3 * y4_2 + K_D_4_4 * y4_3 + K_D_4_5 * y4_4 + K_D_4_6 * y4_5 + 0;
  x1_5 = x1_4;
  x1_4 = x1_3;
  x1_3 = x1_2;
  x1_2 = x1_1;
  x1_1 = x1;
  y1_5 = y1_4;
  y1_4 = y1_3;
  y1_3 = y1_2;
  y1_2 = y1_1;
  y1_1 = y1;
  x2_5 = x2_4;
  x2_4 = x2_3;
  x2_3 = x2_2;
  x2_2 = x2_1;
  x2_1 = x2;
  y2_5 = y2_4;
  y2_4 = y2_3;
  y2_3 = y2_2;
  y2_2 = y2_1;
  y2_1 = y2;
  x3_5 = x3_4;
  x3_4 = x3_3;
  x3_3 = x3_2;
  x3_2 = x3_1;
  x3_1 = x3;
  y3_5 = y3_4;
  y3_4 = y3_3;
  y3_3 = y3_2;
  y3_2 = y3_1;
  y3_1 = y3;
  x4_5 = x4_4;
  x4_4 = x4_3;
  x4_3 = x4_2;
  x4_2 = x4_1;
  x4_1 = x4;
  y4_5 = y4_4;
  y4_4 = y4_3;
  y4_3 = y4_2;
  y4_2 = y4_1;
  y4_1 = y4;
  return y1 + y2 + y3 + y4;
}
void initController() {
  x1_1 = 0;
  x1_2 = 0;
  x1_3 = 0;
  x1_4 = 0;
  x1_5 = 0;
  y1_1 = 0;
  y1_2 = 0;
  y1_3 = 0;
  y1_4 = 0;
  y1_5 = 0;
  x2_1 = 0;
  x2_2 = 0;
  x2_3 = 0;
  x2_4 = 0;
  x2_5 = 0;
  y2_1 = 0;
  y2_2 = 0;
  y2_3 = 0;
  y2_4 = 0;
  y2_5 = 0;
  x3_1 = 0;
  x3_2 = 0;
  x3_3 = 0;
  x3_4 = 0;
  x3_5 = 0;
  y3_1 = 0;
  y3_2 = 0;
  y3_3 = 0;
  y3_4 = 0;
  y3_5 = 0;
  x4_1 = 0;
  x4_2 = 0;
  x4_3 = 0;
  x4_4 = 0;
  x4_5 = 0;
  y4_1 = 0;
  y4_2 = 0;
  y4_3 = 0;
  y4_4 = 0;
  y4_5 = 0;
}
