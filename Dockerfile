FROM python:3.11-slim

# Wotking directory in Docker
WORKDIR /app

# Installing requirments
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy the project files
COPY . .

# Start command
ENTRYPOINT ["python", "src/main.py"]