#include "agent/bc_opus/source/agent.hpp"
#include "source/features.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <random>

namespace kag::agents::bc_opus {
using namespace dc10;

// fp16 network files (scripts/fp16_model.py): a leading int32 magic, then the fp32 layout with 2-byte weights.
constexpr int32_t kHalfMagic = 0x36314648;

float half_to_float(uint16_t h) {
    const int exponent = (h >> 10) & 0x1f, mantissa = h & 0x3ff;
    const float magnitude = exponent == 0    ? std::ldexp(float(mantissa), -24)
                            : exponent == 31 ? (mantissa ? NAN : INFINITY)
                                             : std::ldexp(float(mantissa | 0x400), exponent - 25);
    return h & 0x8000 ? -magnitude : magnitude;
}

bool read_floats(std::FILE* f, float* out, size_t n, bool half) {
    if (!half) return std::fread(out, 4, n, f) == n;
    std::vector<uint16_t> buffer(n);
    if (std::fread(buffer.data(), 2, n, f) != n) return false;
    for (size_t i = 0; i < n; ++i) out[i] = half_to_float(buffer[i]);
    return true;
}

bool Model::load(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    int32_t n = 0;
    bool ok = std::fread(&n, 4, 1, f) == 1;
    const bool half = ok && n == kHalfMagic;
    if (half) ok = std::fread(&n, 4, 1, f) == 1;
    if (ok && n < 0) {  // conv section first
        convs.assign(-n, {});
        for (auto& c : convs) {
            int32_t shape[3];
            ok = ok && std::fread(shape, 4, 3, f) == 3;
            if (!ok) break;
            c.out = shape[0], c.in = shape[1], c.k = shape[2];
            c.w.resize(size_t(c.out) * c.in * c.k * c.k);
            c.b.resize(c.out);
            ok = read_floats(f, c.w.data(), c.w.size(), half) && read_floats(f, c.b.data(), c.b.size(), half);
        }
        ok = ok && std::fread(&n, 4, 1, f) == 1;
    }
    layers.assign(ok ? n : 0, {});
    for (auto& layer : layers) {
        int32_t shape[2];
        ok = ok && std::fread(shape, 4, 2, f) == 2;
        if (!ok) break;
        layer.rows = shape[0], layer.cols = shape[1];
        layer.w.resize(size_t(layer.rows) * layer.cols);
        layer.b.resize(layer.rows);
        ok = read_floats(f, layer.w.data(), layer.w.size(), half) && read_floats(f, layer.b.data(), layer.b.size(), half);
    }
    std::fclose(f);
    if (std::FILE* opening_file = std::fopen((path + ".opening").c_str(), "r")) {
        const bool read = std::fscanf(opening_file, "%d %d", &opening_days, &opening_style) == 2;
        std::fclose(opening_file);
        if (!read) return false;
    }
    gblind_first = 0, gblind_last = -1;
    if (std::FILE* gblind_file = std::fopen((path + ".gblind").c_str(), "r")) {  // step 62
        const bool read = std::fscanf(gblind_file, "%d %d", &gblind_first, &gblind_last) == 2;
        std::fclose(gblind_file);
        if (!read || gblind_first < 0 || gblind_last >= bcopus::GLOBAL) return false;
    }
    land_scale = 0;
    if (std::FILE* land_file = std::fopen((path + ".landcond").c_str(), "r")) {  // step 52
        const bool read = std::fscanf(land_file, "%f", &land_scale) == 1;
        std::fclose(land_file);
        if (!read || land_scale <= 0) return false;
    }
    if (std::FILE* condition_file = std::fopen((path + ".condition").c_str(), "r")) {
        conditioned = std::fscanf(condition_file, "%f %f %f", &condition[0], &condition[1], &condition[2]) == 3;
        std::fclose(condition_file);
        if (!conditioned) return false;
    }
    if (std::FILE* features_file = std::fopen((path + ".features").c_str(), "r")) {
        if (std::fscanf(features_file, "%d", &features) != 1) features = -1;
        std::fclose(features_file);
    }
    goals.clear();
    goals_raw.clear();
    gridblind_first = 0, gridblind_last = -1;
    if (std::FILE* gridblind_file = std::fopen((path + ".gridblind").c_str(), "r")) {
        const bool read = std::fscanf(gridblind_file, "%d %d", &gridblind_first, &gridblind_last) == 2;
        std::fclose(gridblind_file);
        if (!read || gridblind_first < 0 || gridblind_last >= bcopus::GRID_CHANNELS) return false;
    }
    gridoff_first = 0, gridoff_last = -1;
    if (std::FILE* gridoff_file = std::fopen((path + ".gridoff").c_str(), "r")) {
        const bool read = std::fscanf(gridoff_file, "%d %d", &gridoff_first, &gridoff_last) == 2;
        std::fclose(gridoff_file);
        if (!read) return false;
    }
    if (std::FILE* goal_file = std::fopen((path + ".goalraw").c_str(), "r")) {
        for (std::array<float, 7> r; std::fscanf(goal_file, "%f %f %f %f %f %f %f", &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6]) == 7;)
            goals_raw.push_back(r);
        std::fclose(goal_file);
        if (goals_raw.empty()) return false;
    }
    if (std::FILE* goal_file = std::fopen((path + ".goal").c_str(), "r")) {
        for (std::array<float, 5> r; std::fscanf(goal_file, "%f %f %f %f %f", &r[0], &r[1], &r[2], &r[3], &r[4]) == 5;) goals.push_back(r);
        std::fclose(goal_file);
        if (goals.empty()) return false;
    }
    if (std::FILE* style_file = std::fopen((path + ".style").c_str(), "r")) {
        if (std::fscanf(style_file, "%d", &style) != 1) style = -1;
        std::fclose(style_file);
    }
    return ok && layers.size() == 19 && features >= 3 && features <= bcopus::FEATURES_VERSION;
}

