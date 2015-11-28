#include <stdio.h>
#include <mpi.h>

const int TAG_SOME_DATA = 10;

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int np;
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  if (rank == 0)
  {
    int x = 42;
    /* ��������� �������� � ����� ������ 1..np */
	for (int i = 1; i < np; i++)
		MPI_Send(&x, 1, MPI_INT, i, TAG_SOME_DATA, MPI_COMM_WORLD);
  }
  else
  {
    int x = 0;
    /* ������ � �� ������ 0 */
    MPI_Recv(&x, 1, MPI_DOUBLE, 0, TAG_SOME_DATA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Task %d.  Got data = %d\n", rank, x);
  }

  MPI_Finalize();
  return 0;
}

