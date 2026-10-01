#pragma once
// Learned opponent-sales forecast (scripts/train_forecast.py, exported by scripts/export_forecast.py): dc11's forecast
// plus an MLP correction from the dawn inputs (dc11_local/forecast_features.hpp). Used by the vendored dc11 agent when
// the model folder has a <model>.forecast file; replaces the day market's opponent flow (not the funding stress check).
#include "dc11_local/forecast_features.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace fcast {
class LearnedForecast {
public:
    bool load(const std::string& path) {
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) return false;
        int32_t n = 0;
        bool ok = std::fread(&n, 4, 1, f) == 1 && n > 0 && n < 16;
        for (int i = 0; ok && i < n; ++i) {
            Layer l;
            ok = std::fread(&l.rows, 4, 1, f) == 1 && std::fread(&l.cols, 4, 1, f) == 1;
            if (!ok) break;
            l.w.resize(size_t(l.rows) * l.cols), l.b.resize(l.rows);
            ok = std::fread(l.w.data(), 4, l.w.size(), f) == l.w.size() && std::fread(l.b.data(), 4, l.b.size(), f) == l.b.size();
            layers_.push_back(std::move(l));
        }
        std::fclose(f);
        return ok && layers_.front().cols == INPUTS && layers_.back().rows == GRID;
    }
    // rival: dc11's forecast in, the learned forecast out.
    void apply(const agent::AgentObservation& o, const History& h, double rival[HOURS][N_PRODUCTS]) const {
        if (o.day < 1 || o.day >= LAST_DAY) return;  // trained on days 1-28; dc11's forecast on days 0 and 29
        std::vector<float> x(INPUTS);
        inputs(o, h, rival, x.data());
        for (size_t i = 0; i < layers_.size(); ++i) {
            const Layer& l = layers_[i];
            std::vector<float> y(l.b);
            for (int r = 0; r < l.rows; ++r) {
                const float* w = &l.w[size_t(r) * l.cols];
                float s = 0;
                for (int c = 0; c < l.cols; ++c) s += w[c] * x[c];
                y[r] += s;
                if (i + 1 < layers_.size() && y[r] < 0) y[r] = 0;
            }
            x.swap(y);
        }
        for (int t = 0; t < HOURS; ++t)
            for (int p = 0; p < N_PRODUCTS; ++p) rival[t][p] += x[t * N_PRODUCTS + p];
    }

private:
    struct Layer {
        int32_t rows = 0, cols = 0;
        std::vector<float> w, b;
    };
    std::vector<Layer> layers_;
};
}  // namespace fcast
