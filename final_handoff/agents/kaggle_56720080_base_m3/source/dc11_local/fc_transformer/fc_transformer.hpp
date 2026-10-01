#pragma once
// Causal transformer forecast of the opponent's hourly sales (work/sep26_wide_losses fc_tf.py, exported by fc_tf_export.py).
// Per product, one token per dawn (days 1-28): the 34 dawn features (fc_features.hpp; count features log-scaled), the
// opponent's hourly sales of the previous day (History flows, log1p; zero on day 1 as in training) and the product one-hot.
// Each day attends to itself and earlier days; keys/values of earlier days are cached, so a dawn costs one token per product.
// predict() returns expected units per hour for products 1-7 (carrot..wool); callers keep their own forecast for wheat,
// fertilizer, day 0 and day 29. Models trained by fc_tf_intra.py also have an intra-day head: intraday() re-forecasts the
// hours >= cut of today from the dawn state and the opponent's sales in the hours before cut.
#include "fc_features.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fcmodel {

class Transformer {
public:
    explicit Transformer(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("cannot open " + path);
        int32_t hdr[13]{};
        in.read(reinterpret_cast<char*>(hdr), 9 * sizeof(int32_t));
        if (hdr[0] == 0x46435432) in.read(reinterpret_cast<char*>(&hdr[9]), sizeof(int32_t));  // "FCT2": + xlag flag
        else if (hdr[0] == 0x46435433) in.read(reinterpret_cast<char*>(&hdr[9]), 2 * sizeof(int32_t));  // "FCT3": + xlag, stockin
        else if (hdr[0] == 0x46435434) in.read(reinterpret_cast<char*>(&hdr[9]), 3 * sizeof(int32_t));  // "FCT4": + xlag, stockin, xseen
        else if (hdr[0] == 0x46435435) in.read(reinterpret_cast<char*>(&hdr[9]), 4 * sizeof(int32_t));  // "FCT5": + next (tomorrow's hours)
        else if (hdr[0] != 0x46435446) throw std::runtime_error("bad transformer file " + path);
        stockin_ = hdr[10], xseen_ = hdr[11], next_ = hdr[12];
        nf_ = hdr[1], nl_ = hdr[2], np_ = hdr[3], w_ = hdr[4], layers_ = hdr[5], heads_ = hdr[6], nout_ = hdr[7], maxpos_ = hdr[8];
        xlag_ = hdr[9];
        if (nf_ != N_FEATURES || nl_ != 24 || np_ != 7 || nout_ != 24) throw std::runtime_error("unexpected transformer shape");
        nin_ = nf_ + nl_ + np_ + (xlag_ ? 24 : 0);
        auto read = [&](size_t n) {
            std::vector<float> v(n);
            in.read(reinterpret_cast<char*>(v.data()), std::streamsize(n * sizeof(float)));
            if (!in) throw std::runtime_error("truncated transformer file " + path);
            return v;
        };
        mu_ = read(nf_), sd_ = read(nf_), lag_mu_ = read(nl_), lag_sd_ = read(nl_);
        if (xlag_) xlag_mu_ = read(24), xlag_sd_ = read(24);
        inp_w_ = read(size_t(w_) * nin_), inp_b_ = read(w_), pos_ = read(size_t(maxpos_) * w_);
        layer_.resize(layers_);
        for (auto& l : layer_) {
            l.n1w = read(w_), l.n1b = read(w_), l.qkv_w = read(size_t(3) * w_ * w_), l.qkv_b = read(3 * w_);
            l.o_w = read(size_t(w_) * w_), l.o_b = read(w_), l.n2w = read(w_), l.n2b = read(w_);
            l.f1_w = read(size_t(4) * w_ * w_), l.f1_b = read(4 * w_), l.f2_w = read(size_t(4) * w_ * w_), l.f2_b = read(w_);
        }
        fnw_ = read(w_), fnb_ = read(w_), out_w_ = read(size_t(nout_) * w_), out_b_ = read(nout_);
        int32_t iw = 0;  // optional intra-day head: int32 width, then 3 linear layers (SiLU between)
        if (in.read(reinterpret_cast<char*>(&iw), sizeof iw) && iw > 0) {
            iw_ = iw;
            i0_w_ = read(size_t(iw_) * (w_ + 48 + 2 * stockin_ + 24 * xseen_)), i0_b_ = read(iw_), i1_w_ = read(size_t(iw_) * iw_), i1_b_ = read(iw_);
            i2_w_ = read(size_t(24 + next_) * iw_), i2_b_ = read(24 + next_);
        }
        reset();
    }
    void reset() {
        for (auto& c : cache_) c.assign(layers_, LayerCache{});
        tokens_ = 0;
    }
    int tokens() const { return tokens_; }
    bool has_intraday() const { return iw_ > 0; }
    bool has_stock() const { return stockin_; }
    bool has_xseen() const { return xseen_; }
    int next_hours() const { return next_; }  // FCT5: the intra-day head also predicts tomorrow's first next_hours() hours

