#pragma once
#include "certify.hpp"

namespace placement {
struct RouteEdge { int from, to, available, day; int64_t weight; };
struct GeometryScore {
    int excess = 0, distance = 0;
    int64_t weighted_distance = 0;
};

// Logical field cells move with the layout; resource anchors stay physical.
// The first hired-worker edge uses the nearest access cell as a relaxation.
// Exact route replay still decides whether its actual spawn is compatible.
class RouteGeometry {
    std::vector<RouteEdge> edges;
    static int point(int code, const Layout& layout) { return code < 100 ? layout[code] : code - 100; }
public:
    explicit RouteGeometry(const Course& course) {
        for (int d = 0; d < 30; ++d) {
            const auto& day = course.days[d]; const auto anchors = source_anchors(day, identity());
            const auto weight = std::max<int64_t>(1, labor::hire_cost(day.problem.worker_count) - labor::hire_cost(std::max(1, int(day.problem.worker_count) - 1)));
            for (int u = 0; u < day.problem.worker_count; ++u) {
                int born = 0;
                while (born < 24 && day.physical[born].n_units <= u) ++born;
                int previous = u ? -1 : 144, hour = born - 1;
                for (const auto& anchor : anchors[u]) {
                    const int next = anchor.cell + (anchor.field ? 0 : 100);
                    edges.push_back({previous, next, anchor.hour - hour - 1, d, weight});
                    previous = next; hour = anchor.hour;
                }
            }
        }
        if ((*this)(identity()).excess) throw std::runtime_error("source route violates its own travel windows");
    }
    GeometryScore operator()(const Layout& layout) const {
        GeometryScore score;
        for (const auto& edge : edges) {
            const int to = point(edge.to, layout);
            const int from = edge.from < 0 ? -1 : point(edge.from, layout);
            const int distance = from < 0 ? labor::shed_distance(to) : std::abs(to % 10 - from % 10) + std::abs(to / 10 - from / 10);
            score.excess += std::max(0, distance - edge.available);
            score.distance += distance; score.weighted_distance += distance * edge.weight;
        }
        return score;
    }
    size_t size() const { return edges.size(); }
};
}