namespace {
constexpr int CLASSES = 101;

void linear(const Linear& l, const float* x, float* y, bool relu) {
    for (int r = 0; r < l.rows; ++r) {
        float s = l.b[r];
        const float* w = &l.w[size_t(r) * l.cols];
        for (int c = 0; c < l.cols; ++c) s += w[c] * x[c];
        y[r] = relu && s < 0 ? 0 : s;
    }
}

// Linear-ReLU-Linear-ReLU-Linear starting at layer `first`; `relu_out` applies the
// caller's activation on the output (encoders and context).
std::vector<float> mlp(const Model& m, int first, const float* x, bool relu_out) {
    std::vector<float> h1(m.layers[first].rows), h2(m.layers[first + 1].rows), y(m.layers[first + 2].rows);
    linear(m.layers[first], x, h1.data(), true);
    linear(m.layers[first + 1], h1.data(), h2.data(), true);
    linear(m.layers[first + 2], h2.data(), y.data(), relu_out);
    return y;
}

double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

// 3x3 same-padding convolution on a 10x10 grid, then ReLU.
void conv(const Conv& c, const float* x, float* y) {
    for (int o = 0; o < c.out; ++o)
        for (int yy = 0; yy < BOARD; ++yy)
            for (int xx = 0; xx < BOARD; ++xx) {
                float sum = c.b[o];
                for (int i = 0; i < c.in; ++i)
                    for (int ky = 0; ky < 3; ++ky) {
                        const int sy = yy + ky - 1;
                        if (sy < 0 || sy >= BOARD) continue;
                        for (int kx = 0; kx < 3; ++kx) {
                            const int sx = xx + kx - 1;
                            if (sx < 0 || sx >= BOARD) continue;
                            sum += c.w[((size_t(o) * c.in + i) * 3 + ky) * 3 + kx] * x[i * 100 + sy * BOARD + sx];
                        }
                    }
                y[o * 100 + yy * BOARD + xx] = sum > 0 ? sum : 0;
            }
}

// Tile-CNN summary of one farm: per-channel mean then max (as scripts/train.py GridNet).
void grid_summary(const Model& m, const agent::PublicFarm& farm, int day, float* out) {
    std::vector<float> x(bcopus::GRID_CHANNELS * 100), h1(m.convs[0].out * 100), h2(m.convs[1].out * 100);
    if (day < m.gridoff_first || day > m.gridoff_last) bcopus::grid_features(farm, day, x.data());  // <model>.gridoff: zeros
    for (int ch = m.gridblind_first; ch <= m.gridblind_last; ++ch) std::fill_n(x.data() + ch * 100, 100, 0.0f);  // <model>.gridblind
    conv(m.convs[0], x.data(), h1.data());
    conv(m.convs[1], h1.data(), h2.data());
    const int ch = m.convs[1].out;
    for (int o = 0; o < ch; ++o) {
        float sum = 0, top = -1e30f;
        for (int k = 0; k < 100; ++k) sum += h2[o * 100 + k], top = std::max(top, h2[o * 100 + k]);
        out[o] = sum / 100, out[ch + o] = top;
    }
}

// Most likely count in 0..upper from one 101-way count head.
int best_count(const float* logits, int upper) {
    int best = 0;
    for (int c = 1; c <= std::min(upper, CLASSES - 1); ++c)
        if (logits[c] > logits[best]) best = c;
    static const bool sample = std::getenv("BC_OPT_SAMPLE") != nullptr;  // diagnostic (BC, G2): sample the count instead of the mode
    if (sample) {
        thread_local std::mt19937 rng(12345);
        const int hi = std::min(upper, CLASSES - 1);
        double z = 0;
        for (int c = 0; c <= hi; ++c) z += std::exp(double(logits[c] - logits[best]));
        double u = std::uniform_real_distribution<double>(0, z)(rng);
        for (int c = 0; c <= hi; ++c)
            if ((u -= std::exp(double(logits[c] - logits[best]))) <= 0) return c;
        return hi;
    }
    return best;
}

// Exact MAP partition: counts for `k` fields (101-way log-probability heads; fixed
// fields forced to 0) that sum to `total`, maximizing the summed log-probability.
void map_partition(const float* const* heads, const bool* fixed, int k, int total, int* out) {
    std::vector<double> best((k + 1) * (total + 1), -1e300);
    std::vector<int> choice(k * (total + 1), 0);
    best[k * (total + 1) + 0] = 0;  // after the last field nothing may remain
    for (int f = k - 1; f >= 0; --f) {
        double top = -1e30, norm = 0;
        for (int c = 0; c <= total; ++c) top = std::max(top, double(heads[f][c]));
        for (int c = 0; c <= total; ++c) norm += std::exp(heads[f][c] - top);
        for (int r = 0; r <= total; ++r) {
            double& b = best[f * (total + 1) + r];
            for (int c = 0; c <= (fixed[f] ? 0 : r); ++c) {
                const double lp = fixed[f] ? 0 : heads[f][c] - top - std::log(norm);
                const double v = lp + best[(f + 1) * (total + 1) + r - c];
                if (v > b) b = v, choice[f * (total + 1) + r] = c;
            }
        }
    }
    for (int f = 0, r = total; f < k; ++f) r -= out[f] = choice[f * (total + 1) + r];
}

// Whole-group mode: none / all / partial (then the member fraction).
int mode_count(const float* logits, float fraction_logit, int n) {
    int best = 0;
    for (int k = 1; k < 3; ++k)
        if (logits[k] > logits[best]) best = k;
    if (best == 0) return 0;
    if (best == 1) return n;
    return std::clamp(int(std::lround(n * sigmoid(fraction_logit))), 0, n);
}

double knob(const char* name, double fallback) {
    const char* v = std::getenv(name);
    return v && *v ? std::atof(v) : fallback;
}

// Lowers counts (logits of head i at heads[i]) one unit at a time, the unit whose
// removal loses the least log-probability first, until they sum to at most cap.
void cap_counts(const float* const* heads, int* counts, int k, int cap) {
    int sum = 0;
    for (int i = 0; i < k; ++i) sum += counts[i];
    for (; sum > cap; --sum) {
        int best = -1;
        double loss = 1e300;
        for (int i = 0; i < k; ++i)
            if (counts[i] > 0 && heads[i][counts[i]] - heads[i][counts[i] - 1] < loss)
                loss = heads[i][counts[i]] - heads[i][counts[i] - 1], best = i;
        --counts[best];
    }
}

// Integer counts summing to `total` from probabilities over unfixed entries
// (largest remainder; ties to the lower index).
void apportion(const double* p, const bool* fixed, int k, int total, int* out) {
    double sum = 0;
    for (int i = 0; i < k; ++i) out[i] = 0, sum += fixed[i] ? 0 : p[i];
    if (total <= 0) return;
    int first = -1;
    for (int i = 0; i < k && first < 0; ++i)
        if (!fixed[i]) first = i;
    if (first < 0) return;
    if (sum <= 0) {
        out[first] = total;
        return;
    }
    double frac[16];
    int given = 0;
    for (int i = 0; i < k; ++i) {
        const double share = fixed[i] ? 0 : p[i] / sum * total;
        out[i] = int(share);
        frac[i] = fixed[i] ? -1 : share - out[i];
        given += out[i];
    }
    for (; given < total; ++given) {
        int best = first;
        for (int i = 0; i < k; ++i)
            if (frac[i] > frac[best]) best = i;
        ++out[best];
        frac[best] = -1;
    }
}

void softmax_masked(const float* logits, const bool* fixed, int k, double* p) {
    double top = -1e30;
    for (int i = 0; i < k; ++i)
        if (!fixed[i]) top = std::max(top, double(logits[i]));
    for (int i = 0; i < k; ++i) p[i] = fixed[i] ? 0 : std::exp(logits[i] - top);
}
}

