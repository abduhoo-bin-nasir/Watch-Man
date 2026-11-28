// Scheduler.h - a hand-written binary min-heap and the event scheduler built on it.
//
// Why a heap: the game has things that must happen LATER, at a time worked out now - thunder that arrives a few seconds
// after the lightning, a meow in twenty seconds, a door slamming somewhere in the building. Events are pushed in any order
// and popped in time order: push and pop are O(log n), "when is the next one?" is O(1). (Later: schedules for the people
// of the building, the lift cut at 22:00, the Watch's outbursts - same structure, more event types.)
//
// Removal test (does gameplay notice?): without it the thunder would have to be a timer per bolt, every random sound its own
// countdown, and nothing could be added without touching Game.cpp.
#pragma once
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

// A binary min-heap stored in a vector: the children of slot i are 2i+1 and 2i+2, a parent is never larger than its children.
template <class T, class Less = std::less<T>>
class MinHeap {
public:
    bool empty() const { return a.empty(); }
    size_t size() const { return a.size(); }
    const T& top() const { return a.front(); }          // the smallest; only call when not empty()
    void clear() { a.clear(); }

    void push(const T& v) {
        a.push_back(v);
        size_t i = a.size() - 1;
        while (i > 0) {                                  // sift up
            size_t p = (i - 1) / 2;
            if (!less(a[i], a[p])) break;
            std::swap(a[i], a[p]);
            i = p;
        }
    }
    T pop() {                                            // removes and returns the smallest; only call when not empty()
        T result = std::move(a.front());
        a.front() = std::move(a.back());
        a.pop_back();
        size_t i = 0, n = a.size();
        while (true) {                                   // sift down
            size_t l = 2 * i + 1, r = l + 1, m = i;
            if (l < n && less(a[l], a[m])) m = l;
            if (r < n && less(a[r], a[m])) m = r;
            if (m == i) break;
            std::swap(a[i], a[m]);
            i = m;
        }
        return result;
    }
    bool isHeap() const {                                // for tests: the heap property holds everywhere
        for (size_t i = 1; i < a.size(); i++) if (less(a[i], a[(i - 1) / 2])) return false;
        return true;
    }
private:
    std::vector<T> a;
    Less less;
};

struct SchedEvent {
    double time = 0;          // game time at which it fires
    uint64_t seq = 0;         // order of scheduling: two events with the same time fire in the order they were scheduled
    int type = 0;             // what happens (the game defines the numbers)
    float a = 0, b = 0, c = 0;   // three numbers of payload (which variant, how loud, where...)
};

class EventScheduler {
public:
    void schedule(double when, int type, float a = 0, float b = 0, float c = 0) {
        SchedEvent e; e.time = when; e.seq = seq++; e.type = type; e.a = a; e.b = b; e.c = c;
        heap.push(e);
    }
    // Takes the earliest event that is due at 'now', if there is one. Call in a loop to take them all.
    bool popDue(double now, SchedEvent& out) {
        if (heap.empty() || heap.top().time > now) return false;
        out = heap.pop();
        return true;
    }
    bool empty() const { return heap.empty(); }
    size_t size() const { return heap.size(); }
    double nextTime() const { return heap.empty() ? 1e300 : heap.top().time; }
    void clear() { heap.clear(); }
    bool valid() const { return heap.isHeap(); }
private:
    struct Earlier {
        bool operator()(const SchedEvent& x, const SchedEvent& y) const { return x.time < y.time || (x.time == y.time && x.seq < y.seq); }
    };
    MinHeap<SchedEvent, Earlier> heap;
    uint64_t seq = 0;
};
