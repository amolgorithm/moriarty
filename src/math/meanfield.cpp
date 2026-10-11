#include "alife/math/meanfield.hpp"

#include <algorithm>
#include <cmath>

namespace alife::math {

void GeneralizedLV::rhs(const std::vector<double>& x, std::vector<double>& dx) const {
    dx.assign(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        double s = r[i];
        for (std::size_t j = 0; j < n; ++j) s += A[i * n + j] * x[j];
        dx[i] = x[i] * s;
    }
}

void GeneralizedLV::stepRK4(std::vector<double>& x, double dt) const {
    std::vector<double> k1, k2, k3, k4, t(n);
    rhs(x, k1);
    for (std::size_t i = 0; i < n; ++i) t[i] = x[i] + 0.5 * dt * k1[i];
    rhs(t, k2);
    for (std::size_t i = 0; i < n; ++i) t[i] = x[i] + 0.5 * dt * k2[i];
    rhs(t, k3);
    for (std::size_t i = 0; i < n; ++i) t[i] = x[i] + dt * k3[i];
    rhs(t, k4);
    for (std::size_t i = 0; i < n; ++i) x[i] = std::max(0.0, x[i] + dt / 6.0 * (k1[i] + 2 * k2[i] + 2 * k3[i] + k4[i]));
}

std::vector<double> GeneralizedLV::equilibrium(bool& ok) const {
    // solve A x = -r with partial pivoting
    std::vector<double> M(A), b(n);
    for (std::size_t i = 0; i < n; ++i) b[i] = -r[i];
    ok = true;
    for (std::size_t c = 0; c < n; ++c) {
        std::size_t p = c;
        for (std::size_t i = c + 1; i < n; ++i)
            if (std::abs(M[i * n + c]) > std::abs(M[p * n + c])) p = i;
        if (std::abs(M[p * n + c]) < 1e-12) { ok = false; return std::vector<double>(n, 0.0); }
        if (p != c) {
            for (std::size_t j = 0; j < n; ++j) std::swap(M[c * n + j], M[p * n + j]);
            std::swap(b[c], b[p]);
        }
        for (std::size_t i = c + 1; i < n; ++i) {
            const double f = M[i * n + c] / M[c * n + c];
            for (std::size_t j = c; j < n; ++j) M[i * n + j] -= f * M[c * n + j];
            b[i] -= f * b[c];
        }
    }
    std::vector<double> x(n);
    for (std::size_t i = n; i-- > 0;) {
        double s = b[i];
        for (std::size_t j = i + 1; j < n; ++j) s -= M[i * n + j] * x[j];
        x[i] = s / M[i * n + i];
    }
    return x;
}

void ConsumerResource::stepRK4(double& R, double& C, double dt) const {
    auto f = [&](double r, double c, double& dr, double& dc) {
        const double fr = a * r / (1.0 + a * h * r);
        dr = g * r * (1.0 - r / K) - fr * c;
        dc = e * fr * c - m * c;
    };
    double a1, b1, a2, b2, a3, b3, a4, b4;
    f(R, C, a1, b1);
    f(R + 0.5 * dt * a1, C + 0.5 * dt * b1, a2, b2);
    f(R + 0.5 * dt * a2, C + 0.5 * dt * b2, a3, b3);
    f(R + dt * a3, C + dt * b3, a4, b4);
    R = std::max(0.0, R + dt / 6.0 * (a1 + 2 * a2 + 2 * a3 + a4));
    C = std::max(0.0, C + dt / 6.0 * (b1 + 2 * b2 + 2 * b3 + b4));
}

}  // namespace alife::math