DayIntent decode_intent(const Model& m, const agent::AgentObservation& dawn, const History& history, std::string* invalid,
                        std::vector<float>* global_logits, const DecodeKnobs& knobs) {
    const int W = m.layers[0].rows;  // encoder width
    Schema s = describe(dawn);
    double rival[HOURS][N_PRODUCTS];
    history.expected(dawn.day, 3, rival);
    float g_in[bcopus::GLOBAL];
    bcopus::global_features(dawn, s, rival, g_in, m.features);
    for (int i = m.gblind_first; i <= m.gblind_last; ++i) g_in[i] = 0.0f;  // <model>.gblind (step 62)
    // Teacher style pin (models trained with --style): index from data/styles.json, 0 = unknown.
    {
        const char* style = std::getenv("BC_OPUS_STYLE");
        const int index = knobs.style >= 0 ? knobs.style : style && *style ? std::atoi(style) : m.style;
        if (index >= 0 && index < 32) g_in[224 + index] = 1.0f;
    }
    if (m.conditioned) {  // BC_OPUS_STRENGTH: strength (rating points over the ladder reference)
        std::copy_n(m.condition, 3, g_in + 216);
        if (const char* strength = std::getenv("BC_OPUS_STRENGTH"); strength && *strength) g_in[217] = float(std::atof(strength) / 200.0);
    }
    for (const auto& r : m.goals)  // <model>.goal: hindsight goal inputs (known, cows6 / 4, animals14 / 20, q4)
        if (dawn.day >= r[0] && dawn.day <= r[1]) {
            g_in[219] = 1.0f, g_in[220] = r[2] / 4.0f, g_in[221] = r[3] / 20.0f, g_in[222] = r[4];
            break;
        }
    for (const auto& r : m.goals_raw)  // <model>.goalraw
        if (dawn.day >= r[0] && dawn.day <= r[1]) {
            std::copy_n(r.data() + 2, 5, g_in + 219);
            break;
        }
    if (m.land_scale > 0) g_in[215] = knobs.land_input * m.land_scale;  // today's land decision (step 52)
    // Probe (BC, DC11_INDUMP=<file>, DC11_INDUMP_DAY=<d | a-b | all>, default 10): the network's raw inputs at those dawns, appended as
    // float32: header (day, q, n_crops, n_animals, GLOBAL, CROP, ANIMAL, own money, opponent money, seat), global, crops, animals, own
    // grid, opponent grid (scripts/day_blockswap.py reads it; one record per decode call, both seats if both use this decoder).
    if (const char* dump = std::getenv("DC11_INDUMP"); dump && *dump && !knobs.logits_only) {
        const char* dd = std::getenv("DC11_INDUMP_DAY");
        int lo = 10, hi = 10;
        if (dd && *dd) {
            if (std::string(dd) == "all") lo = 0, hi = LAST_DAY;
            else if (std::sscanf(dd, "%d-%d", &lo, &hi) == 1) hi = lo;
        }
        if (dawn.day >= lo && dawn.day <= hi) {
            std::vector<float> rec = {float(dawn.day), float(knobs.q_animal), float(s.n_crops), float(s.n_animals), float(bcopus::GLOBAL),
                                      float(bcopus::CROP), float(bcopus::ANIMAL), float(dawn.self().money), float(dawn.opponent().money),
                                      float(dawn.player)};
            rec.insert(rec.end(), g_in, g_in + bcopus::GLOBAL);
            std::vector<float> f(std::max(bcopus::CROP, bcopus::ANIMAL));
            for (int i = 0; i < s.n_crops; ++i) {
                bcopus::crop_features(s, s.crops[i], f.data(), dawn.market.prices[s.crops[i].crop]);
                rec.insert(rec.end(), f.begin(), f.begin() + bcopus::CROP);
            }
            for (int i = 0; i < s.n_animals; ++i) {
                bcopus::animal_features(s, s.animals[i], f.data(), dawn.market.prices[ANIMALS[s.animals[i].species].product]);
                rec.insert(rec.end(), f.begin(), f.begin() + bcopus::ANIMAL);
            }
            std::vector<float> grid(bcopus::GRID_CHANNELS * 100);
            for (const auto* farm : {&dawn.self(), &dawn.opponent()}) {
                bcopus::grid_features(*farm, dawn.day, grid.data());
                rec.insert(rec.end(), grid.begin(), grid.end());
            }
            static std::mutex dump_mutex;
            std::lock_guard<std::mutex> lock(dump_mutex);
            FILE* out = std::fopen(dump, "ab");
            std::fwrite(rec.data(), sizeof(float), rec.size(), out);
            std::fclose(out);
        }
    }
    const auto g = mlp(m, 0, g_in, true);
    // Existing groups: encodings and pooled context (new groups are not pooled).
    std::vector<std::vector<float>> crop_enc(s.n_crops), animal_enc(s.n_animals);
    const int grid_width = m.convs.empty() ? 0 : 4 * m.convs[1].out;
    std::vector<float> ctx_in(7 * W + grid_width, 0.0f);
    if (grid_width) {
        grid_summary(m, dawn.self(), dawn.day, &ctx_in[7 * W]);
        grid_summary(m, dawn.opponent(), dawn.day, &ctx_in[7 * W + grid_width / 2]);
    }
    std::copy(g.begin(), g.end(), ctx_in.begin());
    auto pool = [&](std::vector<std::vector<float>>& enc, int count, auto fresh_of, auto features_of, int first, int offset) {
        std::vector<float> mean(W, 0.0f), top(W, -1e9f), sum(W, 0.0f);
        int kept = 0;
        for (int i = 0; i < count; ++i) {
            if (fresh_of(i)) continue;
            enc[i] = mlp(m, first, features_of(i).data(), true);
            ++kept;
            for (int j = 0; j < W; ++j) mean[j] += enc[i][j], sum[j] += enc[i][j], top[j] = std::max(top[j], enc[i][j]);
        }
        for (int j = 0; j < W; ++j) {
            ctx_in[offset + j] = kept ? mean[j] / kept : 0.0f;
            ctx_in[offset + W + j] = kept ? top[j] : 0.0f;
            ctx_in[offset + 2 * W + j] = sum[j] / 10.0f;
        }
    };
    auto crop_in = [&](int i) {
        std::vector<float> f(bcopus::CROP);
        bcopus::crop_features(s, s.crops[i], f.data(), dawn.market.prices[s.crops[i].crop]);
        return f;
    };
    auto animal_in = [&](int i) {
        std::vector<float> f(bcopus::ANIMAL);
        bcopus::animal_features(s, s.animals[i], f.data(), dawn.market.prices[ANIMALS[s.animals[i].species].product]);
        return f;
    };
    pool(crop_enc, s.n_crops, [&](int i) { return s.crops[i].fresh; }, crop_in, 3, W);
    pool(animal_enc, s.n_animals, [&](int i) { return s.animals[i].fresh; }, animal_in, 6, 4 * W);
    const auto ctx = mlp(m, 9, ctx_in.data(), true);
    std::vector<float> head(m.layers[12].rows);
    linear(m.layers[12], ctx.data(), head.data(), false);
    if (global_logits) *global_logits = head;
    if (knobs.logits_only) return {};
    if (knobs.head_override && knobs.head_override->size() == head.size()) head = *knobs.head_override;

    // Deterministic pseudo-random numbers in (0, 1) from the dawn state (knobs.sample).
    auto draw = [&](uint64_t salt) {
        uint64_t x = salt * 0x9E3779B97F4A7C15ull ^ uint64_t(dawn.step) << 20 ^ uint64_t(dawn.player) << 40 ^
                     uint64_t(std::llround(dawn.self().money)) ^ uint64_t(std::llround(dawn.opponent().money)) << 24;
        x ^= x >> 30, x *= 0xBF58476D1CE4E5B9ull, x ^= x >> 27, x *= 0x94D049BB133111EBull, x ^= x >> 31;
        return (double(x >> 11) + 0.5) / double(1ull << 53);
    };
    // One decode of the whole intent with at most `cap` new crops and animals.
    auto decode = [&](int cap) {
        // Whole-farm fields (fixed on day 29).
        DayIntent in{};
        // .decode optrate (step 68): the count heads' logits per group and option kind (0 ongoing harvest, 1 ongoing fertilize,
        // 2 feed, 3 care, 4 collect), reallocated after the group loops.
        struct Head { int kind, group; std::vector<float> logits; };
        std::vector<Head> heads_seen;
        struct OneShot { int group; double e[OPTIONS]; bool fixed[OPTIONS]; };  // expected option counts (sum = group size)
        std::vector<OneShot> oneshots;
        auto keep = [&](int kind, int group, const float* logits) {
            if (knobs.opt_rate) heads_seen.push_back({kind, group, std::vector<float>(logits, logits + CLASSES)});
        };
        const bool terminal = dawn.day == LAST_DAY;
        int counts[11];
        // Counts decode at the median of the predicted distribution. The mode collapses to
        // 0 whenever "none" is the single most likely value even if most mass is on
        // 10-25 (BC_COUNT_MODE=argmax restores the mode for comparison).
        const bool mode = std::getenv("BC_COUNT_MODE") && std::string(std::getenv("BC_COUNT_MODE")) == "argmax";
        for (int f = 0; f < 11; ++f) {
            const float* l = &head[f * CLASSES];
            int best = 0;
            for (int c = 1; c < CLASSES; ++c)
                if (l[c] > l[best]) best = c;
            if (!mode) {
                double total = 0, p[CLASSES];
                for (int c = 0; c < CLASSES; ++c) total += p[c] = std::exp(double(l[c] - l[best]));
                double cumulative = 0;
                for (best = 0; best < CLASSES - 1; ++best)
                    if ((cumulative += p[best]) >= 0.5 * total) break;
            }
            counts[f] = best;
        }
        // Factorized heads (when present): total new crops / animals at the median of the
        // total distribution, split by the predicted type shares (largest remainder).
        const int factor_base = 11 * CLASSES + 1;
        const bool factored = int(head.size()) == factor_base + 2 * CLASSES + 8 && !std::getenv("BC_PER_TYPE") && !knobs.per_type;
        // Totals above the sites possible today were masked in training (features v4).
        const int most = m.features >= 4 ? std::min(CLASSES - 1, int(std::lround(g_in[bcopus::SITES + 6] * 50))) : CLASSES - 1;
        // Median of the total distribution (BC_Q_CROP / BC_Q_ANIMAL: another quantile).
        auto median = [&](int offset, double q = 0.5) {
            double top = -1e30, total = 0, p[CLASSES];
            for (int c = 0; c <= most; ++c) top = std::max(top, double(head[offset + c]));
            for (int c = 0; c <= most; ++c) total += p[c] = std::exp(head[offset + c] - top);
            double cumulative = 0;
            int c = 0;
            for (; c < most; ++c)
                if ((cumulative += p[c]) >= q * total) break;
            return c;
        };
        if (factored && !terminal) {
            const int offsets[2] = {factor_base, factor_base + CLASSES + N_CROPS};
            int totals[2] = {median(offsets[0], knobs.sample ? draw(1) : knobs.q_crop),
                             median(offsets[1], knobs.sample ? draw(2) : knobs.q_animal)};
            const float* heads[2] = {&head[offsets[0]], &head[offsets[1]]};
            cap_counts(heads, totals, 2, cap);
            double shares[2][5];
            for (int t = 0; t < 2; ++t) {
                const int first = t ? 5 : 0, k = t ? N_ANIMALS : N_CROPS;
                const bool fixed[5] = {};
                double* p = shares[t];
                float biased[5];
                for (int i = 0; i < k; ++i) biased[i] = head[offsets[t] + CLASSES + i] + (t ? knobs.animal_bias[i] : knobs.crop_bias[i]);
                softmax_masked(biased, fixed, k, p);
                int n[5];
                apportion(p, fixed, k, totals[t], n);
                for (int i = 0; i < k; ++i) counts[first + i] = n[i];
            }
            // Budget revision preferences: removing the k-th unit of type i (others fixed) from
            // total T changes the log-likelihood by lp(T-1) - lp(T) - log s_i + log(n_i / T).
            for (int i = 0; i < 8; ++i) {
                const int t = i >= 5, j = t ? i - 5 : i;
                int T = 0;
                for (int q = t ? 5 : 0; q < (t ? 8 : 5); ++q) T += counts[q];
                for (int k = 1; k <= counts[i] && k <= 32; ++k, --T)
                    in.drop_loss[i][k - 1] = float(head[offsets[t] + T] - head[offsets[t] + T - 1] + std::log(shares[t][j]) -
                                                   std::log(double(counts[i] - k + 1) / T));
            }
            in.has_drop_loss = true;
            // Budget trim order: removing one unit of type i from total T changes the
            // log-likelihood by lp(T-1) - lp(T) - log s_i + log(n_i / T); least loss first (the
            // unit the network is least likely to want; per dollar it drops the day-0 sheep first,
            // -1.4k in work/sep25_bc_weakness X6, vs +5.3k for this order, X10).
            int n[8], total[2] = {};
            for (int i = 0; i < 8; ++i) total[i >= 5] += n[i] = counts[i];
            while (in.trim_count < int(in.trim_order.size())) {
                int best = -1;
                double ratio = 1e300;
                for (int i = 0; i < 8; ++i) {
                    if (!n[i]) continue;
                    const int t = i >= 5, j = t ? i - 5 : i, T = total[t];
                    const double loss = head[offsets[t] + T] - head[offsets[t] + T - 1] + std::log(shares[t][j]) - std::log(double(n[i]) / T);
                    if (loss < ratio) ratio = loss, best = i;
                }
                if (best < 0) break;
                in.trim_order[in.trim_count++] = int8_t(best);
                --n[best], --total[best >= 5];
            }
        } else if (!terminal) {
            const float* heads[8];
            for (int f = 0; f < 8; ++f) heads[f] = &head[f * CLASSES];
            cap_counts(heads, counts, 8, cap);
        }
        for (int c = 0; c < N_CROPS; ++c) in.new_crop[c] = int16_t(terminal ? 0 : counts[c]);
        for (int a = 0; a < N_ANIMALS; ++a) {
            in.new_animal[a] = int16_t(terminal ? 0 : counts[5 + a]);
            in.reserve[a] = int16_t(terminal ? s.unplaced_dawn[a] : counts[8 + a]);
            // The compiler never discards animals: new + reserve >= unplaced at dawn.
            if (in.new_animal[a] + in.reserve[a] < s.unplaced_dawn[a]) in.reserve[a] = int16_t(s.unplaced_dawn[a] - in.new_animal[a]);
        }
        in.buy_land = (knobs.sample ? draw(3) < 1 / (1 + std::exp(-head[11 * CLASSES]))
                                    : head[11 * CLASSES] + knobs.land_bias > 0) &&
                      dawn.self().n_quadrants < std::min(4, knobs.max_quadrants);
        size_new_groups(s, in);
        // Stage A as fed to the group heads (same order and scale as scripts/train.py).
        float stage[12];
        for (int c = 0; c < N_CROPS; ++c) stage[c] = in.new_crop[c] / 20.0f;
        for (int a = 0; a < N_ANIMALS; ++a) stage[5 + a] = in.new_animal[a] / 5.0f, stage[8 + a] = in.reserve[a] / 5.0f;
        stage[11] = in.buy_land;

        // Group fields. New groups are encoded after their sizes are known.
        for (int i = 0; i < s.n_crops; ++i) {
            const CropGroup& grp = s.crops[i];
            const int size = grp.size;
            if (size == 0) continue;
            if (grp.fresh) crop_enc[i] = mlp(m, 3, crop_in(i).data(), true);
            std::vector<float> x(crop_enc[i]);
            x.insert(x.end(), ctx.begin(), ctx.end());
            x.insert(x.end(), stage, stage + 12);
            if (m.layers[13].cols == int(x.size()) + 2 * 13 + 2) {  // sequential count decoder
                const size_t base = x.size();
                int values[13]{};
                auto step = [&](int j, int upper) {
                    std::vector<float> in_step(x);
                    in_step.resize(base + 28, 0.0f);
                    in_step[base + j] = 1;
                    for (int q = 0; q < j; ++q) in_step[base + 13 + q] = values[q] / 100.0f;
                    in_step[base + 26] = size / 100.0f;
                    in_step[base + 27] = upper / 100.0f;
                    const auto logits = mlp(m, 13, in_step.data(), false);
                    return best_count(logits.data(), upper);
                };
                if (!CROPS[grp.crop].ongoing) {
                    int last = -1;
                    for (int o = 0; o < OPTIONS; ++o)
                        if (!option_fixed(grp, s.day, o)) last = o;
                    int remaining = size;
                    for (int o = 0; o < OPTIONS && last >= 0; ++o) {
                        if (option_fixed(grp, s.day, o)) continue;
                        values[o] = o == last ? remaining : step(o, remaining);
                        in.options[i][o] = int16_t(values[o]);
                        remaining -= values[o];
                    }
                    continue;
                }
                if (grp.fresh) continue;
                if (!terminal) {
                    values[9] = can_survive_tonight(grp, s.day) ? step(9, size) : 0;
                    values[10] = can_die_tonight(grp) ? step(10, size - values[9]) : size - values[9];
                    if (!ongoing_fertilize_fixed(grp, s.day)) values[11] = step(11, values[9]);
                }
                if (grp.yield > 0) values[12] = step(12, size);
                in.retain[i] = int16_t(values[9]);
                in.clear[i] = int16_t(values[10]);
                in.fertilize[i] = int16_t(values[11]);
                in.harvest[i] = int16_t(values[12]);
                continue;
            }
            const auto out = mlp(m, 13, x.data(), false);
            if (out.size() == size_t(13 * CLASSES)) {  // count heads (scripts/train.py order)
                const float* f = out.data();
                if (!CROPS[grp.crop].ongoing) {
                    // Exact MAP over the nine option heads with the partition constraint.
                    const float* heads[OPTIONS];
                    bool fixed[OPTIONS];
                    for (int o = 0; o < OPTIONS; ++o) heads[o] = f + o * CLASSES, fixed[o] = option_fixed(grp, s.day, o);
                    int n[OPTIONS];
                    if (knobs.opt_rate) {  // .decode optrate (step 68): each option's expected count, apportioned to the group size
                        double e[OPTIONS], sum = 0;
                        for (int o = 0; o < OPTIONS; ++o) {
                            e[o] = 0;
                            if (fixed[o]) continue;
                            double top = -1e300, z = 0;
                            for (int c = 0; c <= size; ++c) top = std::max(top, double(heads[o][c]));
                            for (int c = 0; c <= size; ++c) z += std::exp(heads[o][c] - top);
                            for (int c = 0; c <= size; ++c) e[o] += c * std::exp(heads[o][c] - top) / z;
                            sum += e[o];
                        }
                        if (sum > 0) {
                            for (int o = 0; o < OPTIONS; ++o) e[o] /= sum;
                            apportion(e, fixed, OPTIONS, size, n);
                            OneShot r{i, {}, {}};
                            for (int o = 0; o < OPTIONS; ++o) r.e[o] = e[o] * size, r.fixed[o] = fixed[o];
                            oneshots.push_back(r);
                        } else {
                            map_partition(heads, fixed, OPTIONS, size, n);
                        }
                    } else {
                        map_partition(heads, fixed, OPTIONS, size, n);
                    }
                    for (int o = 0; o < OPTIONS; ++o) in.options[i][o] = int16_t(n[o]);
                    continue;
                }
                if (grp.fresh) continue;
                if (grp.yield > 0) in.harvest[i] = int16_t(best_count(f + 12 * CLASSES, size)), keep(0, i, f + 12 * CLASSES);
                if (terminal) continue;
                // Retain / clear / abandon: MAP over retain and clear; abandon takes the rest
                // (uniform head: abandon has no count head of its own).
                int retain = 0, clear = 0;
                {
                    const bool can_retain = can_survive_tonight(grp, s.day), can_abandon = can_die_tonight(grp);
                    double best = -1e300;
                    for (int r = 0; r <= (can_retain ? size : 0); ++r)
                        for (int c = 0; c <= size - r; ++c) {
                            if (!can_abandon && r + c != size) continue;
                            const double v = (can_retain ? f[9 * CLASSES + r] : 0) + f[10 * CLASSES + c];
                            if (v > best) best = v, retain = r, clear = c;
                        }
                }
                in.retain[i] = int16_t(retain);
                in.clear[i] = int16_t(clear);
                if (!ongoing_fertilize_fixed(grp, s.day)) in.fertilize[i] = int16_t(best_count(f + 11 * CLASSES, retain)), keep(1, i, f + 11 * CLASSES);
                continue;
            }
            const bool modes = out.size() >= 34 && !std::getenv("BC_NO_MODES");
            if (!CROPS[grp.crop].ongoing) {
                bool fixed[OPTIONS + 1];
                for (int o = 0; o < OPTIONS; ++o) fixed[o] = option_fixed(grp, s.day, o);
                fixed[OPTIONS] = false;
                int pure = OPTIONS;  // whole group to one option, or mixed
                if (modes) {
                    pure = -1;
                    for (int o = 0; o <= OPTIONS; ++o)
                        if (!fixed[o] && (pure < 0 || out[14 + o] > out[14 + pure])) pure = o;
                }
                if (pure < OPTIONS) {
                    in.options[i][pure] = int16_t(size);
                    continue;
                }
                double p[OPTIONS];
                softmax_masked(out.data(), fixed, OPTIONS, p);
                int n[OPTIONS];
                apportion(p, fixed, OPTIONS, size, n);
                for (int o = 0; o < OPTIONS; ++o) in.options[i][o] = int16_t(n[o]);
                continue;
            }
            if (grp.fresh) continue;
            if (grp.yield > 0)
                in.harvest[i] = int16_t(modes ? mode_count(&out[31], out[13], size) : std::lround(size * sigmoid(out[13])));
            if (terminal) continue;
            const bool fixed[4] = {!can_survive_tonight(grp, s.day), false, !can_die_tonight(grp), false};
            int mode = 3;
            if (modes) {
                mode = -1;
                for (int k = 0; k < 4; ++k)
                    if (!fixed[k] && (mode < 0 || out[24 + k] > out[24 + mode])) mode = k;
            }
            int n[3] = {0, 0, 0};
            if (mode < 3) {
                n[mode] = size;
            } else {
                double p[3];
                softmax_masked(out.data() + 9, fixed, 3, p);
                apportion(p, fixed, 3, size, n);
            }
            in.retain[i] = int16_t(n[0]);
            in.clear[i] = int16_t(n[1]);
            if (!ongoing_fertilize_fixed(grp, s.day))
                in.fertilize[i] = int16_t(modes ? mode_count(&out[28], out[12], n[0]) : std::lround(n[0] * sigmoid(out[12])));
        }
        for (int i = 0; i < s.n_animals; ++i) {
            const AnimalGroup& grp = s.animals[i];
            const int size = grp.size;
            if (size == 0) continue;
            if (grp.fresh) animal_enc[i] = mlp(m, 6, animal_in(i).data(), true);
            std::vector<float> x(animal_enc[i]);
            x.insert(x.end(), ctx.begin(), ctx.end());
            x.insert(x.end(), stage, stage + 12);
            if (m.layers[16].cols == int(x.size()) + 2 * 3 + 2) {  // sequential: feed, care, collect
                const size_t base = x.size();
                int values[3]{};
                auto step = [&](int j, int upper) {
                    std::vector<float> in_step(x);
                    in_step.resize(base + 8, 0.0f);
                    in_step[base + j] = 1;
                    for (int q = 0; q < j; ++q) in_step[base + 3 + q] = values[q] / 100.0f;
                    in_step[base + 6] = size / 100.0f;
                    in_step[base + 7] = upper / 100.0f;
                    const auto logits = mlp(m, 16, in_step.data(), false);
                    return best_count(logits.data(), upper);
                };
                if (!terminal) values[0] = step(0, size);
                if (!terminal && !care_fixed(grp, s.day) && values[0] > 0) values[1] = step(1, values[0]);
                if (grp.held > 0) values[2] = step(2, size);
                in.feed[i] = int16_t(values[0]);
                in.care[i] = int16_t(values[1]);
                in.collect[i] = int16_t(values[2]);
                continue;
            }
            const auto out = mlp(m, 16, x.data(), false);
            if (out.size() == size_t(3 * CLASSES)) {  // count heads: feed, care, collect
                if (!terminal) in.feed[i] = int16_t(best_count(&out[0], size)), keep(2, i, &out[0]);
                if (!terminal && !care_fixed(grp, s.day) && in.feed[i] > 0) in.care[i] = int16_t(best_count(&out[CLASSES], in.feed[i]));
                if (!terminal && !care_fixed(grp, s.day)) keep(3, i, &out[CLASSES]);
                if (grp.held > 0) in.collect[i] = int16_t(best_count(&out[2 * CLASSES], size)), keep(4, i, &out[2 * CLASSES]);
                continue;
            }
            const bool modes = out.size() >= 12 && !std::getenv("BC_NO_MODES");
            auto count = [&](int field, int n) {
                return int16_t(modes ? mode_count(&out[3 + 3 * field], out[field], n) : std::lround(n * sigmoid(out[field])));
            };
            if (!terminal) in.feed[i] = count(0, size);
            if (!terminal && !care_fixed(grp, s.day)) in.care[i] = count(1, in.feed[i]);
            if (grp.held > 0) in.collect[i] = count(2, size);
        }
        if (knobs.opt_rate) {  // .decode optrate (step 68)
            // For one option kind: the groups' expected total T (each head's distribution over 0..upper), then the counts with sum T
            // that maximize the summed log-probability (exact DP over groups).
            auto allocate = [&](int kind, auto upper_of, auto set) {
                std::vector<const Head*> hs;
                for (const auto& h : heads_seen)
                    if (h.kind == kind) hs.push_back(&h);
                if (hs.empty()) return;
                std::vector<std::vector<double>> lp(hs.size());
                std::vector<int> ub(hs.size());
                double expected = 0;
                int room = 0;
                for (size_t g = 0; g < hs.size(); ++g) {
                    ub[g] = std::min(upper_of(hs[g]->group), CLASSES - 1);
                    double top = -1e300, z = 0;
                    for (int c = 0; c <= ub[g]; ++c) top = std::max(top, double(hs[g]->logits[c]));
                    for (int c = 0; c <= ub[g]; ++c) z += std::exp(hs[g]->logits[c] - top);
                    lp[g].resize(ub[g] + 1);
                    for (int c = 0; c <= ub[g]; ++c) lp[g][c] = hs[g]->logits[c] - top - std::log(z), expected += c * std::exp(lp[g][c]);
                    room += ub[g];
                }
                const int total = std::min(room, int(std::lround(expected)));
                const size_t G = hs.size();
                std::vector<double> best((G + 1) * (total + 1), -1e300);
                std::vector<int> pick(G * (total + 1), 0);
                best[G * (total + 1)] = 0;
                for (size_t g = G; g-- > 0;)
                    for (int r = 0; r <= total; ++r)
                        for (int c = 0; c <= std::min(r, ub[g]); ++c) {
                            const double v = lp[g][c] + best[(g + 1) * (total + 1) + r - c];
                            if (v > best[g * (total + 1) + r]) best[g * (total + 1) + r] = v, pick[g * (total + 1) + r] = c;
                        }
                if (best[0 * (total + 1) + total] <= -1e299) return;
                for (size_t g = 0, r = size_t(total); g < G; ++g) {
                    const int c = pick[g * (total + 1) + r];
                    set(hs[g]->group, c);
                    r -= size_t(c);
                }
            };
            allocate(0, [&](int i) { return s.crops[i].size; }, [&](int i, int n) { in.harvest[i] = int16_t(n); });
            allocate(1, [&](int i) { return int(in.retain[i]); }, [&](int i, int n) { in.fertilize[i] = int16_t(n); });
            allocate(2, [&](int i) { return s.animals[i].size; }, [&](int i, int n) { in.feed[i] = int16_t(n); });
            for (int i = 0; i < s.n_animals; ++i) in.care[i] = int16_t(std::min(in.care[i], in.feed[i]));
            allocate(3, [&](int i) { return int(in.feed[i]); }, [&](int i, int n) { in.care[i] = int16_t(n); });
            allocate(4, [&](int i) { return s.animals[i].size; }, [&](int i, int n) { in.collect[i] = int16_t(n); });
            // One-shot crops, per crop: the expected number of watered plants over the crop's groups, given out as whole units by
            // largest remainder across groups; inside a group the watered units go to the water options and the rest to the others,
            // each part apportioned by the options' expected counts (a size-1 group waters when its water share wins the remainder).
            for (int crop = 0; crop < N_CROPS; ++crop) {
                std::vector<const OneShot*> gs;
                for (const auto& r : oneshots)
                    if (s.crops[r.group].crop == crop) gs.push_back(&r);
                if (gs.empty()) continue;
                std::vector<double> w(gs.size());
                std::vector<int> give(gs.size()), cap(gs.size());
                double total = 0;
                int given = 0;
                for (size_t g = 0; g < gs.size(); ++g) {
                    bool any = false;
                    for (int o = 0; o < OPTIONS; ++o)
                        if (has_water(o) && !gs[g]->fixed[o]) w[g] += gs[g]->e[o], any = true;
                    cap[g] = any ? s.crops[gs[g]->group].size : 0;
                    give[g] = std::min(cap[g], int(w[g])), given += give[g], total += w[g];
                }
                for (int target = int(std::lround(total)); given < target; ++given) {
                    int best = -1;
                    for (size_t g = 0; g < gs.size(); ++g)
                        if (give[g] < cap[g] && (best < 0 || w[g] - give[g] > w[best] - give[best])) best = int(g);
                    if (best < 0) break;
                    ++give[best];
                }
                for (size_t g = 0; g < gs.size(); ++g) {
                    const int i = gs[g]->group, size = s.crops[i].size;
                    double pw[OPTIONS], pn[OPTIONS];
                    bool fw[OPTIONS], fn[OPTIONS];
                    for (int o = 0; o < OPTIONS; ++o) {
                        fw[o] = gs[g]->fixed[o] || !has_water(o), fn[o] = gs[g]->fixed[o] || has_water(o);
                        pw[o] = fw[o] ? 0 : gs[g]->e[o], pn[o] = fn[o] ? 0 : gs[g]->e[o];
                    }
                    int nw[OPTIONS], nn[OPTIONS];
                    apportion(pw, fw, OPTIONS, give[g], nw);
                    apportion(pn, fn, OPTIONS, size - give[g], nn);
                    bool free_rest = false;
                    for (int o = 0; o < OPTIONS; ++o) free_rest |= !fn[o];
                    if (!free_rest && size - give[g] > 0) continue;  // no non-water option left: keep the group's apportioned split
                    for (int o = 0; o < OPTIONS; ++o) in.options[i][o] = int16_t(nw[o] + nn[o]);
                }
            }
        }
    return in;
    };
    DayIntent in = decode(1 << 20);
    // Site capacity (designs/day_intent.md): with too few free tiles, decode again with
    // fewer new crops and animals; group fields follow the new counts. The cap falls
    // every round, so this ends (at worst with no new entities).
    for (int cap = free_sites(s, in); new_entities(in) > cap; cap = free_sites(s, in)) in = decode(cap);
    // Service pushes (flexibility experiments): every animal fed / cared for / collected,
    // every held ongoing product harvested.
    if (dawn.day < LAST_DAY)
        for (int i = 0; i < s.n_animals; ++i) {
            const AnimalGroup& g = s.animals[i];
            if (knobs.feed_all) in.feed[i] = int16_t(g.size);
            if (knobs.care_all && !care_fixed(g, s.day)) in.care[i] = in.feed[i];
        }
    for (int i = 0; i < s.n_animals; ++i)
        if (knobs.collect_all && s.animals[i].held > 0) in.collect[i] = int16_t(s.animals[i].size);
    for (int i = 0; i < s.n_crops; ++i)
        if (knobs.harvest_all && !s.crops[i].fresh && CROPS[s.crops[i].crop].ongoing && s.crops[i].yield > 0)
            in.harvest[i] = int16_t(s.crops[i].size);
    const std::string why = validate(s, in);
    if (invalid) *invalid = why;
    return in;
}

