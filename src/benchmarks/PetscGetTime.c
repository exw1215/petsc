#include "stdio.h"
#include "petsc.h"

int main( int argc, char **argv)
{
  double x, y;
  
  PetscInitialize(&argc, &argv,0,0,0);

  x = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();
  y = PetscGetTime();

  fprintf(stderr,"%-15s : %e sec\n","PetscGetTime", (y-x)/10.0);
  PetscFinalize();
  return 0;
}
