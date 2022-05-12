////////////////////////////////////////////////////////////////////////////////
//  int Upper_Triangular_Solve(float *U, float *B, float x[], int n)       //
//                                                                            //
//  Description:                                                              //
//     This routine solves the linear equation Ux = B, where U is an n x n    //
//     upper triangular matrix.  (The subdiagonal part of the matrix is       //
//     not addressed.)                                                        //
//     The algorithm follows:                                                 //
//                  x[n-1] = B[n-1]/U[n-1][n-1], and                          //
//     x[i] = [B[i] - (U[i][i+1] * x[i+1]  + ... + U[i][n-1] * x[n-1])]       //
//                                                                 / U[i][i], //
//     for i = n-2, ..., 0.                                                   //
//                                                                            //
//  Arguments:                                                                //
//     float *U   Pointer to the first element of the upper triangular       //
//                 matrix.                                                    //
//     float *B   Pointer to the column vector, (n x 1) matrix, B.           //
//     float *x   Pointer to the column vector, (n x 1) matrix, x.           //
//     int     n   The number of rows or columns of the matrix U.             //
//                                                                            //
//  Return Values:                                                            //
//     0  Success                                                             //
//    -1  Failure - The matrix U is singular.                                 //
//                                                                            //
//  Example:                                                                  //
//     #define N                                                              //
//     float A[N][N], B[N], x[N];                                            //
//                                                                            //
//     (your code to create matrix A and column vector B)                     //
//     err = Upper_Triangular_Solve(&A[0][0], B, x, n);                       //
//     if (err < 0) printf(" Matrix A is singular\n");                        //
//     else printf(" The solution is \n");                                    //
//           ...                                                              //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
int Upper_Triangular_Solve(float *U, float B[], float x[], int n)
{
   int i, k;

//         Solve the linear equation Ux = B for x, where U is an upper
//         triangular matrix.                                      
   
   for (k = n-1, U += n * (n - 1); k >= 0; U -= n, k--) {
      if (*(U + k) == 0.0) return -1;           // The matrix U is singular
      x[k] = B[k];
      for (i = k + 1; i < n; i++) x[k] -= x[i] * *(U + i);
      x[k] /= *(U + k);
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
//  void Unit_Lower_Triangular_Solve(float *L, float *B, float x[], int n) //
//                                                                            //
//  Description:                                                              //
//     This routine solves the linear equation Lx = B, where L is an n x n    //
//     unit lower triangular matrix.  (Only the subdiagonal part of the matrix//
//     is addressed.)  The diagonal is assumed to consist of 1's and is not   //
//     addressed.                                                             //
//     The algorithm follows:                                                 //
//                          x[0] = B[0], and                                  //
//            x[i] = B[i] - (L[i][0] * x[0]  + ... + L[i][i-1] * x[i-1]),     //
//     for i = 1, ..., n-1.                                                   //
//                                                                            //
//  Arguments:                                                                //
//     float *L   Pointer to the first element of the unit lower triangular  //
//                 matrix.                                                    //
//     float *B   Pointer to the column vector, (n x 1) matrix, B.           //
//     float *x   Pointer to the column vector, (n x 1) matrix, x.           //
//     int     n   The number of rows or columns of the matrix L.             //
//                                                                            //
//  Return Values:                                                            //
//     void                                                                   //
//                                                                            //
//  Example:                                                                  //
//     #define N                                                              //
//     float A[N][N], B[N], x[N];                                            //
//                                                                            //
//     (your code to create matrix A and column vector B)                     //
//     Unit_Lower_Triangular_Solve(&A[0][0], B, x, n);                        //
//     printf(" The solution is \n");                                         //
//           ...                                                              //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
void Unit_Lower_Triangular_Solve(float *L, float B[], float x[], int n)
{
   int i, k;

//         Solve the linear equation Lx = B for x, where L is a unit lower
//         triangular matrix.                                      
  
   x[0] = B[0];
   for (k = 1, L += n; k < n; L += n, k++) 
      for (i = 0, x[k] = B[k]; i < k; i++) x[k] -= x[i] * *(L + i);
}

////////////////////////////////////////////////////////////////////////////////
//  int Doolittle_LU_Decomposition(float *A, int n)                          //
//                                                                            //
//  Description:                                                              //
//     This routine uses Doolittle's method to decompose the n x n matrix A   //
//     into a unit lower triangular matrix L and an upper triangular matrix U //
//     such that A = LU.                                                      //
//     The matrices L and U replace the matrix A so that the original matrix  //
//     A is destroyed.                                                        //
//     Note!  In Doolittle's method the diagonal elements of L are 1 and are  //
//            not stored.                                                     //
//     Note!  The determinant of A is the product of the diagonal elements    //
//            of U.  (det A = det L * det U = det U).                         //
//     This routine is suitable for those classes of matrices which when      //
//     performing Gaussian elimination do not need to undergo partial         //
//     pivoting, e.g. positive definite symmetric matrices, diagonally        //
//     dominant band matrices, etc.                                           //
//     For the more general case in which partial pivoting is needed use      //
//                  Doolittle_LU_Decomposition_with_Pivoting.                 //
//     The LU decomposition is convenient when one needs to solve the linear  //
//     equation Ax = B for the vector x while the matrix A is fixed and the   //
//     vector B is varied.  The routine for solving the linear system Ax = B  //
//     after performing the LU decomposition for A is Doolittle_LU_Solve      //
//     (see below).                                                           //
//                                                                            //
//     The Doolittle method is given by evaluating, in order, the following   //
//     pair of expressions for k = 0, ... , n-1:                              //
//       U[k][j] = A[k][j] - (L[k][0]*U[0][j] + ... + L[k][k-1]*U[k-1][j])    //
//                                 for j = k, k+1, ... , n-1                  //
//       L[i][k] = (A[i][k] - (L[i][0]*U[0][k] + . + L[i][k-1]*U[k-1][k]))    //
//                          / U[k][k]                                         //
//                                 for i = k+1, ... , n-1.                    //
//       The matrix U forms the upper triangular matrix, and the matrix L     //
//       forms the lower triangular matrix.                                   //
//                                                                            //
//  Arguments:                                                                //
//     float *A   Pointer to the first element of the matrix A[n][n].        //
//     int     n   The number of rows or columns of the matrix A.             //
//                                                                            //
//  Return Values:                                                            //
//     0  Success                                                             //
//    -1  Failure - The matrix A is singular.                                 //
//                                                                            //
//  Example:                                                                  //
//     #define N                                                              //
//     float A[N][N];                                                        //
//                                                                            //
//     (your code to intialize the matrix A)                                  //
//                                                                            //
//     err = Doolittle_LU_Decomposition(&A[0][0], N);                         //
//     if (err < 0) printf(" Matrix A is singular\n");                        //
//     else { printf(" The LU decomposition of A is \n");                     //
//           ...                                                              //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
int Doolittle_LU_Decomposition(float *A, int n)
{
   int i, j, k, p;
   float *p_k, *p_row, *p_col;

//         For each row and column, k = 0, ..., n-1,
//            find the upper triangular matrix elements for row k
//            and if the matrix is non-singular (nonzero diagonal element).
//            find the lower triangular matrix elements for column k. 
 
   for (k = 0, p_k = A; k < n; p_k += n, k++) {
      for (j = k; j < n; j++) {
         for (p = 0, p_col = A; p < k; p_col += n,  p++)
            *(p_k + j) -= *(p_k + p) * *(p_col + j);
      }
      if ( *(p_k + k) == 0.0 ) return -1;
      for (i = k+1, p_row = p_k + n; i < n; p_row += n, i++) {
         for (p = 0, p_col = A; p < k; p_col += n, p++)
            *(p_row + k) -= *(p_row + p) * *(p_col + k);
         *(p_row + k) /= *(p_k + k);
      }  
   }
   return 0;
}


////////////////////////////////////////////////////////////////////////////////
//  int Doolittle_LU_Solve(float *LU, float *B, float *x,  int n)           //
//                                                                            //
//  Description:                                                              //
//     This routine uses Doolittle's method to solve the linear equation      //
//     Ax = B.  This routine is called after the matrix A has been decomposed //
//     into a product of a unit lower triangular matrix L and an upper        //
//     triangular matrix U without pivoting.  The argument LU is a pointer to //
//     the matrix the subdiagonal part of which is L and the superdiagonal    //
//     together with the diagonal part is U. (The diagonal part of L is 1 and //
//     is not stored.)   The matrix A = LU.                                   //
//     The solution proceeds by solving the linear equation Ly = B for y and  //
//     subsequently solving the linear equation Ux = y for x.                 //
//                                                                            //
//  Arguments:                                                                //
//     float *LU  Pointer to the first element of the matrix whose elements  //
//                 form the lower and upper triangular matrix factors of A.   //
//     float *B   Pointer to the column vector, (n x 1) matrix, B            //
//     float *x   Solution to the equation Ax = B.                           //
//     int     n   The number of rows or columns of the matrix LU.            //
//                                                                            //
//  Return Values:                                                            //
//     0  Success                                                             //
//    -1  Failure - The matrix A is singular.                                 //
//                                                                            //
//  Example:                                                                  //
//     #define N                                                              //
//     float A[N][N], B[N], x[N];                                            //
//                                                                            //
//     (your code to create matrix A and column vector B)                     //
//     err = Doolittle_LU_Decomposition(&A[0][0], N);                         //
//     if (err < 0) printf(" Matrix A is singular\n");                        //
//     else {                                                                 //
//        err = Doolittle_LU_Solve(&A[0][0], B, x, n);                        //
//        if (err < 0) printf(" Matrix A is singular\n");                     //
//        else printf(" The solution is \n");                                 //
//           ...                                                              //
//     }                                                                      //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
int Doolittle_LU_Solve(float *LU, float B[], float x[], int n)
{

//         Solve the linear equation Lx = B for x, where L is a lower
//         triangular matrix with an implied 1 along the diagonal.
   
   Unit_Lower_Triangular_Solve(LU, B, x, n);

//         Solve the linear equation Ux = y, where y is the solution
//         obtained above of Lx = B and U is an upper triangular matrix.

   return Upper_Triangular_Solve(LU, x, x, n);
}

int invertMatrix(float *LU, float *invA, float *I)
{
  int i, j;
  // float I[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
  float resultColumn[4];
  
  for (i = 0; i < 4; ++i)
  {
    int res = Doolittle_LU_Solve(LU, I + i*4, resultColumn, 4);

    if (res != 0) return res;
    for (j = 0; j < 4; ++j)
      *(invA + i  + j * 4) = resultColumn[j];
  }

  return 0;
}

//float A[4][4] = {{1, 1, 1, 1}, {1, 4, 2, 3}, {1, 2, 1, 2}, {1, 1, 1, 0}};
//float invA[4][4]= {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};

int fun(float *A, float *invA, float *b, float *x, float *I)
{
  int res = Doolittle_LU_Decomposition((float *)A, 4);

  if (res != 0) return res;

  //  float b[4] = {63, 105, 48, 186};
  // float x[4];

  res = Doolittle_LU_Solve((float *)A, b, x, 4);

  if (res != 0) return res;

  res = invertMatrix((float *)A, (float *)invA, I);

  return res;
}