std::string summarize(const DayIntent& in, const Schema& s) {
    std::string o = "new";
    for (int c = 0; c < N_CROPS; ++c) o += ' ' + std::to_string(in.new_crop[c]);
    o += " | animals";
    for (int a = 0; a < N_ANIMALS; ++a) o += ' ' + std::to_string(in.new_animal[a]) + '+' + std::to_string(in.reserve[a]);
    int harvest = 0, feed = 0, care = 0, collect = 0, water = 0, retain = 0, clear = 0, fert = 0;
    for (int i = 0; i < s.n_crops; ++i) {
        for (int op = 0; op < OPTIONS; ++op) {
            harvest += has_harvest(op) * in.options[i][op];
            water += has_water(op) * in.options[i][op];
            fert += has_fertilize(op) * in.options[i][op];
            clear += (op == CLEAR) * in.options[i][op];
        }
        harvest += in.harvest[i], retain += in.retain[i], clear += in.clear[i], fert += in.fertilize[i];
    }
    for (int i = 0; i < s.n_animals; ++i) feed += in.feed[i], care += in.care[i], collect += in.collect[i];
    return o + " | land " + std::to_string(in.buy_land) + " harvest " + std::to_string(harvest) + " water " +
           std::to_string(water) + " fert " + std::to_string(fert) + " retain " + std::to_string(retain) + " clear " +
           std::to_string(clear) + " feed " + std::to_string(feed) + " care " + std::to_string(care) + " collect " +
           std::to_string(collect);
}

