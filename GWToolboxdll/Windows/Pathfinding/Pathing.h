#pragma once

#include <cstdint>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Pathing.h>
#include "MapSpecificData.h"

namespace Pathing {
    inline static auto max_visibility_range = 5000.0f;

    enum class Error : uint32_t {
        OK,
        Unknown,
        FailedToFindGoalBox,
        FailedToFindStartBox,
        FailedToFinializePath,
        InvalidMapContext,
        BuildPathLengthExceeded,
        FailedToGetPathingMapBlock
    };

    // basically a copy of Pathing trapezoid with additional layer and corner points.
    //  a----d
    //   \    \
    //    b____c
    class SimplePT {
    public:
        enum class adjacentSide {
            none, aBottom_bTop, aTop_bBottom, aLeft_bRight, aRight_bLeft
        };

        SimplePT(const GW::PathingTrapezoid& pt, uint32_t layer);
        adjacentSide Touching(const SimplePT& rhs) const;
        adjacentSide TouchingHeight(const SimplePT& rhs, float max_height_diff = 200.0f) const;

        uint32_t id, layer;
        GW::Vec2f a, b, c, d;
        const bool IsOnPathingTrapezoid(const GW::Vec2f& p) const;
    };

    class AABB {
    public:
        // https://noonat.github.io/intersect/
        using box_id = uint32_t;

        AABB(const SimplePT& t);
        bool intersect(const GW::Vec2f& rhs) const;                                           // point
        bool intersect(const GW::Vec2f& rhs, float radius) const;                             // circle
        bool intersect(const GW::Vec2f& a, const GW::Vec2f& b, GW::Vec2f padding = {}) const; // segment
        bool intersect(const AABB& rhs, GW::Vec2f padding = {}) const;                        // aabb

        box_id m_id;
        GW::Vec2f m_pos, m_half;
        const SimplePT* m_t;
    };

    class MilePath {
        volatile bool m_processing = false;
        volatile bool m_done = false;
        volatile bool m_terminateThread = false;
        volatile int m_progress = 0;

        std::thread* worker_thread = nullptr;

    public:
        MilePath();
        ~MilePath() { shutdown(); }

        MilePath* instance();
        // Signals terminate to worker thread. Usually followed late by shutdown() to grab the thread again.
        void stopProcessing() { m_terminateThread = true; }
        bool isProcessing() { return m_processing; }
        // Signals terminate to worker thread, waits for thread to finish. Blocking.
        void shutdown()
        {
            stopProcessing();
            while (isProcessing())
                Sleep(10);
            if (worker_thread) {
                ASSERT(worker_thread->joinable());
                worker_thread->join();
                delete worker_thread;
                worker_thread = nullptr;
            }
        }

        int progress()
        {
            return m_progress;
        }

        bool ready()
        {
            return m_progress >= 100;
        }

        MapSpecific::MapSpecificData m_msd;

        // Portal is a helper contruct between pathing trapezoids and it represents a line through which it
        // is possible to cross from one pathing trapezoid into another.
        class Portal {
        public:
            Portal(const GW::Vec2f& start, const GW::Vec2f& goal, const AABB* box1, const AABB* box2);
            inline bool intersect(const GW::Vec2f& p1, const GW::Vec2f& p2) const;

            const AABB* m_box1,* m_box2;
            GW::Vec2f m_start, m_goal;
        };

        class point {
        public:
            typedef int Id;
            Id id = 0;
            GW::Vec2f pos = {};
            const AABB* box = nullptr;
            const AABB* box2 = nullptr;
            const Portal* portal = nullptr;

            operator GW::GamePos() const
            {
                return {pos.x, pos.y, box && box->m_t ? box->m_t->layer : 0};
            }
        };

        struct PointVisElement {
            point::Id point_id; // other point
            float distance;
            std::vector<uint32_t> blocking_ids; // Holds all layer changes; for checking if it's passable or blocked.
        } ;

        std::vector<AABB> m_aabbs;
        std::vector<SimplePT> m_trapezoids;
        std::vector<std::vector<PointVisElement>> m_visGraph;    // [point.id]
        std::vector<std::vector<const AABB*>> m_AABBgraph;       // [box.id]
        std::vector<Portal> m_portals;                           // [portal.id]
        std::vector<std::vector<const Portal*>> m_PTPortalGraph; // [simple_pt.id]
        std::vector<point> m_points;                             // [point.id]
        MapSpecific::Teleports m_teleports;
        std::vector<GW::MapProp*> travel_portals;
        std::vector<MapSpecific::teleport_node> m_teleportGraph;

        // Generate distance graph among teleports
        void GenerateTeleportGraph();
        MilePath::point CreatePoint(const GW::GamePos& pos);

        bool HasLineOfSight(const point& start, const point& goal,
                            std::vector<const AABB*>& open, std::vector<bool>& visited,
                            std::vector<uint32_t>* blocking_ids = nullptr);

        const AABB* FindAABB(const GW::GamePos& pos);
        bool IsOnPathingTrapezoid(const GW::Vec2f& p, const SimplePT** pt = nullptr);

        // Get the nearest point on the map that is within a trapezoid
        GW::GamePos GetClosestPoint(const GW::GamePos& pos);

    private:
        void LoadMapSpecificData();


        // Generate Axis Aligned Bounding Boxes around trapezoids
        // This is used for quick intersection checks.
        void GenerateAABBs();

        bool CreatePortal(const AABB* box1, const AABB* box2, const SimplePT::adjacentSide& ts);

        // Connect trapezoid AABBS.
        void GenerateAABBGraph();

        void GeneratePoints();

        void GenerateVisibilityGraph();

        enum class teleport_point_type : uint8_t { enter, exit, both } ;

        void insertTeleportPointIntoVisGraph(MilePath::point& point, teleport_point_type type);
        void InsertTeleportsIntoVisibilityGraph();
    };

    class AStar {
    public:
        class Path {
        public:
            Path(AStar* _parent)
                : m_astar(_parent),
                  m_cost(0.0f) {}

            const std::vector<MilePath::point>& points()
            {
                return m_points;
            }

            float cost() const
            {
                return m_cost;
            }

            bool ready() const
            {
                return finalized;
            }

            void clear()
            {
                finalized = false;
                m_points.clear();
            }

            void insertPoint(const MilePath::point& point)
            {
                m_points.emplace_back(point);
            }

            void setCost(float cost)
            {
                m_cost = cost;
            }

            void finalize()
            {
                if (!finalized)
                    std::ranges::reverse(m_points);
                finalized = true;
            }

        private:
            bool finalized = false;
            AStar* m_astar;
            std::vector<MilePath::point> m_points;
            float m_cost; // distance
        };

        Path m_path;

        AStar(MilePath* mp);

        void InsertPointIntoVisGraph(MilePath::point& point) const;

        Error BuildPath(const MilePath::point& start, const MilePath::point& goal, const std::vector<MilePath::point::Id>& came_from);

        inline float TeleporterHeuristic(const MilePath::point& start, const MilePath::point& goal) const;

        Error Search(const GW::GamePos& start_pos, const GW::GamePos& goal_pos);

        GW::GamePos GetClosestPoint(const GW::Vec2f& pos);
        static GW::GamePos GetClosestPoint(Path& path, const GW::Vec2f& pos);

    private:
        std::vector<std::vector<MilePath::PointVisElement>> m_visGraph{};
        MilePath* m_mp;
    };
}
