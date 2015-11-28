#include <mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "linalg.h"

#define RESULT_FILE "det.txt"

/* ������� ������� (�������� ���������� ����������) */
int main(int argc, char *argv[])
{
  /* ��'� �������� ����� */
  const char *input_file_MA = "MA.txt";
  /* ��� ����������, �� ������ �������� ������� */
  const int COLUMN_TAG = 0x1;
  
  /* Execution start time */
  clock_t start_clock;

  /* ����������� MPI */
  MPI_Init(&argc, &argv);

  /* ��������� �������� ������� ����� �� ����� ������� ������ */
  int np, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* ���������� ����� � ������ 0 */
  struct my_matrix *MA;
  int N;
  if(rank == 0)
  {
    MA = read_matrix(input_file_MA);

    if(MA->rows != MA->cols) {
      fatal_error("Matrix is not square!", 4);
    }
    N = MA->rows;

	/* Initialize start time */
	start_clock = clock();
  }

  /* �������� ��� ������� ��������� ������� �� ������� */
  MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

  /* ���������� ������� ��������, �� ������ ���������� � ����� ������ ��
   * �������� ���'�� ��� �� ��������� */
  int part = N / np;
  struct my_matrix *MAh = matrix_alloc(N, part, .0);

  /* ��������� �� ��������� ���� ����� ��� ������� �������� ������� */
  MPI_Datatype matrix_columns;
  MPI_Type_vector(N*part, 1, np, MPI_DOUBLE, &matrix_columns);
  MPI_Type_commit(&matrix_columns);

  /* ��������� �� ��������� ���� ����� ��� ��������� ������� */
  MPI_Datatype vector_struct;
  MPI_Aint extent;
  MPI_Type_extent(MPI_INT, &extent);            // ���������� ������ � ������
  MPI_Aint offsets[] = {0, extent};
  int lengths[] = {1, N+1};
  MPI_Datatype oldtypes[] = {MPI_INT, MPI_DOUBLE};
  MPI_Type_struct(2, lengths, offsets, oldtypes, &vector_struct);
  MPI_Type_commit(&vector_struct);

  /* �������� �������� ������� � ������ 0 � ���� ������ */
  if(rank == 0)
  {
    for(int i = 1; i < np; i++)
    {
      MPI_Send(&(MA->data[i]), 1, matrix_columns, i, COLUMN_TAG, MPI_COMM_WORLD);
    }
    /* ��������� �������� �������� ���� ������ */
    for(int i = 0; i < part; i++)
    {
      int col_index = i*np;
      for(int j = 0; j < N; j++)
      {
        MAh->data[j*part + i] = MA->data[j*N + col_index];
      }
    }
    free(MA);
  }
  else
  {
    MPI_Recv(MAh->data, N*part, MPI_DOUBLE, 0, COLUMN_TAG, MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);
  }

  /* ������� �������� ������� l_i */
  struct my_vector *current_l = vector_alloc(N, .0);
  /* ������� �������� ������� L */
  struct my_matrix *MLh = matrix_alloc(N, part, .0);

  /* �������� ���� �������� (�����) */
  for(int step = 0; step < N-1; step++)
  {
    /* ���� ������, �� ������ �������� � ������� ��������� �� ����������
     * �������� ������� ������� l_i  */
    if(step % np == rank)
    {
      int col_index = (step - (step % np)) / np;
      MLh->data[step*part + col_index] = 1.;
      for(int i = step+1; i < N; i++)
      {
        MLh->data[i*part + col_index] = MAh->data[i*part + col_index] / 
                                        MAh->data[step*part + col_index];
      }
      for(int i = 0; i < N; i++)
      {
        current_l->data[i] = MLh->data[i*part + col_index];
      }
    }
    /* �������� �������� ������� l_i */
    MPI_Bcast(current_l, 1, vector_struct, step % np, MPI_COMM_WORLD);

    /* ����������� �������� ������� �� �������� �� ��������� l_i */
    for(int i = step+1; i < N; i++)
    {
      for(int j = 0; j < part; j++)
      {
        MAh->data[i*part + j] -= MAh->data[step*part + j] * current_l->data[i];
      }
    }
  }

  /* ��������� ������� ��������, �� ����������� �� ������� �������
   * ������� ������� (� ����������� ������ ������� � ������) */
  double prod = 1.;
  for(int i = 0; i < part; i++)
  {
    int row_index = i*np + rank;
    prod *= MAh->data[row_index*part + i];
  }

  /* ������� ������� �������� ������� ������� �� ���� ���������� � ������ 0 */
  if(rank == 0)
  {
    MPI_Reduce(MPI_IN_PLACE, &prod, 1, MPI_DOUBLE, MPI_PROD, 0, MPI_COMM_WORLD);
	
	/* Time of computations in milliseconds */
    clock_t milliseconds = (clock() - start_clock) * 1000 / CLOCKS_PER_SEC;

	/* Create file */
	FILE *file = fopen(RESULT_FILE, "w");
	if (file == NULL)
	{
		fatal_error("Can not create result file!", 666);
	}
	fprintf(file, "det = %lf\n", prod);
	fprintf(file, "Execution time:\t%ld milliseconds\n", milliseconds);
	fclose(file);
  }
  else
  {
    MPI_Reduce(&prod, NULL, 1, MPI_DOUBLE, MPI_PROD, 0, MPI_COMM_WORLD);
  }

  /* ���������� �������� ������� */
  MPI_Type_free(&matrix_columns);
  MPI_Type_free(&vector_struct);
  return MPI_Finalize();
}