namespace {
// Models are loaded once per path and shared (self-play: several versions per process).
std::shared_ptr<const Model> shared_model(const std::string& path) {
    static std::mutex lock;
    static std::map<std::string, std::shared_ptr<const Model>> models;
    const std::lock_guard<std::mutex> guard(lock);
    auto& model = models[path];
    if (!model) {
        auto m = std::make_shared<Model>();
        if (!m->load(path)) std::abort();
        model = m;
    }
    return model;
}
}

void Agent::reset(const agent::AgentInit&) {
    const char* env = std::getenv("BC_OPUS_MODEL");
    const std::string path = !model_path.empty() ? model_path : env && *env ? env : BC_OPUS_DEFAULT_MODEL;
    model_ = shared_model(path);
    // Compiler configuration: defaults, then the model's sidecar (<model>.compiler, "key value"
    // lines), then DC10_* experiment variables. The sidecar keeps each agent's settings its own
    // when several versions of this agent share a process.
    const dc10::CompileOptions defaults;
    options_.trim_model = defaults.trim_model;
    options_.race = defaults.race;
    options_.race_deadline = defaults.race_deadline;
    options_.sale_tie_now = defaults.sale_tie_now;
    options_.recovery = defaults.recovery;
    options_.reserve_from_day = defaults.reserve_from_day;
    options_.sell_order = defaults.sell_order;
    options_.fert_value = defaults.fert_value;
    options_.return_wages = defaults.return_wages;
    options_.full_care = defaults.full_care;
    options_.market_deadline = defaults.market_deadline;
    options_.level_hires = defaults.level_hires;
    options_.hire_cap_stock = defaults.hire_cap_stock;
    options_.collect_all = defaults.collect_all;
    options_.forecast_blend = defaults.forecast_blend;
    options_.route_search = defaults.route_search;
    options_.slack_care = defaults.slack_care;
    options_.race_dp = defaults.race_dp;
    options_.blend_visible = defaults.blend_visible;
    options_.forecast_stock = defaults.forecast_stock;
    options_.forecast_adapt = defaults.forecast_adapt;
    options_.forecast_fit = defaults.forecast_fit;
    options_.forecast_select = defaults.forecast_select;
    options_.forecast_intraday = defaults.forecast_intraday;
    options_.forecast_timing = defaults.forecast_timing;
    options_.forecast_prior = defaults.forecast_prior;
    options_.race_early = defaults.race_early;
    options_.race_steps = defaults.race_steps;
    options_.melon_tie_now = defaults.melon_tie_now;
    options_.first_full = defaults.first_full;
    options_.race_stream_first = defaults.race_stream_first;
    options_.ladder_effort = defaults.ladder_effort;
    options_.race_stream_rate = defaults.race_stream_rate;
    auto set = [&](const std::string& key, int value) {
        if (key == "trim_model") options_.trim_model = value;
        else if (key == "race") options_.race = value;
        else if (key == "race_deadline") options_.race_deadline = value;
        else if (key == "sale_tie_now") options_.sale_tie_now = value;
        else if (key == "recovery") options_.recovery = value;
        else if (key == "reserve_from_day") options_.reserve_from_day = value;
        else if (key == "sell_order") options_.sell_order = value;
        else if (key == "fert_value") options_.fert_value = value;
        else if (key == "return_wages") options_.return_wages = value;
        else if (key == "full_care") options_.full_care = value;
        else if (key == "market_deadline") options_.market_deadline = value;
        else if (key == "level_hires") options_.level_hires = value;
        else if (key == "hire_cap_stock") options_.hire_cap_stock = value;
        else if (key == "collect_all") options_.collect_all = value;
        else if (key == "forecast_blend") options_.forecast_blend = value;
        else if (key == "route_search") options_.route_search = value;
        else if (key == "slack_care") options_.slack_care = value;
        else if (key == "race_dp") options_.race_dp = value;
        else if (key == "blend_visible") options_.blend_visible = value;
        else if (key == "forecast_stock") options_.forecast_stock = value;
        else if (key == "forecast_adapt") options_.forecast_adapt = value;
        else if (key == "forecast_fit") options_.forecast_fit = value;
        else if (key == "forecast_select") options_.forecast_select = value;
        else if (key == "forecast_intraday") options_.forecast_intraday = value;
        else if (key == "forecast_timing") options_.forecast_timing = value;
        else if (key == "forecast_prior") options_.forecast_prior = value;
        else if (key == "race_early") options_.race_early = value;
        else if (key == "race_steps") options_.race_steps = value;
        else if (key == "melon_tie_now") options_.melon_tie_now = value;
        else if (key == "first_full") options_.first_full = value;
        else if (key == "race_stream_first") options_.race_stream_first = value;
        else if (key == "ladder_effort") options_.ladder_effort = value;
        else if (key == "race_stream_rate") options_.race_stream_rate = value;
        else if (key == "capacity_defer") options_.capacity_defer = value;
        else if (key == "hold_discount") options_.hold_discount = value / 100.0;  // percent
        else if (key == "max_hires") options_.solve.max_hires = value;
        else if (key == "race_first") options_.race_first = value;
        else if (key == "feed_reserve") options_.feed_reserve = value;
        else if (key == "capacity_trim") options_.capacity_trim = value;
        else if (key == "solve_variants") options_.solve.variants = value;  // route solver search parts
        else if (key == "solve_minimize_variants") options_.solve.minimize_variants = value;
        else if (key == "solve_opportunistic") options_.solve.opportunistic_hire_reduction = value;
        else if (key == "solve_route_rounds") options_.solve.route_rounds = value;
        else std::abort();  // unknown setting in the sidecar
    };
    if (std::FILE* file = std::fopen((path + ".compiler").c_str(), "r")) {
        char key[64];
        int value = 0;
        while (std::fscanf(file, "%63s %d", key, &value) == 2) set(key, value);
        std::fclose(file);
    }
    if (std::getenv("DC10_TRIM_MODEL")) set("trim_model", 1);
    if (std::getenv("DC10_RACE")) set("race", 1);
    if (const char* v = std::getenv("DC10_RACE_DEADLINE")) set("race_deadline", std::atoi(v));
    if (std::getenv("DC10_SALE_TIE_NOW")) set("sale_tie_now", 1);
    if (std::getenv("DC10_NO_RECOVERY")) set("recovery", 0);
    if (const char* v = std::getenv("DC10_RESERVE_FROM_DAY")) set("reserve_from_day", std::atoi(v));
    if (const char* v = std::getenv("DC10_SELL_ORDER")) set("sell_order", std::atoi(v));
    if (std::getenv("DC10_FERT_VALUE")) set("fert_value", 1);
    if (std::getenv("DC10_RETURN_WAGES")) set("return_wages", 1);
    if (std::getenv("DC10_FULL_CARE")) set("full_care", 1);
    if (const char* v = std::getenv("DC10_MARKET_DEADLINE")) set("market_deadline", std::atoi(v));
    if (const char* v = std::getenv("DC10_LEVEL_HIRES")) set("level_hires", std::atoi(v));
    if (std::getenv("DC10_HIRE_CAP_STOCK")) set("hire_cap_stock", 1);
    if (std::getenv("DC10_COLLECT_ALL")) set("collect_all", 1);
    if (std::getenv("DC10_FORECAST_BLEND")) set("forecast_blend", 1);
    if (const char* v = std::getenv("DC10_ROUTE_SEARCH")) set("route_search", std::atoi(v));
    if (std::getenv("DC10_SLACK_CARE")) set("slack_care", 1);
    if (std::getenv("DC10_RACE_DP")) set("race_dp", 1);
    if (const char* v = std::getenv("DC10_BLEND_VISIBLE")) set("blend_visible", std::atoi(v));
    if (std::getenv("DC10_FORECAST_STOCK")) set("forecast_stock", 1);
    if (std::getenv("DC10_FORECAST_ADAPT")) set("forecast_adapt", 1);
    if (std::getenv("DC10_FORECAST_FIT")) set("forecast_fit", 1);
    if (std::getenv("DC10_FORECAST_SELECT")) set("forecast_select", 1);
    if (std::getenv("DC10_CAPACITY_DEFER")) set("capacity_defer", 1);
    if (std::getenv("DC10_FORECAST_INTRADAY")) set("forecast_intraday", 1);
    if (std::getenv("DC10_FORECAST_TIMING")) set("forecast_timing", 1);
    if (std::getenv("DC10_FORECAST_PRIOR")) set("forecast_prior", 1);
    if (std::getenv("DC10_RACE_EARLY")) set("race_early", 1);
    if (const char* v = std::getenv("DC10_RACE_STEPS")) set("race_steps", std::atoi(v));
    if (std::getenv("DC10_MELON_TIE_NOW")) set("melon_tie_now", 1);
    if (std::FILE* file = std::fopen((path + ".decode").c_str(), "r")) {
        char key[64];
        for (double a = 0, b = 0; std::fscanf(file, "%63s", key) == 1;) {
            const std::string k = key;
            if (k == "days" && std::fscanf(file, "%lf %lf", &a, &b) == 2) decode_first = int(a), decode_last = int(b);
            else if (k == "q_crop" && std::fscanf(file, "%lf", &a) == 1) decode_q_crop = a;
            else if (k == "q_animal" && std::fscanf(file, "%lf", &a) == 1) decode_q_animal = a;
            else if (k == "reach_days" && std::fscanf(file, "%lf %lf", &a, &b) == 2) reach_first = int(a), reach_last = int(b);
            else if (k == "reach") {
                reach_q.clear();
                for (double q; std::fscanf(file, "%lf", &q) == 1;) reach_q.push_back(q);
            } else if (k == "creach_days" && std::fscanf(file, "%lf %lf", &a, &b) == 2) creach_first = int(a), creach_last = int(b);
            else if (k == "creach") {
                creach_q.clear();
                for (double q; std::fscanf(file, "%lf", &q) == 1;) creach_q.push_back(q);
            } else if (k == "reach_stress" && std::fscanf(file, "%lf", &a) == 1) reach_stress = a != 0;
            else if (k == "max_land" && std::fscanf(file, "%lf", &a) == 1) max_land = int(a);
            else if (k == "land_push" && std::fscanf(file, "%lf %lf %lf", &a, &b, &land_push_bias) == 3)
                land_push_first = int(a), land_push_last = int(b);
            else if (k == "stress_down") {
                stress_down_q.clear();
                for (double q; std::fscanf(file, "%lf", &q) == 1;) stress_down_q.push_back(q);
            }
            else std::abort();
        }
        std::fclose(file);
    }
    if (opening_days < 0) {
        opening_days = int(knob("BC_OPENING_DAYS", model_->opening_days));
        // <model>.opening_model: the model used on opening days (a path relative to this model's
        // folder), e.g. a fine-tune on one team's games; BC_OPENING_MODEL overrides it.
        if (std::FILE* file = std::fopen((path + ".opening_model").c_str(), "r")) {
            char name[512];
            if (std::fscanf(file, "%511s", name) != 1) std::abort();
            std::fclose(file);
            const size_t slash = path.find_last_of('/');
            opening_model = name[0] == '/' || slash == std::string::npos ? name : path.substr(0, slash + 1) + name;
        }
        const char* env_path = std::getenv("BC_OPENING_MODEL");
        if (env_path && *env_path) opening_model = env_path;
        if (std::FILE* file = std::fopen((path + ".land_model").c_str(), "r")) {
            char name[512];
            if (std::fscanf(file, "%511s %d", name, &land_style) != 2) std::abort();
            std::fclose(file);
            const size_t slash = path.find_last_of('/');
            land_model = name[0] == '/' || slash == std::string::npos ? name : path.substr(0, slash + 1) + name;
        }
        // <model>.ensemble: further models (paths relative to this model's folder, one per line)
        // whose whole-farm heads are averaged with this model's; BC_ENSEMBLE overrides it.
        if (std::FILE* file = std::fopen((path + ".ensemble").c_str(), "r")) {
            const size_t slash = path.find_last_of('/');
            for (char name[512]; std::fscanf(file, "%511s", name) == 1;)
                ensemble.push_back(name[0] == '/' || slash == std::string::npos ? name : path.substr(0, slash + 1) + name);
            std::fclose(file);
        }
        opening_style = int(knob("BC_OPENING_STYLE", model_->opening_style));
        if (const char* list = std::getenv("BC_ENSEMBLE"); list && *list)
            for (std::string rest = (ensemble.clear(), list); !rest.empty();) {
                const size_t comma = rest.find(',');
                ensemble.push_back(rest.substr(0, comma));
                rest = comma == std::string::npos ? "" : rest.substr(comma + 1);
            }
    }
    opening_model_ = opening_model.empty() ? nullptr : shared_model(opening_model);
    land_model_ = land_model.empty() ? nullptr : shared_model(land_model);
    ensemble_.clear();
    for (const auto& path : ensemble) ensemble_.push_back(shared_model(path));
    history_ = History{};
    executor_ = DayExecutor{};
    if (!solver_) solver_ = std::make_unique<dp::Solver>();
    planned_day_ = -1;
    reports_.clear();
}

