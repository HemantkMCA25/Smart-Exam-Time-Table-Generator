#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <numeric>

using namespace std;

struct Room { string name; int capacity; };
struct Subject { int id; string name; int studentCount = 0; };
struct TimetableEntry { string day, timeSlot, subjectName, roomName; int studentCount, roomCapacity; };

string trim(const string& str) {
    size_t f = str.find_first_not_of(" \r\n\t"), l = str.find_last_not_of(" \r\n\t");
    return (f == string::npos) ? "" : str.substr(f, l - f + 1);
}

vector<Room> parseRoomConfig(const string& roomConfigStr) {
    vector<Room> rooms;
    stringstream ss(roomConfigStr);
    string token;
    while (getline(ss, token, ',')) {
        size_t colon = token.find(':');
        if (colon != string::npos) {
            rooms.push_back({trim(token.substr(0, colon)), stoi(trim(token.substr(colon + 1)))});
        }
    }
    sort(rooms.begin(), rooms.end(), [](const Room& a, const Room& b) { return a.capacity > b.capacity; });
    return rooms;
}

bool solveGraphColoring(int idx, int maxSlots, int total, const vector<int>& order, const vector<unordered_set<int>>& adj, vector<int>& slots) {
    if (idx == total) return true;
    int u = order[idx];
    for (int s = 0; s < maxSlots; ++s) {
        bool valid = true;
        for (int v : adj[u]) {
            if (slots[v] == s) { valid = false; break; }
        }
        if (valid) {
            slots[u] = s;
            if (solveGraphColoring(idx + 1, maxSlots, total, order, adj, slots)) return true;
            slots[u] = -1;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 2) { cerr << "Usage: scheduler <csv_file> [max_slots] [room_config]\n"; return 1; }
    
    string filePath = argv[1];
    int maxSlots = (argc >= 3) ? stoi(argv[2]) : 10;
    vector<Room> rooms = parseRoomConfig((argc >= 4) ? argv[3] : "Room A:60,Room B:40,Room C:30");

    ifstream file(filePath);
    if (!file.is_open()) { cerr << "Error opening file\n"; return 1; }

    unordered_set<string> uniqueStudents;
    unordered_map<string, int> nameToId;
    vector<Subject> subjects;
    vector<vector<int>> studentEnrollments;

    string line;
    getline(file, line); // Skip header

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token, roll;
        getline(ss, roll, ',');
        roll = trim(roll);
        if (roll.empty()) continue;

        uniqueStudents.insert(roll);
        vector<int> subs;

        while (getline(ss, token, ',')) {
            string name = trim(token);
            if (name.empty()) continue;
            if (!nameToId.count(name)) {
                nameToId[name] = subjects.size();
                subjects.push_back({(int)subjects.size(), name, 0});
            }
            int id = nameToId[name];
            subs.push_back(id);
            subjects[id].studentCount++;
        }
        if (!subs.empty()) studentEnrollments.push_back(move(subs));
    }

    int totalSubjects = subjects.size();
    vector<unordered_set<int>> adj(totalSubjects);
    for (const auto& subs : studentEnrollments) {
        for (size_t i = 0; i < subs.size(); ++i) {
            for (size_t j = i + 1; j < subs.size(); ++j) {
                if (subs[i] != subs[j]) {
                    adj[subs[i]].insert(subs[j]);
                    adj[subs[j]].insert(subs[i]);
                }
            }
        }
    }

    vector<int> subjectOrder(totalSubjects), slots(totalSubjects, -1);
    iota(subjectOrder.begin(), subjectOrder.end(), 0);
    sort(subjectOrder.begin(), subjectOrder.end(), [&](int a, int b) { return adj[a].size() > adj[b].size(); });

    bool success = solveGraphColoring(0, maxSlots, totalSubjects, subjectOrder, adj, slots);

    int totalConflicts = 0, usedSlots = 0;
    for (int i = 0; i < totalSubjects; ++i) {
        usedSlots = max(usedSlots, slots[i] + 1);
        for (int neighbor : adj[i]) if (i < neighbor) totalConflicts++;
    }

    vector<vector<int>> subjectsBySlot(usedSlots);
    for (int i = 0; i < totalSubjects; ++i) if (slots[i] != -1) subjectsBySlot[slots[i]].push_back(i);

    vector<string> days = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    vector<string> timeSlots = {"09:00", "14:00"};
    vector<TimetableEntry> timetable;
    bool overflow = false;

    for (int s = 0; s < usedSlots; ++s) {
        string day = days[(s / 2) % days.size()], time = timeSlots[s % 2];
        auto slotSubs = subjectsBySlot[s];
        sort(slotSubs.begin(), slotSubs.end(), [&](int a, int b) { return subjects[a].studentCount > subjects[b].studentCount; });

        vector<bool> occupied(rooms.size(), false);
        for (int subId : slotSubs) {
            bool assigned = false;
            for (size_t r = 0; r < rooms.size(); ++r) {
                if (!occupied[r] && rooms[r].capacity >= subjects[subId].studentCount) {
                    occupied[r] = assigned = true;
                    timetable.push_back({day, time, subjects[subId].name, rooms[r].name, subjects[subId].studentCount, rooms[r].capacity});
                    break;
                }
            }
            if (!assigned) {
                overflow = true;
                timetable.push_back({day, time, subjects[subId].name, rooms[0].name + " (OVERFLOW)", subjects[subId].studentCount, rooms[0].capacity});
            }
        }
    }

    // JSON Formatting
    cout << "{\n  \"success\": " << (success && !overflow ? "true" : "false")
         << ",\n  \"method\": \"Backtracking + First-Fit Decreasing\""
         << ",\n  \"students_count\": " << uniqueStudents.size()
         << ",\n  \"subjects_count\": " << totalSubjects
         << ",\n  \"conflicts_count\": " << totalConflicts
         << ",\n  \"slots_count\": " << usedSlots
         << ",\n  \"capacity_overflow\": " << (overflow ? "true" : "false")
         << ",\n  \"timetable\": [\n";

    for (size_t i = 0; i < timetable.size(); ++i) {
        const auto& t = timetable[i];
        cout << "    {\"day\":\"" << t.day << "\",\"time\":\"" << t.timeSlot
             << "\",\"subject\":\"" << t.subjectName << "\",\"room\":\"" << t.roomName
             << "\",\"students\":" << t.studentCount << ",\"capacity\":" << t.roomCapacity << "}"
             << (i + 1 < timetable.size() ? ",\n" : "\n");
    }
    cout << "  ],\n  \"conflicts\": [\n";

    bool firstConflict = true;
    for (int i = 0; i < totalSubjects; ++i) {
        for (int neighbor : adj[i]) {
            if (i < neighbor) {
                if (!firstConflict) cout << ",\n";
                cout << "    {\"from\":\"" << subjects[i].name << "\",\"to\":\"" << subjects[neighbor].name << "\"}";
                firstConflict = false;
            }
        }
    }
    cout << "\n  ]\n}\n";
    return 0;
}