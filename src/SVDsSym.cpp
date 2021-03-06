#include <RcppEigen.h>
#include "ARPACK.h"
#include "EigsSym.h"
#include "MatOp.h"

using Rcpp::as;
using Rcpp::wrap;

// For symmetric matrices, svds() is equivalent to eigs(),
// so many steps can be simplified.
//
// Main function to calculate real, symmetric SVD
SEXP do_svds_sym(MatOp *op, SEXP n_scalar_r, SEXP k_scalar_r,
                 SEXP nu_scalar_r, SEXP nv_scalar_r,
                 SEXP params_list_r)
{
BEGIN_RCPP

    // We will use EigsSym to calculate the results,
    // but the parameters list is slightly different between
    // eigs() and svds().

    // Retrieve parameters
    Rcpp::List params_svds(params_list_r);
    int n = as<int>(n_scalar_r);
    int k = as<int>(k_scalar_r);
    int ncv = as<int>(params_svds["ncv"]);
    std::string which = "LM";
    int workmode = 1;
    char bmat = 'I';
    double tol = as<double>(params_svds["tol"]);
    int maxitr = as<int>(params_svds["maxitr"]);
    // Whether to calculate singular vectors or not.
    int nu = as<int>(nu_scalar_r);
    int nv = as<int>(nv_scalar_r);
    bool rvec = (nu > 0) || (nv > 0);

    EigsSym eig(n, k, ncv, op, which, workmode,
                bmat, tol, maxitr);
    eig.compute(rvec);
    Rcpp::List ret = eig.extract();

    int nconv = as<int>(ret["nconv"]);
    if (nconv < k)
        ::Rf_warning("only %d singular values converged, less than k", nconv);
    nu = nu > nconv ? nconv : nu;
    nv = nv > nconv ? nconv : nv;

    // Currently ret has components of values, vectors, nconv
    // and niter, while what we need is d, u, v, nconv and niter,
    // so we first insert the v matrix into the list, and then
    // change the list names.
    if (!rvec)
    {
        ret.insert(2, R_NilValue);
    } else {
        // At least one of nu and nv are not zero
        if (nu != 0)
        {
            Rcpp::NumericMatrix u = ret["vectors"];
            if (nv == 0)
                // nu != 0, nv == 0
                ret.insert(2, R_NilValue);
            else {
                // nu != 0, nv != 0
                Rcpp::NumericMatrix v(n, nv);
                // In SVD, V == U
                std::copy(u.begin(),
                          u.begin() + nv * n,
                          v.begin());
                ret.insert(2, v);
            }
            // u.erase(start, end) removes u[start <= i < end]
            u.erase(nu * n, nconv * n);
        } else {
            // nu == 0, nv != 0
            Rcpp::NumericMatrix u = ret["vectors"];
            Rcpp::NumericMatrix v(n, nv);
            std::copy(u.begin(),
                      u.begin() + nv * n,
                      v.begin());
            ret["vectors"] = R_NilValue;
            ret.insert(2, v);
        }
    }
    ret.names() = Rcpp::CharacterVector::create("d", "u", "v",
                                                "nconv", "niter");

    return ret;

END_RCPP
}