DecodeKnobs DecodeKnobs::from_env() {
    DecodeKnobs k;
    k.q_crop = knob("BC_Q_CROP", 0.5);
    k.q_animal = knob("BC_Q_ANIMAL", 0.5);
    k.land_bias = knob("BC_LAND_BIAS", 0);
    k.feed_all = knob("BC_FEED_ALL", 0), k.care_all = knob("BC_CARE_ALL", 0);
    k.collect_all = knob("BC_COLLECT_ALL", 0), k.harvest_all = knob("BC_HARVEST_ALL", 0);
    k.sample = knob("BC_SAMPLE", 0);
    if (const char* days = std::getenv("BC_PUSH_DAYS"); days && std::sscanf(days, "%d-%d", &k.push_first, &k.push_last) != 2)
        std::abort();
    return k;
}

Agent::Agent(const Agent& o)
    : knobs(o.knobs), model_path(o.model_path), opening_days(o.opening_days), opening_model(o.opening_model),
      land_model(o.land_model), land_style(o.land_style),
      opening_style(o.opening_style), ensemble(o.ensemble), decode_q_crop(o.decode_q_crop),
      decode_q_animal(o.decode_q_animal), decode_first(o.decode_first), decode_last(o.decode_last),
      reach_q(o.reach_q), reach_first(o.reach_first), reach_last(o.reach_last), creach_q(o.creach_q),
      creach_first(o.creach_first), creach_last(o.creach_last), reach_stress(o.reach_stress), max_land(o.max_land),
      stress_down_q(o.stress_down_q), land_push_first(o.land_push_first), land_push_last(o.land_push_last),
      land_push_bias(o.land_push_bias),
      model_(o.model_), opening_model_(o.opening_model_), land_model_(o.land_model_),
      ensemble_(o.ensemble_), history_(o.history_), options_(o.options_), executor_(o.executor_),
      solver_(std::make_unique<dc10::dp::Solver>()), planned_day_(o.planned_day_), reports_(o.reports_),
      last_intent_(o.last_intent_), last_schema_(o.last_schema_) {
    executor_.rebind(history_);
}

