#include <stdio.h>
#include <math.h>
#include <mpi.h>

int MPI_FlattreeColectiva(void * buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
    int numprocs , rank;
    
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            
    if(rank == root){
        for(int i=0; i < numprocs; i++){
            if(i != root)
                MPI_Send(buf, count, datatype, i, 0, comm);
        }           
    }
    else MPI_Recv(buf, count, datatype, root, 0, comm, MPI_STATUS_IGNORE);
    
    return MPI_SUCCESS;
}

int MPI_BinomialColectiva(void * buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm){ //suponemos root = 0
    int numprocs , myrank;
    
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);         
    
    
    for(int i=1; pow(2,i-1) <= numprocs; i++){
        if(myrank < pow(2,i-1) && myrank+pow(2,i-1) < numprocs){
            MPI_Send(buf, count, datatype, myrank+pow(2,i-1), 0, comm);    
            printf("Iteration %d: From %d to %d\n", i, myrank, (int)(myrank+pow(2,i-1)));
        }     
        
        if(myrank >= pow(2,i-1) && myrank < pow(2,i))
            MPI_Recv(buf, count, datatype, myrank-pow(2,i-1), 0, comm, MPI_STATUS_IGNORE);        
    }

    return MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
    int i, done = 0, n;
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
            
            if (n == 0) break; //no se utilizaría ningún proceso       
        } 
        
        //MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        //MPI_FlattreeColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
        h   = 1.0 / (double) n;
        sum = 0.0;
        for (i = 1+rank; i <= n; i+= numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = h * sum;

        MPI_Reduce(&pi, &aux_pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if(rank == 0) printf("pi is approximately %.16f, Error is %.16f\n", aux_pi, fabs(aux_pi - PI25DT));
                     
    }
    
    MPI_Finalize();   
}



