FROM python:3.11-slim

# Working directory inside the container
WORKDIR /app

# Install Python dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy the project files
COPY . .

# Entry point
ENTRYPOINT ["python", "src/main.py"]