#include "blocksqp.hpp"
#include "blocksqp_general_purpose.hpp"

namespace blockSQP
{

SQPstats::SQPstats( PATHSTR myOutpath )
{
    strcpy( outpath, myOutpath );

    itCount = 0;
    qpItTotal = 0;
    qpIterations = 0;
    qpIterations2 = 0;
    qpResolve = 0;
    rejectedSR1 = 0;
    hessSkipped = 0;
    hessDamped = 0;
    averageSizingFactor = 0.0;
}

/**
 * One line of output. If termination criterion was met in the current
 * iteration, return 1, otherwise 0.
 */
void SQPstats::printProgress( Problemspec *prob, SQPiterate *vars, SQPoptions *param, bool hasConverged )
{
    int i;
    Matrix dummyWeights( 0, 0, 0 );

    /*
     * vars->steptype:
     *-1: full step was accepted because it reduces the KKT error although line search failed
     * 0: standard line search step
     * 1: Hessian has been reset to identity
     * 2: feasibility restoration heuristic has been called
     * 3: feasibility restoration phase has been called
     */

    if( itCount == 0 )
    {
        if( param->printLevel > 0 )
        {
            prob->printInfo();

            // Headline
            printf("%-8s", "   it" );
            printf("%-21s", " qpIt" );
            printf("%-9s","obj" );
            printf("%-11s","feas" );
            printf("%-7s","opt" );
            if( param->printLevel > 1 )
            {
                printf("%-11s","|lgrd|" );
                printf("%-9s","|stp|" );
                printf("%-10s","|lstp|" );
            }
            printf("%-8s","alpha" );
            if( param->printLevel > 1 )
            {
                printf("%-6s","nSOCS" );
                printf("%-18s","sk, da, sca" );
                printf("%-6s","QPr,mu" );
            }
            printf("\n");

            // Values for first iteration
            printf("%5i  ", itCount );
            printf("%11i ", 0 );
            printf("% 10e  ", vars->obj );
            printf("%-10.2e", vars->cNorm );
            printf("%-10.2e", vars->tol );
            printf("\n");
        }

#ifdef MYDEBUG
        // Print everything in a CSV file as well
        fprintf( progressFile, "%23.16e, %23.16e, %23.16e, %23.16e, %23.16e, %23.16e, %23.16e, %23.16e, %i, %i, %23.16e, %i, %23.16e\n",
                 vars->obj, vars->cNorm, vars->tol, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0.0, 0, 0.0 );
#endif
    }
    else
    {
        // Every twenty iterations print headline
        if( itCount % 20 == 0 && param->printLevel > 0 )
        {
            printf("%-8s", "   it" );
            printf("%-21s", " qpIt" );
            printf("%-9s","obj" );
            printf("%-11s","feas" );
            printf("%-7s","opt" );
            if( param->printLevel > 1 )
            {
                printf("%-11s","|lgrd|" );
                printf("%-9s","|stp|" );
                printf("%-10s","|lstp|" );
            }
            printf("%-8s","alpha" );
            if( param->printLevel > 1 )
            {
                printf("%-6s","nSOCS" );
                printf("%-18s","sk, da, sca" );
                printf("%-6s","QPr,mu" );
            }
            printf("\n");
        }

        // All values
        if( param->printLevel > 0 )
        {
            printf("%5i  ", itCount );
            printf("%5i+%5i ", qpIterations, qpIterations2 );
            printf("% 10e  ", vars->obj );
            printf("%-10.2e", vars->cNorm );
            printf("%-10.2e", vars->tol );
            if( param->printLevel > 1 )
            {
                printf("%-10.2e", vars->gradNorm );
                printf("%-10.2e", lInfVectorNorm( vars->deltaXi ) );
                printf("%-10.2e", vars->lambdaStepNorm );
            }

            if( (vars->alpha == 1.0 && vars->steptype != -1) || !param->printColor )
                printf("%-9.1e", vars->alpha );
            else
                printf("\033[0;36m%-9.1e\033[0m", vars->alpha );

            if( param->printLevel > 1 )
            {
                if( vars->nSOCS == 0 || !param->printColor )
                    printf("%5i", vars->nSOCS );
                else
                    printf("\033[0;36m%5i\033[0m", vars->nSOCS );
                printf("%3i, %3i, %-9.1e", hessSkipped, hessDamped, averageSizingFactor );
                printf("%i, %-9.1e", qpResolve, l1VectorNorm( vars->deltaH )/vars->nBlocks );
            }
            printf("\n");
        }
#ifdef MYDEBUG
        // Print everything in a CSV file as well
        fprintf( progressFile, "%23.16e, %23.16e, %23.16e, %23.16e, %23.16e, %23.16e, %23.16e, %i, %i, %i, %23.16e, %i, %23.16e\n",
                 vars->obj, vars->cNorm, vars->tol, vars->gradNorm, lInfVectorNorm( vars->deltaXi ),
                 vars->lambdaStepNorm, vars->alpha, vars->nSOCS, hessSkipped, hessDamped, averageSizingFactor,
                 qpResolve, l1VectorNorm( vars->deltaH )/vars->nBlocks );

        // Print update sequence
        fprintf( updateFile, "%i\t", qpResolve );
#endif
    }

    // Print Debug information
    printDebug( prob, vars );

    // Do not accidentally print hessSkipped in the next iteration
    hessSkipped = 0;
    hessDamped = 0;

    // qpIterations = number of iterations for the QP that determines the step, can be a resolve (+SOC)
    // qpIterations2 = number of iterations for a QP which solution was discarded
    qpItTotal += qpIterations;
    qpItTotal += qpIterations2;
    qpIterations = 0;
    qpIterations2 = 0;
    qpResolve = 0;

    if( param->printLevel > 0 )
        if( hasConverged && vars->steptype < 2 )
            if( param->printColor )
                printf("\033[1;32m***CONVERGENCE ACHIEVED!***\n\033[0m");
            else
                printf("***CONVERGENCE ACHIEVED!***\n");
}


void SQPstats::initStats()
{
    PATHSTR filename;

    // Open files

#ifdef MYDEBUG
    // SQP progress
    strcpy( filename, outpath );
    strcat( filename, "sqpits.csv" );
    progressFile = fopen( filename, "w");

    // Update sequence
    strcpy( filename, outpath );
    strcat( filename, "updatesequence.txt" );
    updateFile = fopen( filename, "w" );

    // Primal variables
    strcpy( filename, outpath );
    strcat( filename, "pv.m" );
    primalVarsFile = fopen( filename, "w");
    fprintf( primalVarsFile, "xi=[ " );

    // Dual variables
    strcpy( filename, outpath );
    strcat( filename, "dv.m" );
    dualVarsFile = fopen( filename, "w");
    fprintf( dualVarsFile, "lambda=[ " );
#endif

    itCount = 0;
    qpItTotal = 0;
    qpIterations = 0;
    hessSkipped = 0;
    hessDamped = 0;
    averageSizingFactor = 0.0;
}


/**
 * Print primal variables to a MATLAB file
 */
void SQPstats::printPrimalVars( const Matrix &xi )
{
    for( int i=0; i<xi.M()-1; i++ )
        fprintf( primalVarsFile, " %23.16e,", xi( i ) );
    fprintf( primalVarsFile, " %23.16e;\n", xi( xi.M()-1 ) );
}


/**
 * Print primal variables to a MATLAB file
 */
void SQPstats::printDualVars( const Matrix &lambda )
{
    for( int i=0; i<lambda.M()-1; i++ )
        fprintf( dualVarsFile, " %23.16e,", lambda( i ) );
    fprintf( dualVarsFile, " %23.16e;\n", lambda( lambda.M()-1 ) );
}


/**
 * Print dense Hessian in a MATLAB file
 */
void SQPstats::printHessian( int nBlocks, const SymMatrix *&hess )
{
    PATHSTR filename;
    int offset, i, j, iBlock, nVar;

    nVar = 0;
    for( iBlock=0; iBlock<nBlocks; iBlock++ )
        nVar += hess[iBlock].M();

    SymMatrix fullHessian;
    fullHessian.Dimension( nVar ).Initialize( 0.0 );

    strcpy( filename, outpath );
    strcat( filename, "hes.m" );
    hessFile = fopen( filename, "w");

    offset = 0;
    for( iBlock=0; iBlock<nBlocks; iBlock++ )
    {
        for( i=0; i<hess[iBlock].N(); i++ )
            for( j=i; j<hess[iBlock].N(); j++ )
                fullHessian( offset + i, offset + j ) = hess[iBlock]( i,j );

        offset += hess[iBlock].N();
    }

    fprintf( hessFile, "H=" );
    fullHessian.Print( hessFile, 23, 1 );
    fprintf( hessFile, "\n" );
    fclose( hessFile );
}


/**
 * Print sparse Hessian (in a MATLAB readable format)
 */
void SQPstats::printHessian( int nVar, double *hesNz, int *hesIndRow, int *hesIndCol )
{
    PATHSTR filename;

    strcpy( filename, outpath );
    strcat( filename, "hes.dat" );
    hessFile = fopen( filename, "w");

    printSparseMatlab( hessFile, nVar, nVar, hesNz, hesIndRow, hesIndCol );

    fprintf( hessFile, "\n" );
    fclose( hessFile );
}


/**
 * Print dense Jacobian in a MATLAB file
 */
void SQPstats::printJacobian( const Matrix &constrJac )
{
    PATHSTR filename;

    strcpy( filename, outpath );
    strcat( filename, "jac.m" );
    jacFile = fopen( filename, "w");

    fprintf( jacFile, "A=" );
    constrJac.Print( jacFile, 23, 1 );
    fprintf( jacFile, "\n" );

    fclose( jacFile );
}


/**
 * Print sparse Jacobian (in a MATLAB readable format)
 */
void SQPstats::printJacobian( int nCon, int nVar, double *jacNz, int *jacIndRow, int *jacIndCol )
{
    PATHSTR filename;

    strcpy( filename, outpath );
    strcat( filename, "jac.dat" );
    jacFile = fopen( filename, "w");

    printSparseMatlab( jacFile, nCon, nVar, jacNz, jacIndRow, jacIndCol );

    fprintf( jacFile, "\n" );
    fclose( jacFile );
}


/**
 * Print a sparse Matrix in (column compressed) to a MATLAB readable file
 */
void SQPstats::printSparseMatlab( FILE *file, int nRow, int nCol, double *nz, int *indRow, int *indCol )
{
    int i, j, count;

    count = 0;
    fprintf( file, "%i %i 0\n", nRow, nCol );
    for( i=0; i<nCol; i++ )
        for( j=indCol[i]; j<indCol[i+1]; j++ )
        {
            // +1 for MATLAB indices!
            fprintf( file, "%i %i %23.16e\n", indRow[count]+1, i+1, nz[count] );
            count++;
        }
}


void SQPstats::printDebug( Problemspec *prob, SQPiterate *vars )
{
#ifdef MYDEBUG
    printPrimalVars( vars->xi );
    printDualVars( vars->lambda );
#endif
}


/**
 * Call before leaving run() routine
 */
void SQPstats::finish( Problemspec *prob, SQPiterate *vars )
{
#ifdef MYDEBUG
    printDebug( prob, vars );
    fprintf( progressFile, "\n" );
    fclose( progressFile );
    fprintf( updateFile, "\n" );
    fclose( updateFile );
    fprintf( primalVarsFile, "];\n" );
    fclose( primalVarsFile );
    fprintf( dualVarsFile, "];\n" );
    fclose( dualVarsFile );
#endif
}


void SQPstats::printCppNull( FILE *outfile, char* varname )
{
    fprintf( outfile, "    double *%s = NULL;\n", varname );
}


void SQPstats::printVectorCpp( FILE *outfile, double *vec, int len, char* varname )
{
    int i;

    fprintf( outfile, "    double %s[%i] = { ", varname, len );
    for( i=0; i<len; i++ )
    {
        fprintf( outfile, "%23.16e", vec[i] );
        if( i != len-1 )
            fprintf( outfile, ", " );
        if( (i+1) % 10 == 0 )
            fprintf( outfile, "\n          " );
    }
    fprintf( outfile, " };\n\n" );
}


void SQPstats::printVectorCpp( FILE *outfile, int *vec, int len, char* varname )
{
    int i;

    fprintf( outfile, "    int %s[%i] = { ", varname, len );
    for( i=0; i<len; i++ )
    {
        fprintf( outfile, "%i", vec[i] );
        if( i != len-1 )
            fprintf( outfile, ", " );
        if( (i+1) % 15 == 0 )
            fprintf( outfile, "\n          " );
    }
    fprintf( outfile, " };\n\n" );
}


#ifdef QPSOLVER_QPOASES
void SQPstats::dumpQPCpp( Problemspec *prob, SQPiterate *vars, qpOASES::SQProblem *qp )
{
    int i, j;
    PATHSTR filename;
    FILE *outfile;
    int n = prob->nVar;
    int m = prob->nCon;

    // Print dimensions
    strcpy( filename, outpath );
    strcat( filename, "qpoases_dim.dat" );
    outfile = fopen( filename, "w" );
    fprintf( outfile, "%i %i\n", n, m );
    fclose( outfile );

    // Print Hessian
#ifdef QPSOLVER_SPARSE
    strcpy( filename, outpath );
    strcat( filename, "qpoases_H_sparse.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<prob->nVar+1; i++ )
        fprintf( outfile, "%i ", vars->hessIndCol[i] );
    fprintf( outfile, "\n" );

    for( i=0; i<vars->hessIndCol[prob->nVar]; i++ )
        fprintf( outfile, "%i ", vars->hessIndRow[i] );
    fprintf( outfile, "\n" );

    for( i=0; i<vars->hessIndCol[prob->nVar]; i++ )
        fprintf( outfile, "%23.16e ", vars->hessNz[i] );
    fprintf( outfile, "\n" );
    fclose( outfile );
#endif
    strcpy( filename, outpath );
    strcat( filename, "qpoases_H.dat" );
    outfile = fopen( filename, "w" );
    int blockCnt = 0;
    for( i=0; i<n; i++ )
    {
        for( j=0; j<n; j++ )
        {
            if( i == vars->blockIdx[blockCnt+1] )
                blockCnt++;
            if( j >= vars->blockIdx[blockCnt] && j < vars->blockIdx[blockCnt+1] )
                fprintf( outfile, "%23.16e ", vars->hess[blockCnt]( i - vars->blockIdx[blockCnt], j - vars->blockIdx[blockCnt] ) );
            else
                fprintf( outfile, "0.0 " );
        }
        fprintf( outfile, "\n" );
    }
    fclose( outfile );

    // Print gradient
    strcpy( filename, outpath );
    strcat( filename, "qpoases_g.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<n; i++ )
        fprintf( outfile, "%23.16e ", vars->gradObj( i ) );
    fprintf( outfile, "\n" );
    fclose( outfile );

    // Print Jacobian
    strcpy( filename, outpath );
    strcat( filename, "qpoases_A.dat" );
    outfile = fopen( filename, "w" );
#ifdef QPSOLVER_SPARSE
    // Also print dense Jacobian
    Matrix constrJacTemp;
    constrJacTemp.Dimension( prob->nCon, prob->nVar ).Initialize( 0.0 );
    for( i=0; i<prob->nVar; i++ )
        for( j=vars->jacIndCol[i]; j<vars->jacIndCol[i+1]; j++ )
            constrJacTemp( vars->jacIndRow[j], i ) = vars->jacNz[j];
    for( i=0; i<m; i++ )
    {
        for( j=0; j<n; j++ )
            fprintf( outfile, "%23.16e ", constrJacTemp( i, j ) );
        fprintf( outfile, "\n" );
    }
    fclose( outfile );
#else

    for( i=0; i<m; i++ )
    {
        for( j=0; j<n; j++ )
            if( vars->constrJac( i, j ) < myInf )
                fprintf( outfile, "%23.16e ", vars->constrJac( i, j ) );
            else
                fprintf( outfile, "%23.16e ", 0.0 );
        fprintf( outfile, "\n" );
    }
    fclose( outfile );
#endif

#ifdef QPSOLVER_SPARSE
    strcpy( filename, outpath );
    strcat( filename, "qpoases_A_sparse.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<prob->nVar+1; i++ )
        fprintf( outfile, "%i ", vars->jacIndCol[i] );
    fprintf( outfile, "\n" );

    for( i=0; i<vars->jacIndCol[prob->nVar]; i++ )
        fprintf( outfile, "%i ", vars->jacIndRow[i] );
    fprintf( outfile, "\n" );

    for( i=0; i<vars->jacIndCol[prob->nVar]; i++ )
        fprintf( outfile, "%23.16e ", vars->jacNz[i] );
    fprintf( outfile, "\n" );
    fclose( outfile );
#endif

    // Print variable lower bounds
    strcpy( filename, outpath );
    strcat( filename, "qpoases_lb.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<n; i++ )
        fprintf( outfile, "%23.16e ", vars->deltaBl( i ) );
    fprintf( outfile, "\n" );
    fclose( outfile );

    // Print variable upper bounds
    strcpy( filename, outpath );
    strcat( filename, "qpoases_ub.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<n; i++ )
        fprintf( outfile, "%23.16e ", vars->deltaBu( i ) );
    fprintf( outfile, "\n" );
    fclose( outfile );

    // Print constraint lower bounds
    strcpy( filename, outpath );
    strcat( filename, "qpoases_lbA.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<m; i++ )
        fprintf( outfile, "%23.16e ", vars->deltaBl( i+n ) );
    fprintf( outfile, "\n" );
    fclose( outfile );

    // Print constraint upper bounds
    strcpy( filename, outpath );
    strcat( filename, "qpoases_ubA.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<m; i++ )
        fprintf( outfile, "%23.16e ", vars->deltaBu( i+n ) );
    fprintf( outfile, "\n" );
    fclose( outfile );

    // Print active set
    qpOASES::Bounds b;
    qpOASES::Constraints c;
    qp->getBounds( b );
    qp->getConstraints( c );

    strcpy( filename, outpath );
    strcat( filename, "qpoases_as.dat" );
    outfile = fopen( filename, "w" );
    for( i=0; i<n; i++ )
        fprintf( outfile, "%i ", b.getStatus( i ) );
    fprintf( outfile, "\n" );
    for( i=0; i<m; i++ )
        fprintf( outfile, "%i ", c.getStatus( i ) );
    fprintf( outfile, "\n" );
    fclose( outfile );
}
#endif


void SQPstats::dumpQPMatlab( Problemspec *prob, SQPiterate *vars )
{
    Matrix temp;
    PATHSTR filename;
    FILE *qpFile;
    FILE *vecFile;

    // Print vectors g, lb, lu, lbA, luA
    strcpy( filename, outpath );
    strcat( filename, "vec.m" );
    vecFile = fopen( filename, "w");

    fprintf( vecFile, "g=" );
    vars->gradObj.Print( vecFile, 23, 1 );
    fprintf( vecFile, "\n\n" );

    temp.Submatrix( vars->deltaBl, prob->nVar, 1, 0, 0 );
    fprintf( vecFile, "lb=" );
    temp.Print( vecFile, 23, 1 );
    fprintf( vecFile, "\n\n" );

    temp.Submatrix( vars->deltaBu, prob->nVar, 1, 0, 0 );
    fprintf( vecFile, "lu=" );
    temp.Print( vecFile, 23, 1 );
    fprintf( vecFile, "\n\n" );

    temp.Submatrix( vars->deltaBl, prob->nCon, 1, prob->nVar, 0 );
    fprintf( vecFile, "lbA=" );
    temp.Print( vecFile, 23, 1 );
    fprintf( vecFile, "\n\n" );

    temp.Submatrix( vars->deltaBu, prob->nCon, 1, prob->nVar, 0 );
    fprintf( vecFile, "luA=" );
    temp.Print( vecFile, 23, 1 );
    fprintf( vecFile, "\n" );

    fclose( vecFile );

    // Print sparse Jacobian and Hessian
    #ifdef QPSOLVER_SPARSE
    printJacobian( prob->nCon, prob->nVar, vars->jacNz, vars->jacIndRow, vars->jacIndCol );
    printHessian( prob->nVar, vars->hessNz, vars->hessIndRow, vars->hessIndCol );
    #endif

    // Print a script that correctly reads everything
    strcpy( filename, outpath );
    strcat( filename, "getqp.m" );
    qpFile = fopen( filename, "w");

    fprintf( qpFile, "%% Read vectors g, lb, lu, lbA, luA\n" );
    fprintf( qpFile, "vec;\n" );
    fprintf( qpFile, "%% Read sparse Jacobian\n" );
    fprintf( qpFile, "load jac.dat\n" );
    fprintf( qpFile, "if jac(1) == 0\n" );
    fprintf( qpFile, "    A = [];\n" );
    fprintf( qpFile, "else\n" );
    fprintf( qpFile, "    A = spconvert( jac );\n" );
    fprintf( qpFile, "end\n" );
    fprintf( qpFile, "%% Read sparse Hessian\n" );
    fprintf( qpFile, "load hes.dat\n" );
    fprintf( qpFile, "H = spconvert( hes );\n" );

    fclose( qpFile );
}

/**
 * For tracking problem: save reference solution that has to be tracked
 *
 * \todo this should be a method of VplProblem
 */
//void SQPstats::setF1( VplProblem *prob )
//{
    //int count, iExOpt, iShoot, k, dimF1;

    //dimF1 = 0;
    //for( iExOpt=0; iExOpt<prob->nExOpt; iExOpt++ )
        //for( iShoot=0; iShoot<prob->expEval[iExOpt]->nShoot-1; iShoot++ )
            //dimF1 += prob->expEval[iExOpt]->measContrib[iShoot].M();
    //prob->refF1.Dimension( dimF1 ).Initialize( 0.0 );

    //count = 0;
    //for( iExOpt=0; iExOpt<prob->nExOpt; iExOpt++ )
        //for( iShoot=0; iShoot<prob->expEval[iExOpt]->nShoot-1; iShoot++ )
            //for( k=0; k<prob->expEval[iExOpt]->measContrib[iShoot].M(); k++ )
                //prob->refF1( count++ ) = prob->expEval[iExOpt]->measContrib[iShoot]( k );
//}

} // namespace blockSQP
