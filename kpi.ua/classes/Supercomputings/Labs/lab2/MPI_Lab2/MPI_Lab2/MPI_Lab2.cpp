/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parallel Arcsin evaluation using MPI								 *
 * Programming of computer networks Lab �2 (1st semester 2010)		 *
 *																	 *
 *		Author:														 *
 * Podolsky Sergey													 *
 * Group:	KV-64M													 *
 *																	 *
 * written:	17.11.2010												 *
 * remarks:	debugged with Visual Studio 2010. Tested on a cluster.	 *
 *																	 *
 *		Project definition:											 *
 * MPI_Lab1.cpp		The entry point for the console application		 *
 * stdafx.h			Include file for standard system include files	 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "stdafx.h"

#define input_filename	"input.txt"
#define output_filename	"output.txt"

/* ���������� ������� */
double function(double x)
{
	return pow(x, 3) * cos(x);
}

/* ������������ ������� �������� */
double integrate_composite_trapezium_rule(double start, double finish, double epsilon)
{
	int iteration_count = (int)(1 / sqrt(epsilon));	/* ��������� ������� �������� */
	double previous_result, next_result = 0;
	do
	{
		previous_result = next_result;
		next_result = (function(start) + function(finish)) / 2;
		double delta_x = (finish - start) / iteration_count;
		for (int i = 1; i < iteration_count; i++)
			next_result += function(start + i * delta_x);
		next_result *= delta_x;
		iteration_count *= 2;
	}
	while (abs(next_result - previous_result) / 3 > epsilon);
	return next_result;
}

int main(int argc, char* argv[])
{
	int np;
	int rank;
	
	/* ����������� MPI */
	MPI_Init(&argc,&argv);
	
	/* �������� ������� ������� */
	MPI_Comm_size(MPI_COMM_WORLD, &np);
	
	/* �������� ������������� ������������� ������� */
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	/* �������� ����� � ����� � ����� � ����� ������.
	 * ³��������� � ������ � ������ 0.
	 * 1 - ����� ���� ������������
	 * 2 - ������ ���� ������������
	 * 3 - ��������� ������� */
	double input[3];
	if (rank == 0)
	{
		/* ³������� �������� ����� � ����� ���� ��� ������� */
		FILE *file;
		/* �������� ����������� �������� ����� */
		if (fopen_s(&file, input_filename, "r") != 0)
		{
			printf("Failed to open the file");
			/* ���������� ��� �������, �� ������� � ���������� MPI_COMM_WORLD */
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		/* ���������� 3 ����� ���� double */
		for (int i = 0; i < 3; i++)
			fscanf_s(file, "%lg", &input[i]);
		/* �������� ����� */
		fclose(file);
	}
	/* �������� �������� ����� �� ������� � ������ 0 �� ��� ����� �������,
	 * �� ������� � ���������� MPI_COMM_WORLD */
	MPI_Bcast(input, 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	double start = input[0];
	double finish = input[1];
	double epsilon = input[2];
	double step = (finish - start) / np;
	double result_part = integrate_composite_trapezium_rule(start + rank * step, start + (rank + 1) * step, epsilon / np);
	double total_result;
	/* �������� ��������� ���������� ������������ �� ��� ������� �� ������� � ������ 0
	 * �� ���������� ���� �������� ���������� � ����� result */
	MPI_Reduce(&result_part, &total_result, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	
	/* ��������� �������� 0 ���������� ������ �������� � �������� ���� */
	if (rank == 0)
	{
		/* ³������� ����� �� ����� */
		FILE *file;
		/* �������� ����������� �������� ����� */
		if (fopen_s(&file, output_filename, "w") != 0)
		{
			printf("Failed to open the file");
			/* ���������� ��� �������, �� ������� � ���������� MPI_COMM_WORLD */
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		/* ����� 1 ����� ���� double � ���� */
		fprintf_s(file, "%f\n", total_result);
		/* �������� ����� */
		fclose(file);
	}
	
	/* �������� ������ � MPI */
	MPI_Finalize();
	return 0;
}