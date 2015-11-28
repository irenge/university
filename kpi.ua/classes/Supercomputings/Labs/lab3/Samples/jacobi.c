#include <mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "linalg.h"

/* ������� ���������� ������ */
const double epsilon = 0.001;

/* ������� ���������� ���������� ���������� ������������ ������� ���� */
void jacobi_iteration(
    int start_row,                     // ������ ������� ����� ������� �������,
                                       // ��� ���������� ����� ��������, � 
                                       // ������� �������

    struct my_matrix *mat_A_part,      // ������� ����� ������� �����������
    struct my_vector *b,               // ������ ������ �����
    struct my_vector *vec_prev_x,      // ������ ������������ ����������
    struct my_vector *vec_next_x_part, // ������� ������� ���������� ����������
                                       // (�������������� � �������)
    double *residue_norm_part)         // �������� ����� �� ������������ �����
                                       // ��������� (�������������� � �������)
{
  /* ���������� �������� ����� ���� ������� ��������� */
  double my_residue_norm_part = 0.0;

  /* ����������� ���������� ������� ������� ���������� ���������� */
  for(int i = 0; i < vec_next_x_part->size; i++)
  {
    double sum = 0.0;
    for(int j = 0; j < mat_A_part->cols; j++)
    {
      if(i + start_row != j)
      {
        sum += mat_A_part->data[i * mat_A_part->cols + j] * vec_prev_x->data[j];
      }
    }

    sum = b->data[i + start_row] - sum;

    vec_next_x_part->data[i] = sum / mat_A_part->data[i * mat_A_part->cols + i + start_row];

    /* ���������� ����� �� ������������ ����� */
    sum = -sum + mat_A_part->data[i * mat_A_part->cols + i + start_row] * vec_prev_x->data[i + start_row];
    my_residue_norm_part += sum * sum;
  }

  *residue_norm_part = my_residue_norm_part;
}

/* ������� ������� */
int main(int argc, char *argv[])
{
  const char *input_file_MA = "MA.txt";
  const char *input_file_b = "b.txt";
  const char *output_file_x = "x.txt";

  /* ����������� MPI */
  MPI_Init(&argc, &argv);

  /* ��������� �������� ������� ����� �� ����� ������� ������ */
  int np, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  /* ���������� ����� � ������ 0 */
  struct my_matrix *MA;
  struct my_vector *b;
  int N;
  if(rank == 0)
  {
    MA = read_matrix(input_file_MA);
    b = read_vector(input_file_b);

    if(MA->rows != MA->cols) {
      fatal_error("Matrix is not square!", 4);
    }
    if(MA->rows != b->size) {
      fatal_error("Dimensions of matrix and vector don't match!", 5);
    }
    N = b->size;
  }

  /* �������� ��� ������� ��������� ������� �� ������� */
  MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
  /* ������������ ���'�� ��� ��������� ������� ������ ����� */
  if(rank != 0)
  {
    b = vector_alloc(N, .0);
  }

  /* ���������� ������� ������� �� �������, ��� ���� ���������� � �����
   * ������, �������� �� N = k*np. ������������ ���'�� ��� ��������� ������
   * ������� �� ������� � ����� ������ �� ������������ �� ���������� ������� */
  int part = N / np;
  struct my_matrix *MAh = matrix_alloc(part, N, .0);
  struct my_vector *oldx = vector_alloc(N, .0);
  struct my_vector *newx = vector_alloc(part, .0);

  /* �������� ������� ������� MA �� ������� �� part ����� �� �������� ������
   * � �� ������.  ���������� ���'��, ������� ��� ������� �� ������. */
  if(rank == 0) 
  {
    MPI_Scatter(MA->data, N*part, MPI_DOUBLE,
                MAh->data, N*part, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    free(MA);
  } 
  else 
  {
    MPI_Scatter(NULL, 0, MPI_DATATYPE_NULL,
                MAh->data, N*part, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
  }
  /* �������� ������� ������ ����� */
  MPI_Bcast(b->data, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  /* ���������� ����� ������� ������ ����� � ������ 0 �� �������� �� ��������
   * � �� ������ */
  double b_norm = 0.0;
  if(rank == 0)
  {
    for(int i = 0; i < b->size; i++)
    {
      b_norm = b->data[i] * b->data[i];
    }
    b_norm = sqrt(b_norm);
  }
  MPI_Bcast(&b_norm, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  /* �������� ������� ������� �������� */
  double last_stop_criteria;
  /* �������� ���� �������� ���� */
  for(int i = 0; i < 1000; i++)
  {
    double residue_norm_part;
    double residue_norm;

    jacobi_iteration(rank * part, MAh, b, oldx, newx, &residue_norm_part);

    /* ���������� ��������� �������� ���'���� */
    MPI_Allreduce(&residue_norm_part, &residue_norm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    residue_norm = sqrt(residue_norm);

    /* �������� ������� ������� ��������.  ������� �� ������� �����
     * ������������ �������� ����� �� ������������ �����, �� �����������
     * ���������� � ��� ������������ ����� */
    last_stop_criteria = residue_norm / b_norm;
    if(last_stop_criteria < epsilon)
    {
      break;
    }

    /* ��� ������� ��������� ���������� ������� �������� */
    MPI_Allgather(newx->data, part, MPI_DOUBLE,
        oldx->data, part, MPI_DOUBLE, MPI_COMM_WORLD);
  }

  /* ���� ���������� */
  if(rank == 0) 
  {
    write_vector(output_file_x, oldx);
  }
  
  /* ���������� �������� ������� ������ �� ���������� ���������� MPI */
  free(MAh);
  free(oldx);
  free(newx);
  free(b);
  return MPI_Finalize();
}

