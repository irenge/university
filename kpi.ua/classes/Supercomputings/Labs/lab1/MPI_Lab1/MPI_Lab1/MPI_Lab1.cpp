/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parallel Arcsin evaluation using MPI								 *
 * Programming of computer networks Lab �1 (1st semester 2010)		 *
 *																	 *
 *		Author:														 *
 * Podolsky Sergey													 *
 * Group:	KV-64M													 *
 *																	 *
 * written:	21.10.2010												 *
 * remarks:	debugged with Visual Studio 2010. Tested on a cluster.	 *
 *																	 *
 *		Project definition:											 *
 * MPI_Lab1.cpp		The entry point for the console application		 *
 * stdafx.h			Include file for standard system include files	 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "stdafx.h"

#define EPSILON				1E-11		// ������� ���������� ��������
#define VALUE_TAG			1			// �� ��������� ������� ����� E
#define ELEMENT_NUMBER_TAG	2			// �� ������ ��������� �������� ����
#define ELEMENT_TAG			3			// �� �������� ��������� �������� ����
#define BREAK_TAG			4			// �� ������� ��� ���������� ���������

#define input_file_name		"in.txt"	// ��'� ����� ������� �����
#define output_file_name	"out.txt"	// ��'� ����� ����������

/* ������� ���������� ��������� */
double factorial(int value)
{
	/* �������� ��'������ ����� �� ���������� */
	if (value < 0)
		return 0; // NAN

	/* 0! = 1 �� ����������� */
	if (value == 0)
		return 1.;

	/* ���������� ��������� N �� ������� ��� ����������� ����� �� 1 �� N */
	double fact = 1.;
	for (int i = 2; i <= value; i++)
		fact *= i;
	return fact;
}

/* ������� ���������� �������� ���� �� ���� ������� � ����� x
 * ��� arcsin(x) ������� ���� ������� x^(2n+1) * (2n)! / (4^n * n!^2 * (2n+1)))
 */
double calc_series_element(int element_number, double x)
{
	return pow(x, 2 * element_number + 1) * factorial(2 * element_number) / (pow(4., element_number) * pow(factorial(element_number), 2) * (2 * element_number + 1));
}

int _tmain(int argc, char* argv[])
{
	/* ����������� ���������� MPI */
	MPI_Init(&argc, &argv);
	
	/* ��������� ������ ���� ������ */
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	/* ��������� �������� ������� ����� */
	int np;
	MPI_Comm_size(MPI_COMM_WORLD, &np);
	
	/* �������� � ��� ���������� arcsin(x) */
	double x;
	
	/* �������� � � ������ 0 � ����� */
	if (rank == 0)
	{
		FILE *input_file = fopen(input_file_name, "r");
		/* ������� ���������� ��� �����, ���� �� ������� ������� ������� ���� */
		if (!input_file)
		{
			fprintf(stderr, "Can't open input file!\n\n");
			MPI_Abort(MPI_COMM_WORLD, 1);
			return 1;
		}
		/* ���������� � � ����� */
		fscanf(input_file, "%lf", &x);
		fclose(input_file);
	}
	
	/* �������� � � ������ 0 ��� ����� ������� */
	if (rank == 0)
	{
		/* ��������� �������� � ����� ������ 1..np */
		for (int i = 1; i < np; i++)
			MPI_Send(&x, 1, MPI_DOUBLE, i, VALUE_TAG, MPI_COMM_WORLD);
	}
	else
		/* ������ � �� ������ 0 */
		MPI_Recv(&x, 1, MPI_DOUBLE, 0, VALUE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
	/* ����� ���������� ����������� �������� ���� */
	int last_element_number = 0;
	/* ���� �������� ���� */
	double sum = .0;
	
	/* �������� ���� �������� */
	for (int step = 0; step < 1000; step++)
	{
		/* ����� ��������, �� ������������ �� ��������� ����� � ���� ������ */
		int element_number;
		
		/* ��������� � ������ 0 �� ��� ����� ����� ������ ��������, �� ����
		 * ����� ����������� �� ��������� �����
		 */
		if (rank == 0)
		{
			element_number = last_element_number++;
			int current_element_number = last_element_number;
			for (int i = 1; i < np; i++)
			{
				MPI_Send(&current_element_number, 1, MPI_INT, i, ELEMENT_NUMBER_TAG, MPI_COMM_WORLD);
				current_element_number++;
			}
			last_element_number = current_element_number;
		}
		else
			MPI_Recv(&element_number, 1, MPI_INT, 0, ELEMENT_NUMBER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		/* ���������� ��������� �������� ���� */
		double element = calc_series_element(element_number, x);
		
		/* ��������� "�������� ���������, ��� �� ��������� ��������� �������" */
		int need_break = false;
		
		/* ���������� ���� �������� ���� */
		if (rank == 0)
		{
			double current_element = element;
			/* ��������� �� �������� ���� ��������, ����������� � ������ 0 */
			sum += current_element;
			
			/* ������� ��� ��� arcsin(x) � ��������� ���������, �������� ����
			 * ������������ �������� �� ���������� ����� �� ������ �������� ����
			 * �� ����� �������� �� ���������� �����, �� ���� ������� ���������
			 * ������� �� ������� ����� ��������� EPSILON, �� � �� ������� ��������
			 * ����� ����� ���� ���������.  ϳ��� ��������� ������ �������� ���������
			 * ������� ��������� � ����� ����������� ��������
			 */
			if (abs(current_element) < EPSILON)
				need_break = true;
			
			for (int i = 1; i < np; i++)
			{
				/* ������ �������� �� i-�� ������, ��������� ���� �� �������� ���� */
				MPI_Recv(&current_element, 1, MPI_DOUBLE, i, ELEMENT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				sum += current_element;
				
				/* �������� ����� ���������� �������� (���. ����) */
				if (abs(current_element) < EPSILON)
				{
					need_break = true;
					break;
				}
			}
			
			/* �������� ������� ��� ����������� ���������� �������� � ������ 0 ���
			 * ����� ������� */
			for (int i = 1; i < np; i++)
				MPI_Send(&need_break, 1, MPI_INT, i, BREAK_TAG, MPI_COMM_WORLD);
		}
		else
		{
			/* �������� ����������� �������� ���� � ������ 0 */
			MPI_Send(&element, 1, MPI_DOUBLE, 0, ELEMENT_TAG, MPI_COMM_WORLD);
			
			/* ������� �� ������ 0 ������� ��� ����������� ���������� �������� */
			MPI_Recv(&need_break, 1, MPI_INT, 0, BREAK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		
		/* ���������� ��������, ���� ��������� ��������� ������� */
		if (need_break)
			break;
	}
	
	/* ���� ���������� � ������ 0 */
	if (rank == 0)
	{
		FILE *output_file = fopen(output_file_name, "w");
		/* ������� ����������, ���� �� ������� ������� ���� ���������� */
		if (!output_file)
		{
			fprintf(stderr, "Can't open output file!\n\n");
			MPI_Abort(MPI_COMM_WORLD, 2);
			return 2;
		}
		fprintf(output_file, "%.15lf\n", sum);
	}
	
	/* ��-����������� ���������� MPI �� ����� � �������� */
	MPI_Finalize();
	return 0;
}
