# Exam Scheduler

Django UI + C++ engine. Subjects are graph nodes; a shared student creates a conflict edge.
Backtracking graph coloring assigns slots, first-fit decreasing assigns rooms, and a final
verifier re-checks that no student has two exams in one slot and no room is double-booked.

## Run
```bash
pip install django
g++ scheduler.cpp -std=c++17 -O2 -o scheduler      # Windows: -o scheduler.exe
python manage.py runserver
```
Open http://127.0.0.1:8000/ and upload `sample_data.csv`.

## CSV format
```
roll_no,subject1,subject2,subject3
MCA001,DBMS,DAA,OS
```
Slot mapping: two sessions per day (Morning, Afternoon).