    // Today's expected opponent units for hours >= cut of `product` given its sales `seen` in hours < cut and (stock
    // models) its inferred shed stock and visible output now, (xseen models) its sales of all products 1-7 in hours < cut
    // (after the dawn's predict()); out[h] for h < cut is left unchanged.
    void intraday(int product, const double seen[24], int cut, double out[24], double stock = 0, double visible = 0,
                  const double* all_seen = nullptr, double* next_out = nullptr) const {
        const auto& hid = hid_[product - 1];
        std::vector<float> x(w_ + 48 + 2 * stockin_ + 24 * xseen_), a(iw_), b(iw_);
        if (stockin_) x[w_ + 48] = float(std::log1p(std::max(0.0, stock))), x[w_ + 49] = float(std::log1p(std::max(0.0, visible)));
        if (xseen_)
            for (int h = 0; h < cut; ++h) x[w_ + 48 + 2 * stockin_ + h] = float(std::log1p(std::max(0.0, all_seen[h])));
        std::copy(hid.begin(), hid.end(), x.begin());
        for (int h = 0; h < 24; ++h) x[w_ + h] = h < cut ? float(std::log1p(std::max(0.0, seen[h]))) : 0.f, x[w_ + 24 + h] = h < cut;
        dense(i0_w_, i0_b_, x, a, true);
        dense(i1_w_, i1_b_, a, b, true);
        std::vector<float> lr(24 + next_);
        dense(i2_w_, i2_b_, b, lr, false);
        for (int h = cut; h < 24; ++h) out[h] = std::exp(double(lr[h]));
        if (next_out)
            for (int h = 0; h < next_; ++h) next_out[h] = std::exp(double(lr[24 + h]));  // tomorrow's hours 0..next_-1
    }

