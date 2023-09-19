#include "common.h"

#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

const size_t P = 11;

void helm_naive(
    real_t w[P],
    real_t L[P][P],
    real_t d[4],
    real_t u[P][P][P],
    real_t r[P][P][P]
)
{
    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        r[x][y][z] = d[0] * w[x] * w[y] * w[z] * u[x][y][z];
    }

    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        real_t accu = 0;
        for (size_t k = 0; k < P; ++k) {
            accu += L[x][k] * w[y] * w[z] * u[k][y][z];
        }
        r[x][y][z] += d[1] * accu;
    }

    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        real_t accu = 0;
        for (size_t k = 0; k < P; ++k) {
            accu += w[x] * L[y][k] * w[z] * u[x][k][z];
        }
        r[x][y][z] += d[2] * accu;
    }

    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        real_t accu = 0;
        for (size_t k = 0; k < P; ++k) {
            accu += w[x] * w[y] * L[z][k] * u[x][y][k];
        }
        r[x][y][z] += d[3] * accu;
    }
}

void helm_factor_impl(
    real_t w[P],
    real_t L[P][P],
    real_t d[4],
    real_t u[P][P][P],
    real_t L_hat[P][P],
    real_t M_u[P][P][P],
    real_t r[P][P][P]
)
{
    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        real_t M_u_xyz = w[x] * w[y] * w[z] * u[x][y][z];
        M_u[x][y][z] = M_u_xyz;
        r[x][y][z] = M_u_xyz * d[0];
    }

    for (size_t i = 0; i < P; ++i)
    for (size_t j = 0; j < P; ++j) {
        L_hat[i][j] = L[i][j] / w[j];
    }

    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        real_t accu = 0;
        for (size_t k = 0; k < P; ++k) {
            accu += L_hat[x][k] * M_u[k][y][z];
        }
        r[x][y][z] += d[1] * accu;
    }

    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        real_t accu = 0;
        for (size_t k = 0; k < P; ++k) {
            accu += L_hat[y][k] * M_u[x][k][z];
        }
        r[x][y][z] += d[2] * accu;
    }

    for (size_t x = 0; x < P; ++x)
    for (size_t y = 0; y < P; ++y)
    for (size_t z = 0; z < P; ++z) {
        real_t accu = 0;
        for (size_t k = 0; k < P; ++k) {
            accu += L_hat[z][k] * M_u[x][y][k];
        }
        r[x][y][z] += d[3] * accu;
    }
}

void helm_factor(
    real_t w[P],
    real_t L[P][P],
    real_t d[4],
    real_t u[P][P][P],
    real_t r[P][P][P]
)
{
    real_t* L_hat = make_empty(P*P);
    real_t* M_u = make_empty(P*P*P);

    helm_factor_impl(
        w,
        L,
        d,
        u,
        L_hat,
        M_u,
        r
    );
}

int main(int argc, const char* argv[])
{
    srandom(0xDEADBEEF);

    real_t* w = make_random(P);
    real_t* L = make_random(P*P);
    real_t* d = make_random(4);
    real_t* u = make_random(P*P*P);

    real_t* r1 = make_empty(P*P*P);
    helm_naive(w, L, d, u, r1);

    real_t* r2 = make_empty(P*P*P);
    helm_factor(w, L, d, u, r2);
    real_t mse2 = mse(r1, r2, P*P*P);
    printf("mse2 = %G\n", mse2);

    return EXIT_SUCCESS;
}
