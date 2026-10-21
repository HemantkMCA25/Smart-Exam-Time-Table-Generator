from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
SECRET_KEY = "dev-only-change-me"
DEBUG = True
ALLOWED_HOSTS = ["*"]
ROOT_URLCONF = "app"
INSTALLED_APPS = []
MIDDLEWARE = ["django.middleware.csrf.CsrfViewMiddleware"]
TEMPLATES = [{
    "BACKEND": "django.template.backends.django.DjangoTemplates",
    "DIRS": [BASE_DIR / "templates"],
}]