void Agent::act(const agent::AgentObservation& obs, const agent::DecisionBudget&, Action& action) {
    if (obs.hour == 0 && obs.day != planned_day_) {
        planned_day_ = obs.day;
        DayReport report;
        report.day = obs.day;
        const bool opening = obs.day < opening_days;
        DecodeKnobs day_knobs = knobs;
        if (obs.day < knobs.push_first || obs.day > knobs.push_last) {
            day_knobs.q_crop = day_knobs.q_animal = 0.5;
            day_knobs.land_bias = 0;
        }
        if (opening && opening_style >= 0) day_knobs.style = opening_style;
        if (obs.day >= land_push_first && obs.day <= land_push_last) day_knobs.land_bias = land_push_bias;
        day_knobs.max_quadrants = max_land;
        const bool land_phase = land_model_ && obs.day >= land_push_first &&
                                (obs.day <= land_push_last || obs.self().n_quadrants >= 4);
        if (land_phase) day_knobs.style = land_style;
        if (obs.day >= decode_first && obs.day <= decode_last) {
            if (knobs.q_crop == 0.5) day_knobs.q_crop = decode_q_crop;
            if (knobs.q_animal == 0.5) day_knobs.q_animal = decode_q_animal;
        }
        const Model& day_model = opening && opening_model_ ? *opening_model_ : land_phase ? *land_model_ : *model_;
        std::vector<float> averaged;
        if (!ensemble_.empty()) {
            decode_intent(day_model, obs, history_, nullptr, &averaged, day_knobs);
            for (const auto& member : ensemble_) {
                std::vector<float> logits;
                decode_intent(*member, obs, history_, nullptr, &logits, DecodeKnobs{});
                if (logits.size() != averaged.size()) std::abort();
                for (size_t i = 0; i < logits.size(); ++i) averaged[i] += logits[i];
            }
            for (auto& v : averaged) v /= float(1 + ensemble_.size());
            day_knobs.head_override = &averaged;
        }
        DayIntent intent = decode_intent(day_model, obs, history_, &report.invalid, nullptr, day_knobs);
        const double time_left = knob("BC_TIME_LEFT", time_left_);  // BC_TIME_LEFT: test override
        options_.time_pressure = time_left < 10 ? 2 : time_left < 25 ? 1 : 0;
        options_.time_left = time_left;
        // DC10_NO_DEADLINE (set by the C++ game tools): no wall-clock limit, so screens stay deterministic.
        // DC10_TIME_SCALE=f (tools/full_games FULL_BUDGET): the target machine is f times slower than this one.
        options_.deadline_ms = std::getenv("DC10_NO_DEADLINE")
                                   ? 0
                                   : now_ms() + 1000 * (0.9 + std::max(0.0, time_left) / std::max(1, LAST_DAY + 1 - obs.day)) /
                                                    knob("DC10_TIME_SCALE", 1.0);
        DayPlan plan = compile_day(obs, history_, intent, options_, *solver_);
        double herd_q = day_knobs.q_animal;  // the animal quantile of the plan kept
        if (obs.day >= reach_first && obs.day <= reach_last && !options_.time_pressure && plan.status == CompileStatus::Ok &&
            plan.fallback == KeepAll && !plan.stress_funded) {
            auto animals = [](const DayIntent& in) { return in.new_animal[0] + in.new_animal[1] + in.new_animal[2]; };
            double spent = plan.compile_ms;
            for (const double q : stress_down_q) {
                DecodeKnobs fewer = day_knobs;
                fewer.q_animal = q;
                const DayIntent smaller = decode_intent(day_model, obs, history_, nullptr, nullptr, fewer);
                if (animals(smaller) >= animals(intent)) continue;
                DayPlan tried = compile_day(obs, history_, smaller, options_, *solver_);
                spent += tried.compile_ms;
                if (tried.status == CompileStatus::Ok && tried.fallback == KeepAll && tried.stress_funded) {
                    tried.reason = "stress down q" + std::to_string(q).substr(0, 4) + ";" + tried.reason;
                    intent = smaller, plan = tried, herd_q = q;
                    break;
                }
            }
            plan.compile_ms = spent;
        }
        if (obs.day >= reach_first && obs.day <= reach_last && !options_.time_pressure && plan.status == CompileStatus::Ok &&
            plan.fallback == KeepAll) {
            auto animals = [](const DayIntent& in) { return in.new_animal[0] + in.new_animal[1] + in.new_animal[2]; };
            double spent = plan.compile_ms;
            for (const double q : reach_q) {
                if (options_.deadline_ms && now_ms() > options_.deadline_ms) break;
                DecodeKnobs more = day_knobs;
                more.q_animal = q;
                const DayIntent bigger = decode_intent(day_model, obs, history_, nullptr, nullptr, more);
                if (animals(bigger) <= animals(intent)) continue;
                DayPlan tried = compile_day(obs, history_, bigger, options_, *solver_);
                spent += tried.compile_ms;
                if (tried.status == CompileStatus::Ok && tried.fallback == KeepAll && (!reach_stress || tried.stress_funded)) {
                    tried.reason = "herd reach q" + std::to_string(q).substr(0, 4) + ";" + tried.reason;
                    intent = bigger, plan = tried, herd_q = q;
                    break;
                }
            }
            plan.compile_ms = spent;
        }
        if (obs.day >= creach_first && obs.day <= creach_last && !options_.time_pressure && plan.status == CompileStatus::Ok &&
            plan.fallback == KeepAll) {
            auto crops = [](const DayIntent& in) {
                int n = 0;
                for (int c = 0; c < N_CROPS; ++c) n += in.new_crop[c];
                return n;
            };
            double spent = plan.compile_ms;
            for (const double q : creach_q) {
                if (options_.deadline_ms && now_ms() > options_.deadline_ms) break;
                DecodeKnobs more = day_knobs;
                more.q_crop = q;
                more.q_animal = herd_q;  // keep today's herd reach
                const DayIntent bigger = decode_intent(day_model, obs, history_, nullptr, nullptr, more);
                if (crops(bigger) <= crops(intent)) continue;
                DayPlan tried = compile_day(obs, history_, bigger, options_, *solver_);
                spent += tried.compile_ms;
                if (tried.status == CompileStatus::Ok && tried.fallback == KeepAll && (!reach_stress || tried.stress_funded)) {
                    tried.reason = "crop reach q" + std::to_string(q).substr(0, 4) + ";" + tried.reason;
                    intent = bigger, plan = tried;
                    break;
                }
            }
            plan.compile_ms = spent;
        }
        // Budgeted wide routing: only within today's budget and while more than 30 s of overage remain
        // (the rest is kept for the required compiles of later days).
        if (options_.route_search == 4 && !options_.time_pressure && plan.status == CompileStatus::Ok &&
            (!options_.deadline_ms || (options_.time_left > 30 && now_ms() + 3.5 * plan.compile_ms < options_.deadline_ms))) {
            CompileOptions wide = options_;
            wide.route_search = 1;
            wide.time_left = std::max(wide.time_left, 41.0);  // the budget was checked above (compiler gate: > 40 s)
            DayPlan better = compile_day(obs, history_, intent, wide, *solver_);
            const double spent = plan.compile_ms + better.compile_ms;
            if (better.status == CompileStatus::Ok && better.fallback <= plan.fallback) {
                better.reason = "wide routes;" + better.reason;
                plan = better;
            }
            plan.compile_ms = spent;
        }
        last_intent_ = intent;
        last_schema_ = describe(obs);
        size_new_groups(last_schema_, intent);
        print_profile(obs.day, plan.compile_ms);
        report.status = int(plan.status);
        report.fallback = plan.fallback;
        report.return_percent = plan.return_percent;
        report.hires = plan.hires;
        report.compile_ms = plan.compile_ms;
        report.reason = plan.reason;
        if (std::getenv("DC10_DAYLOG")) {  // DC10_DAYLOG: one line per dawn on stderr
            int feeds = 0, wheat = 0;
            for (const uint8_t e : plan.input.events) feeds += bool(e & dp::Feed);
            for (const int n : plan.input.buy_wheat) wheat += n;
            std::fprintf(stderr, "daylog p%d d%d money %.0f status %s fallback %d hires %d animals %d %d %d feeds %d buy_wheat %d shed_wheat %d stress %d funding %d | %s\n",
                         obs.player, obs.day, obs.self().money, status_name(plan.status), plan.fallback, plan.hires,
                         intent.new_animal[0], intent.new_animal[1], intent.new_animal[2], feeds, wheat, int(obs.own.shed[WHEAT]), int(plan.stress_funded), plan.funding,
                         plan.reason.c_str());
        }
        if (plan.status != CompileStatus::Ok) {  // no schedule: markets only
            DayPlan idle;
            idle.hours = obs.day == LAST_DAY ? HOURS - 1 : HOURS;
            idle.status = plan.status;
            plan = idle;
        }
        reports_.push_back(report);
        executor_.start(obs, history_, plan, options_);
    }
    executor_.act(obs, action);
    history_.observe(obs, action);
}
}
