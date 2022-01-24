#include <stdio.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int i, done = 0, n, cnt=1;
    int numprocs , rank;
    double PI25DT = 3.141592653589793238462643;
    double pi, h, sum, x, aux_pi;

    MPI_Init(&argc , &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while (!done)
    {
        if(rank == 0){
            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d",&n);
            
            if (n == 0) break;
            
            cnt = 0;
            while (cnt < numprocs-1){
                cnt++;
                MPI_Send(&n, 1, MPI_INT, cnt, 0, MPI_COMM_WORLD);
            }
            
        } else{
            MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
     
     
        h   = 1.0 / (double) n;
        sum = 0.0;
        for (i = 1+rank; i <= n; i+= numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = h * sum;


        if(rank == 0){               
            cnt = 0;
            while (cnt < numprocs-1){
                cnt++;
                MPI_Recv(&aux_pi, 1, MPI_DOUBLE, cnt, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
                pi = pi + aux_pi;              
            }
            
            printf("pi is approximately %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
            
        } else{
            MPI_Send(&pi, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }            
    }
    
    MPI_Finalize();   
}


























