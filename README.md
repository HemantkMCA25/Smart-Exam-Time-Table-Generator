# Smart Exam Timetable Generator

Django website + C++ DSA scheduling engine.

## Architecture

Django handles the UI and input/output.
C++ performs the actual scheduling:

- `unordered_map` → student/subject data
- adjacency-list graph → subject conflicts
- `priority_queue` → most-constrained-first ordering
- greedy graph coloring → exam-slot assignment
- greedy room allocation → room capacity
- validation → checks student conflicts and room reuse

## Windows setup

Install a C++ compiler such as MinGW g++ and Python.

### 1. Create environment

```bash
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
```

### 2. Compile the C++ engine

From the project folder:

```bash
g++ scheduler/dsa.cpp -std=c++17 -O2 -o scheduler/scheduler.exe
```

### 3. Run Django

```bash
python manage.py migrate
python manage.py runserver
```

Open:

http://127.0.0.1:8000/

## CSV format

```csv
roll_no,subject1,subject2,subject3,subject4
MCA001,DBMS,DAA,OS,ML
MCA002,DBMS,DAA,CN,AI
```

Multiple CSV files can be uploaded together.

## Important algorithm idea

If two students take the same two subjects, those subjects have a conflict edge.

For example:

`MCA001 -> DBMS, DAA`

creates:

`DBMS <-> DAA`

A subject can share an exam slot only with subjects that have no conflict edge with it.

The project intentionally keeps the AI layer out of the first version. Once the deterministic scheduler is stable, a small LLM feature can be added to explain the generated timetable.
