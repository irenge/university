#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <mpi.h>

/* ���������� ������� */
double function(double x)
{
  /* x^3 */
  return x*x*x;
}

/* �������� ������� ����� */
bool check_Runge(double I2, double I, double epsilon)
{
  return ((double)fabs(I2 - I) / 3) < epsilon;
}

/* ������������ ������� ���� ������������ */
double integrate_left_rectangle(double start, double finish, double epsilon)
{
  int num_iterations = 1; /* ��������� ������� �������� */
  double last_res = 0;    /* ��������� ���e�������� �� ������������ ����� */
  double res = -1;        /* �������� ��������� ����������� */
  double h = 0;           /* ������ ������������ */
  while(!check_Runge(res, last_res, epsilon))
  {
    num_iterations *= 2;
    last_res = res;
    res = 0;
    h = (finish - start) / num_iterations;
    for(int i = 0; i < num_iterations; i++)
    {
      res += function(start + i * h) * h;
    }
  }
  return res;
}

/* ����� ������ ����� ���� double � ���� � ��'�� filename */
void write_double_to_file(const char* filename, double data)
{
  /* ³������� ����� �� ����� */
  FILE *fp = fopen(filename, "w");
  /* �������� ����������� �������� ����� */
  if(fp == NULL)
  {
    printf("Failed to open the file\n");
    /* ������� ���������� ��� ����� */
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  /* ����� 1 ����� ���� double � ���� */
  fprintf(fp, "%lg\n", data);
  /* �������� ����� */
  fclose(fp);
}

int main(int argc, char* argv[])
{
  int np; /* ���� ���� ������ */
  int rank; /* ������� ����� */

  MPI_Init(&argc,&argv);

  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* �������� ����� � ����� � ����� � 3-� ������.
   * ³��������� � ������ 0.
   * input[0] -- ����� ���� ������������
   * input[1] -- ������ ���� ������������
   * input[2] -- ��������� ������� */
  double input[3];
  if(rank == 0)
  {
    /* ³������� ����� input.txt � ����� ���� ��� ������� */
    FILE *fp = fopen("input.txt", "r");
    /* �������� ����������� �������� ����� */
    if(fp == NULL)
    {
      printf("Failed to open the file\n");
      /* ������� ���������� ��� ����� */
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* ���������� 3 ����� ���� double */
    for(int i = 0; i < 3; i++)
      fscanf(fp, "%lg", &input[i]);

    /* �������� ����� */
    fclose(fp);
  }

  /* �������� �������� ����� �� ������ 0 �� ��� ����� ����� */
  MPI_Bcast(input, 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  double start = input[0];
  double finish = input[1];
  double epsilon = input[2];
  double step = (finish - start) / np;
  double res = integrate_left_rectangle(start + rank * step, start +
               (rank + 1) * step, epsilon / np);

  /* �������� ��������� ���������� ������������ �� ��� �����,
   * ���� ������ 0, �� ������ 0 */
  if(rank != 0)
  {
    MPI_Send(&res, 1, MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);
  }

  /* ������ ������� 0 �������� ���������� ������������
   * �� ��� ����� ����� */
  if(rank == 0)
  {
    MPI_Request recv_reqs[np - 1];
    MPI_Status status[np - 1];
    double resall[np - 1];
    for(int i = 0; i < (np - 1); i++)
    {
      /* ������ ��������� ���������� ������������ ��� ���������� ���������
       * ��������. ϳ��� ��������� �������� ����� resall ����� �����������
       * ��� ������������ */
      MPI_Irecv(&resall[i], 1, MPI_DOUBLE, (i+1), (i+1), MPI_COMM_WORLD,
                &recv_reqs[i]);
    }
    /* ���������� �� ���������� ������� ��� �������� ����������
     * ������������. ϳ��� ��������� �������� ����� resall �����
     * ����� ��������� ��� ������������ */
    MPI_Waitall(np - 1, recv_reqs, status);
    for(int i = 0; i < (np - 1); i++)
    {
      res += resall[i];
    }
    /* ��������� ������� 0 ���������� ������ �������� � �������� ���� � ��'��
     * output.txt */
    write_double_to_file("output.txt", res);
  }

  MPI_Finalize();
  return 0;
}