    // Standardized model input of one token from raw features (fc_features order), the previous day's hourly sales of
    // this product and (xlag models) of all products 1-7.
    std::vector<float> token(const std::array<double, N_FEATURES>& raw, const double lag[24], int product, const double* all = nullptr) const {
        static const bool is_count[N_FEATURES] = {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // s1-s3, y0-y3, cum, stock, visible, avail, txa, herd, herd_ready, own_stock, f_total
        std::vector<float> x(nin_, 0.f);
        for (int i = 0; i < nf_; ++i) {
            double v = raw[i];
            if (is_count[i]) v = (v < 0 ? -1 : 1) * std::log1p(std::abs(v));
            if (i == 21 || i == 22) v = std::log1p(std::max(0.0, v));  // money, opp_money
            x[i] = float((v - mu_[i]) / sd_[i]);
        }
        for (int h = 0; h < 24; ++h) x[nf_ + h] = float((std::log1p(std::max(0.0, lag[h])) - lag_mu_[h]) / lag_sd_[h]);
        x[nf_ + nl_ + product - 1] = 1.f;
        if (xlag_)
            for (int h = 0; h < 24; ++h) x[nf_ + nl_ + np_ + h] = float((std::log1p(std::max(0.0, all[h])) - xlag_mu_[h]) / xlag_sd_[h]);
        return x;
    }

    // Appends one day's token for `product` (days in order, day 1 first) and returns 24 log-rates.
    std::array<double, 24> step(int product, const std::vector<float>& x) {
        auto& cache = cache_[product - 1];
        const int t = int(cache[0].k.size() / w_);  // position of this token
        if (t >= maxpos_) throw std::runtime_error("transformer sequence too long");
        std::vector<float> h(w_), a(w_), q(w_), tmp(4 * w_);
        for (int o = 0; o < w_; ++o) {
            double s = inp_b_[o];
            const float* row = &inp_w_[size_t(o) * nin_];
            for (int i = 0; i < nin_; ++i) s += row[i] * x[i];
            h[o] = float(s + pos_[size_t(t) * w_ + o]);
        }
        const int hd = w_ / heads_;
        for (int li = 0; li < layers_; ++li) {
            const auto& l = layer_[li];
            auto& lc = cache[li];
            norm(h, l.n1w, l.n1b, a);
            // q, k, v of this token; k, v appended to the cache
            std::vector<float> kv(2 * w_);
            for (int o = 0; o < 3 * w_; ++o) {
                double s = l.qkv_b[o];
                const float* row = &l.qkv_w[size_t(o) * w_];
                for (int i = 0; i < w_; ++i) s += row[i] * a[i];
                if (o < w_) q[o] = float(s);
                else kv[o - w_] = float(s);
            }
            lc.k.insert(lc.k.end(), kv.begin(), kv.begin() + w_);
            lc.v.insert(lc.v.end(), kv.begin() + w_, kv.end());
            std::vector<float> att(w_, 0.f), score(t + 1);
            for (int hh = 0; hh < heads_; ++hh) {
                double top = -1e30;
                for (int j = 0; j <= t; ++j) {
                    double s = 0;
                    for (int i = 0; i < hd; ++i) s += q[hh * hd + i] * lc.k[size_t(j) * w_ + hh * hd + i];
                    score[j] = float(s / std::sqrt(double(hd)));
                    top = std::max(top, double(score[j]));
                }
                double sum = 0;
                for (int j = 0; j <= t; ++j) sum += score[j] = float(std::exp(score[j] - top));
                for (int j = 0; j <= t; ++j)
                    for (int i = 0; i < hd; ++i) att[hh * hd + i] += float(score[j] / sum) * lc.v[size_t(j) * w_ + hh * hd + i];
            }
            for (int o = 0; o < w_; ++o) {
                double s = l.o_b[o];
                const float* row = &l.o_w[size_t(o) * w_];
                for (int i = 0; i < w_; ++i) s += row[i] * att[i];
                h[o] += float(s);
            }
            norm(h, l.n2w, l.n2b, a);
            for (int o = 0; o < 4 * w_; ++o) {
                double s = l.f1_b[o];
                const float* row = &l.f1_w[size_t(o) * w_];
                for (int i = 0; i < w_; ++i) s += row[i] * a[i];
                tmp[o] = float(std::max(0.0, s));
            }
            for (int o = 0; o < w_; ++o) {
                double s = l.f2_b[o];
                const float* row = &l.f2_w[size_t(o) * 4 * w_];
                for (int i = 0; i < 4 * w_; ++i) s += row[i] * tmp[i];
                h[o] += float(s);
            }
        }
        hid_[product - 1].assign(h.begin(), h.end());  // the intra-day head's input
        norm(h, fnw_, fnb_, a);
        std::array<double, 24> out{};
        for (int o = 0; o < nout_; ++o) {
            double s = out_b_[o];
            const float* row = &out_w_[size_t(o) * w_];
            for (int i = 0; i < w_; ++i) s += row[i] * a[i];
            out[o] = s;
        }
        return out;
    }

    // Dawn of day d (1-28): today's expected opponent units per hour for products 1-7. `forecast` is the seller's own
    // forecast (a feature). Call on every dawn 1-28 in order (the cache holds one token per earlier day); reset() per game.
    void predict(const dc10::agent::AgentObservation& o, const dc10::History& h, const double forecast[dc10::HOURS][dc10::N_PRODUCTS],
                 double out[dc10::HOURS][dc10::N_PRODUCTS]) {
        const int d = o.day;
        if (d < 1 || d > 28) return;
        if (tokens_ != d - 1) throw std::runtime_error("transformer: dawns must be fed in order from day 1");
        double all[24]{};
        if (d >= 2)
            for (int p = dc10::CARROT; p <= dc10::WOOL; ++p)
                for (int hr = 0; hr < 24; ++hr) all[hr] += std::max(0, h.flow_at((d - 1) * dc10::HOURS + hr, p));
        for (int p = dc10::CARROT; p <= dc10::WOOL; ++p) {
            double lag[24]{};
            if (d >= 2)
                for (int hr = 0; hr < 24; ++hr) lag[hr] = std::max(0, h.flow_at((d - 1) * dc10::HOURS + hr, p));
            const auto lr = step(p, token(features(o, h, forecast, p), lag, p, all));
            for (int hr = 0; hr < 24; ++hr) out[hr][p] = std::exp(lr[hr]);
        }
        ++tokens_;
    }

private:
    struct Layer { std::vector<float> n1w, n1b, qkv_w, qkv_b, o_w, o_b, n2w, n2b, f1_w, f1_b, f2_w, f2_b; };
    struct LayerCache { std::vector<float> k, v; };
    static void dense(const std::vector<float>& w, const std::vector<float>& b, const std::vector<float>& x, std::vector<float>& y, bool silu) {
        const size_t n = x.size();
        for (size_t o = 0; o < y.size(); ++o) {
            double s = b[o];
            for (size_t i = 0; i < n; ++i) s += w[o * n + i] * x[i];
            y[o] = float(silu ? s / (1 + std::exp(-s)) : s);
        }
    }
    void norm(const std::vector<float>& x, const std::vector<float>& g, const std::vector<float>& b, std::vector<float>& y) const {
        double m = 0, s = 0;
        for (int i = 0; i < w_; ++i) m += x[i];
        m /= w_;
        for (int i = 0; i < w_; ++i) s += (x[i] - m) * (x[i] - m);
        const double inv = 1.0 / std::sqrt(s / w_ + 1e-5);
        for (int i = 0; i < w_; ++i) y[i] = float((x[i] - m) * inv * g[i] + b[i]);
    }
    int nf_ = 0, nl_ = 0, np_ = 0, w_ = 0, layers_ = 0, heads_ = 0, nout_ = 0, maxpos_ = 0, nin_ = 0, tokens_ = 0, xlag_ = 0, stockin_ = 0, xseen_ = 0, next_ = 0;
    std::vector<float> mu_, sd_, lag_mu_, lag_sd_, xlag_mu_, xlag_sd_, inp_w_, inp_b_, pos_, fnw_, fnb_, out_w_, out_b_;
    std::vector<Layer> layer_;
    std::vector<LayerCache> cache_[7];
    std::vector<float> hid_[7];  // last token's hidden state per product
    int iw_ = 0;
    std::vector<float> i0_w_, i0_b_, i1_w_, i1_b_, i2_w_, i2_b_;
};
}
