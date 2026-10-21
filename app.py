import os
import subprocess
import tempfile
from pathlib import Path

from django.shortcuts import render
from django.urls import path

EXE = Path(__file__).resolve().parent / ("scheduler.exe" if os.name == "nt" else "scheduler")


def clean_rooms(text):
    """Validate 'Name:Capacity,Name:Capacity' and return it normalised."""
    rooms = []
    for part in text.split(","):
        name, _, cap = part.partition(":")
        if not name.strip() or not cap.strip().isdigit() or int(cap) <= 0:
            raise ValueError(f"Invalid room entry: '{part.strip()}'. Use Name:Capacity.")
        rooms.append(f"{name.strip()}:{int(cap)}")
    return ",".join(rooms)


def run_engine(csv_file, max_slots, rooms):
    if not EXE.exists():
        raise ValueError("Engine not compiled. Run: g++ scheduler.cpp -std=c++17 -O2 -o scheduler")

    with tempfile.TemporaryDirectory() as tmp:
        csv_path = Path(tmp) / "input.csv"
        csv_path.write_bytes(csv_file.read())
        result = subprocess.run([str(EXE), str(csv_path), str(max_slots), rooms],
                                capture_output=True, text=True, timeout=30)

    lines = result.stdout.strip().splitlines()
    if not lines:
        raise ValueError("Engine produced no output.")
    if lines[0].startswith("ERROR"):
        raise ValueError(lines[0][6:])

    students, subjects, conflicts, slots = map(int, lines[1].split())
    rows = []
    for line in lines[2:]:
        slot, subject, room, count, cap = line.split("|")
        slot = int(slot)
        rows.append({"day": slot // 2 + 1,
                     "session": "Morning" if slot % 2 == 0 else "Afternoon",
                     "subject": subject, "room": room,
                     "students": count, "capacity": cap})
    return {"students": students, "subjects": subjects,
            "conflicts": conflicts, "slots": slots, "rows": rows}


def index(request):
    if request.method != "POST":
        return render(request, "index.html")

    ctx = {"max_slots": request.POST.get("max_slots", "8"),
           "room_config": request.POST.get("room_config", "")}
    try:
        csv_file = request.FILES.get("csv_file")
        if not csv_file:
            raise ValueError("Please upload a CSV file.")
        max_slots = int(ctx["max_slots"])
        if not 1 <= max_slots <= 50:
            raise ValueError("Max slots must be between 1 and 50.")
        ctx.update(run_engine(csv_file, max_slots, clean_rooms(ctx["room_config"])))
    except ValueError as e:
        ctx["error"] = str(e)
    except subprocess.TimeoutExpired:
        ctx["error"] = "Scheduling took too long. Try more slots or less data."
    return render(request, "index.html", ctx)


urlpatterns = [path("", index)]