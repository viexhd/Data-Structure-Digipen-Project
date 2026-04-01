#pragma once
#include <cstddef>
#include <memory>
#include <vector>

// A vertex node in a doubly-linked circular list representing one ring.
struct Vertex {
    double x, y;
    int    original_id;   // vertex_id from input CSV (for reference)
    bool   removed;       // tombstone: set true when unlinked, deferred delete
    Vertex* prev;
    Vertex* next;
    mutable unsigned long long last_query_epoch;  // for spatial grid deduplication

    Vertex(double x, double y, int id = -1)
        : x(x), y(y), original_id(id), removed(false), prev(nullptr), next(nullptr),
          last_query_epoch(0) {}
};

// One ring (exterior or interior) stored as a circular doubly-linked list.
// Ownership: Ring owns all its Vertex nodes (allocated/freed here).
class Ring {
public:
    int    ring_id;
    size_t size;   // current vertex count
    Vertex* head;  // arbitrary entry point into the circle

    explicit Ring(int id) : ring_id(id), size(0), head(nullptr), next_generated_id(0), garbage() {}
    ~Ring() { flush_garbage(); clear(); }

    // Non-copyable (owns raw pointers)
    Ring(const Ring&)            = delete;
    Ring& operator=(const Ring&) = delete;

    // Append a vertex to the end of the ring (before head, maintaining circularity).
    void push_back(double x, double y, int vid = -1) {
        Vertex* v = new Vertex(x, y, vid);
        if (vid >= 0 && vid + 1 > next_generated_id) next_generated_id = vid + 1;
        if (!head) {
            v->prev = v;
            v->next = v;
            head = v;
        } else {
            Vertex* tail = head->prev;
            tail->next = v;
            v->prev    = tail;
            v->next    = head;
            head->prev = v;
        }
        ++size;
    }

    // Unlink vertex v from the ring and mark it removed (tombstone).
    // Does NOT free memory — deferred to allow safe stale-candidate checks.
    // Returns the next vertex (for iteration).
    // Caller must ensure size >= 3 after removal to keep ring valid.
    Vertex* remove(Vertex* v) {
        Vertex* nxt = v->next;
        v->prev->next = v->next;
        v->next->prev = v->prev;
        if (head == v) head = nxt;
        v->removed = true;
        --size;
        garbage.push_back(v);  // deferred delete
        return nxt;
    }

    // Free all tombstoned vertices accumulated since last flush.
    void flush_garbage() {
        for (Vertex* v : garbage) delete v;
        garbage.clear();
    }

    // Insert a new vertex with coordinates (x,y) between prev_v and prev_v->next.
    // Returns the newly inserted vertex.
    Vertex* insert_after(Vertex* prev_v, double x, double y) {
        Vertex* v   = new Vertex(x, y, next_generated_id++);
        Vertex* nxt = prev_v->next;
        prev_v->next = v;
        v->prev      = prev_v;
        v->next      = nxt;
        nxt->prev    = v;
        ++size;
        return v;
    }

private:
    int next_generated_id;
    std::vector<Vertex*> garbage;  // tombstoned vertices awaiting deletion

    void clear() {
        if (!head) return;
        Vertex* cur = head->next;
        while (cur != head) {
            Vertex* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
        delete head;
        head = nullptr;
        size = 0;
    }
};
