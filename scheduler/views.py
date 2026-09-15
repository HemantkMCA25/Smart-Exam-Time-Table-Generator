import os
import json
import time
import subprocess
from pathlib import Path
from django.shortcuts import render, redirect

try:
    from google import genai
except ImportError:
    genai = None


def generate_ai_explanation(data):
    if not genai or not os.environ.get("GEMINI_API_KEY"):
        return (
            f"💡 [AI Layer Offline] Scheduled {data.get('subjects_count', 0)} subjects across "
            f"{data.get('slots_count', 0)} slots for {data.get('students_count', 0)} students."
        )

    try:
        client = genai.Client(api_key=os.environ["GEMINI_API_KEY"])
        prompt = (
            "Analyze this exam timetable JSON from a C++ graph-coloring engine:\n"
            f"{json.dumps(data, indent=2)}\n\n"
            "Provide 3 bullet points without LaTeX formulas covering:\n"
            "1. Bottleneck & Degree Analysis (Welsh-Powell order)\n"
            "2. Room Allocation Strategy (First-Fit Decreasing)\n"
            "3. Chromatic Efficiency & zero conflict assurance"
        )
        response = client.models.generate_content(
            model='gemini-3.6-flash', contents=prompt
        )
        return response.text
    except Exception as e:
        return f"AI Explanation generation failed: {e}"


def home(request):
    return render(request, 'home.html')


def upload_file(request):
    if request.method != 'POST' or 'csv_file' not in request.FILES:
        return redirect('home')

    csv_file = request.FILES['csv_file']
    max_slots = request.POST.get('max_slots', '10')
    room_config = request.POST.get('room_config', 'Room A:60,Room B:40,Room C:30')

    base_dir = Path(__file__).resolve().parent.parent
    executable = base_dir / 'scheduler' / ('scheduler.exe' if os.name == 'nt' else 'scheduler')

    if not executable.exists():
        return render(request, 'home.html', {'error': f'Executable missing at {executable}. Compile dsa.cpp first.'})

    temp_path = base_dir / f"temp_{int(time.time())}.csv"
    try:
        with open(temp_path, 'wb+') as destination:
            for chunk in csv_file.chunks():
                destination.write(chunk)

        start = time.perf_counter()
        result = subprocess.run(
            [str(executable), str(temp_path), str(max_slots), room_config],
            capture_output=True, text=True, check=True
        )
        elapsed_ms = round((time.perf_counter() - start) * 1000, 2)

        data = json.loads(result.stdout)
        data['execution_time_ms'] = elapsed_ms
        data['ai_explanation'] = generate_ai_explanation(data)
        data['raw_json'] = json.dumps(data)

        return render(request, 'result.html', {'data': data})
    except Exception as e:
        return render(request, 'home.html', {'error': str(e)})
    finally:
        if temp_path.exists():
            temp_path.unlink()