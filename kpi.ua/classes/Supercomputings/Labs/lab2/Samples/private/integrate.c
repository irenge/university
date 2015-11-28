#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <mpi.h>

//#define FUNCTION ZZZfunctionZZZ
//#define METHOD ZZZmethodZZZ
#include "config.h"

/* Обчислення функції */
double function(double x)
{
  return FUNCTION;
}

/* Перевірка правила Рунге */
bool check_Runge(double I2, double I, double epsilon)
{
  return ((double)fabs(I2 - I) / 3) < epsilon;
}

double integrate_left_rectangle(double start, double finish, double epsilon)
{
  int num_iterations = 1; /* Початкова кількість ітерацій */
  double last_res = 0;    /* Результат інтeгрування на попередньому кроці */
  double res = -1;        /* Поточний результат інтегрвання */
  double h = 0;           /* Ширина прямокутників */
  while(!check_Runge(res, last_res, epsilon))
  {
    num_iterations *= 2;
    printf("last_res = %lg, res = %lg, doing %d iters\n", last_res, res, num_iterations);
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

double integrate_right_rectangle(double start, double finish, double epsilon)
{
  int num_iterations = 1;
  double last_res = 0;
  double res = -1;
  double h = 0;
  while(!check_Runge(res, last_res, epsilon))
  {
    num_iterations *= 2;
    printf("last_res = %lg, res = %lg, doing %d iters\n", last_res, res, num_iterations);
    last_res = res;
    res = 0;
    h = (finish - start) / num_iterations;
    for(int i = 1; i <= num_iterations; i++)
    {
      res += function(start + i * h) * h;
    }
  }
  return res;
}

double integrate_middle_rectangle(double start, double finish, double epsilon)
{
  int num_iterations = 1;
  double last_res = 0;
  double res = -1;
  double h = 0;
  while(!check_Runge(res, last_res, epsilon))
  {
    num_iterations *= 2;
    printf("last_res = %lg, res = %lg, doing %d iters\n", last_res, res, num_iterations);
    last_res = res;
    res = 0;
    h = (finish - start) / num_iterations;
    for(int i = 1; i <= num_iterations; i++)
    {
      res += function(start + i * h - h / 2.0) * h;
    }
  }
  return res;
}

double integrate_trap(double start, double finish, double epsilon)
{
  int num_iterations = 1;
  double last_res = 0;
  double res = -1;
  double h = 0;
  while(!check_Runge(res, last_res, epsilon))
  {
    num_iterations *= 2;
    printf("last_res = %lg, res = %lg, doing %d iters\n", last_res, res, num_iterations);
    last_res = res;
    res = 0;
    h = (finish - start) / num_iterations;
    for(int i = 0; i < num_iterations; i++)
    {
      res += function(start + i * h + h / 2.0);
    }
    res += (function(start) + function(finish)) / 2.0;
    res *= h;
  }
  return res;
}

double integrate_simps(double start, double finish, double epsilon)
{
  int num_iterations = 1;
  double last_res = 0;
  double res = -1;
  double h = 0;
  while(!check_Runge(res, last_res, epsilon))
  {
    num_iterations *= 2;
    printf("last_res = %lg, res = %lg, doing %d iters\n", last_res, res, num_iterations);
    last_res = res;
    res = 0;
    h = (finish - start) / num_iterations;

    for(int i = 1; i < num_iterations; i++)
    {
      res += function(start + i * h);
    }

    for(int i = 0; i < num_iterations; i++)
    {
      res += function(start + i * h + h / 2.0);
    }

    res += (function(start) + function(finish)) / 2.0;
    res *= h / 3.0;
  }
  return res;
}

/* Запис одного числа типу double в файл з ім'ям filename */
void write_double_to_file(const char* filename, double data)
{
  /* Відкриття файлу на запис */
  FILE *fp = fopen(filename, "w");
  /* Перевірка правильності відкриття файлу */
  if(fp == NULL)
  {
    printf("Failed to open the file\n");
    /* Аварійне завершення всіх задач */
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  /* Запис 1 числа типу double в файл */
  fprintf(fp, "%lg\n", data);
  /* Закриття файлу */
  fclose(fp);
}

int main(int argc, char* argv[])
{
  int np; /* ранг цієї задачі */
  int rank; /* кількість задач */

  MPI_Init(&argc,&argv);

  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* Введення даних з файлу в масив з 3-х змінних.
   * Відбувається у задачі 0.
   * input[0] -- нижня межа інтегрування
   * input[1] -- верхня межа інтегрування
   * input[2] -- допустима похибка */
  double input[3];
  if(rank == 0)
  {
    /* Відкриття файлу input.txt у режимі лише для читання */
    FILE *fp = fopen("input.txt", "r");
    /* Перевірка правильності відкриття файлу */
    if(fp == NULL)
    {
      printf("Failed to open the file\n");
      /* Аварійне завершення всіх задач */
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* Зчитування 3 чисел типу double */
    for(int i = 0; i < 3; i++)
      fscanf(fp, "%lg", &input[i]);

    /* Закриття файлу */
    fclose(fp);
  }

  /* Передача введених даних від задачі 0 до всіх інших задач */
  MPI_Bcast(input, 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  double start = input[0];
  double finish = input[1];
  double epsilon = input[2];
  double step = (finish - start) / np;
  double res = METHOD(start + rank * step, start +
               (rank + 1) * step, epsilon / np);

  /* Передача проміжного результату інтегрування від усіх задач,
   * окрім задачі 0, до задачі 0 */
  if(rank != 0)
  {
    MPI_Send(&res, 1, MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);
  }

  /* Прийом задачею 0 проміжних результатів інтегрування
   * від усіх інших задач */
  if(rank == 0)
  {
    MPI_Request recv_reqs[np - 1];
    MPI_Status status[np - 1];
    double resall[np - 1];
    for(int i = 0; i < (np - 1); i++)
    {
      /* Прийом проміжного результату інтегрування без блокування виконання
       * програми. Після виконання операції масив resall стане недоступним
       * для використання */
      MPI_Irecv(&resall[i], 1, MPI_DOUBLE, (i+1), (i+1), MPI_COMM_WORLD,
                &recv_reqs[i]);
    }
    /* Очікування на завершення прийому усіх проміжних результатів
     * інтегрування. Після виконання операції масив resall знову
     * стане доступним для використання */
    MPI_Waitall(np - 1, recv_reqs, status);
    for(int i = 0; i < (np - 1); i++)
    {
      res += resall[i];
    }
    /* Виведення задачею 0 результату роботи програми у вихідний файл з ім'ям
     * output.txt */
    write_double_to_file("output.txt", res);
  }

  MPI_Finalize();
  return 0;
}
