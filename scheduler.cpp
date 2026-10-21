#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \r\n\t"), b = s.find_last_not_of(" \r\n\t");
    return a == string::npos ? "" : s.substr(a, b - a + 1);
}

static int fail(const string& msg) {
    cout << "ERROR " << msg << "\n";
    return 0;
}

// Backtracking graph coloring: give subject order[i] a slot, recurse, undo if stuck.
// 'used' = slots opened so far; opening only the next new slot avoids duplicate work.
static bool colorGraph(size_t i, int maxSlots, int used, const vector<int>& order,
                       const vector<set<int>>& adj, vector<int>& slot) {
    if (i == order.size()) return true;
    int u = order[i];
    for (int s = 0; s < maxSlots && s <= used; ++s) {
        bool clash = false;
        for (int v : adj[u]) if (slot[v] == s) { clash = true; break; }
        if (clash) continue;
        slot[u] = s;
        if (colorGraph(i + 1, maxSlots, max(used, s + 1), order, adj, slot)) return true;
        slot[u] = -1;
    }
    return false;
}

// Independent re-check of the final timetable. Returns "" if valid.
static string verify(const vector<vector<int>>& enrolments, const vector<int>& slot,
                     const vector<array<int, 3>>& assign,   // {slot, roomIndex, subject}
                     const vector<pair<string, int>>& rooms, const vector<int>& count) {
    for (size_t k = 0; k < enrolments.size(); ++k) {
        set<int> seen;
        for (int s : enrolments[k])
            if (!seen.insert(slot[s]).second)
                return "student #" + to_string(k + 1) + " has two exams in the same slot";
    }
    set<pair<int, int>> used;
    for (auto& a : assign) {
        if (!used.insert({a[0], a[1]}).second) return "a room is double-booked in one slot";
        if (rooms[a[1]].second < count[a[2]]) return "a room is over capacity";
    }
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 4) return fail("Usage: scheduler <csv> <max_slots> <rooms>");
    int maxSlots = atoi(argv[2]);

    // Rooms "Name:Capacity,...", largest first
    vector<pair<string, int>> rooms;
    stringstream rs(argv[3]);
    string part;
    while (getline(rs, part, ',')) {
        size_t c = part.find(':');
        if (c != string::npos) rooms.push_back({trim(part.substr(0, c)), atoi(part.substr(c + 1).c_str())});
    }
    sort(rooms.begin(), rooms.end(), [](auto& a, auto& b) { return a.second > b.second; });

    ifstream in(argv[1]);
    if (!in) return fail("Cannot open CSV file.");
    if (rooms.empty() || maxSlots < 1) return fail("Invalid slot or room settings.");

    // Read CSV: roll_no, subject, subject, ...
    map<string, int> id;
    vector<string> names;
    vector<int> count;
    vector<vector<int>> enrolments;
    string line, cell;
    getline(in, line);  // header
    while (getline(in, line)) {
        stringstream ls(line);
        getline(ls, cell, ',');
        if (trim(cell).empty()) continue;
        set<int> subs;
        while (getline(ls, cell, ',')) {
            string name = trim(cell);
            if (name.empty()) continue;
            if (!id.count(name)) { id[name] = names.size(); names.push_back(name); count.push_back(0); }
            subs.insert(id[name]);
        }
        for (int s : subs) count[s]++;
        enrolments.push_back(vector<int>(subs.begin(), subs.end()));
    }
    int n = names.size();
    if (n == 0) return fail("No subjects found in the CSV.");

    // Conflict graph: two subjects taken by the same student are connected
    vector<set<int>> adj(n);
    for (auto& subs : enrolments)
        for (size_t i = 0; i < subs.size(); ++i)
            for (size_t j = i + 1; j < subs.size(); ++j) {
                adj[subs[i]].insert(subs[j]);
                adj[subs[j]].insert(subs[i]);
            }
    int edges = 0;
    for (auto& a : adj) edges += a.size();
    edges /= 2;

    // Most-constrained subjects first (Welsh-Powell order)
    vector<int> order(n), slot(n, -1);
    for (int i = 0; i < n; ++i) order[i] = i;
    sort(order.begin(), order.end(), [&](int a, int b) { return adj[a].size() > adj[b].size(); });

    if (!colorGraph(0, maxSlots, 0, order, adj, slot))
        return fail("No clash-free timetable exists within " + to_string(maxSlots) + " slots. Increase the slot limit.");

    int usedSlots = *max_element(slot.begin(), slot.end()) + 1;

    // Rooms: within each slot, biggest exam gets the first free room that fits
    vector<array<int, 3>> assign;
    ostringstream out;
    for (int s = 0; s < usedSlots; ++s) {
        vector<int> subs;
        for (int i = 0; i < n; ++i) if (slot[i] == s) subs.push_back(i);
        sort(subs.begin(), subs.end(), [&](int a, int b) { return count[a] > count[b]; });

        vector<bool> taken(rooms.size(), false);
        for (int sub : subs) {
            size_t r = 0;
            while (r < rooms.size() && (taken[r] || rooms[r].second < count[sub])) ++r;
            if (r == rooms.size())
                return fail("Not enough room capacity for '" + names[sub] + "' (" +
                            to_string(count[sub]) + " students). Add or enlarge rooms.");
            taken[r] = true;
            assign.push_back({s, (int)r, sub});
            out << s << "|" << names[sub] << "|" << rooms[r].first << "|"
                << count[sub] << "|" << rooms[r].second << "\n";
        }
    }

    string problem = verify(enrolments, slot, assign, rooms, count);
    if (!problem.empty()) return fail("Internal check failed: " + problem);

    cout << "OK\n" << enrolments.size() << " " << n << " " << edges << " " << usedSlots << "\n" << out.str();
    return 0;
}
